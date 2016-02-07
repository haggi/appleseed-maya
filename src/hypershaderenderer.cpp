
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016 Haggi Krey, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "hypershaderenderer.h"

#if MAYA_API_VERSION >= 201600

// appleseed-maya headers.
#include "osl/oslutils.h"
#include "shadingtools/material.h"
#include "shadingtools/shadingutils.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "utilities/meshtools.h"
#include "utilities/tools.h"
#include "appleseedutils.h"
#include "mayatoworld.h"

#include "renderer/api/edf.h"
#include "renderer/api/environment.h"
#include "renderer/api/environmentedf.h"
#include "renderer/api/environmentshader.h"
#include "renderer/api/texture.h"
#include "renderer/global/globallogger.h"
#include "renderer/modeling/shadergroup/shadergroup.h"

// appleseed.renderer headers.
#include "renderer/api/bsdf.h"
#include "renderer/api/camera.h"
#include "renderer/api/color.h"
#include "renderer/api/environment.h"
#include "renderer/api/frame.h"
#include "renderer/api/light.h"
#include "renderer/api/log.h"
#include "renderer/api/material.h"
#include "renderer/api/object.h"
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"
#include "renderer/api/scene.h"
#include "renderer/api/surfaceshader.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/core/appleseed.h"
#include "foundation/image/image.h"
#include "foundation/image/tile.h"
#include "foundation/math/matrix.h"
#include "foundation/math/scalar.h"
#include "foundation/math/transform.h"
#include "foundation/utility/containers/specializedarrays.h"
#include "foundation/utility/log/consolelogtarget.h"
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/searchpaths.h"

// Maya headers.
#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPlugArray.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MStringArray.h>

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

// Standard headers.
#include <vector>

#define kNumChannels 4
#define GETASM() project->get_scene()->assemblies().get_by_name("world")

HypershadeRenderer::HypershadeRenderer()
{
    initProject();
    tileSize = 32;
    initialSize = 256;
    width = height = initialSize;
    lastMaterialName = "default";
    this->rb = (float*)malloc(width*height*kNumChannels*sizeof(float));
    asyncStarted = false;
}

HypershadeRenderer::~HypershadeRenderer()
{
    mrenderer.reset();
    project.release();
    objectArray.clear();
}

void HypershadeRenderer::initEnv()
{
    MString mayaRoot = getenv("MAYA_LOCATION");
    MString envMapAttrName = mayaRoot + "/presets/Assets/IBL/Exterior1_Color.exr";

    asr::ParamArray fileparams;
    fileparams.insert("filename", envMapAttrName.asChar());
    fileparams.insert("color_space", "linear_rgb");

    asf::SearchPaths searchPaths;
    asf::auto_release_ptr<asr::Texture> textureElement(
        asr::DiskTexture2dFactory().create(
        "envTex",
        fileparams,
        searchPaths));

    project->get_scene()->textures().insert(textureElement);

    asr::ParamArray tInstParams;
    tInstParams.insert("addressing_mode", "clamp");
    tInstParams.insert("filtering_mode", "bilinear");

    MString textureInstanceName = "envTex_texInst";
    asf::auto_release_ptr<asr::TextureInstance> tinst = asr::TextureInstanceFactory().create(
        textureInstanceName.asChar(),
        tInstParams,
        "envTex");
    project->get_scene()->texture_instances().insert(tinst);


    asf::auto_release_ptr<asr::EnvironmentEDF> environmentEDF = asr::LatLongMapEnvironmentEDFFactory().create(
        "sky_edf",
        asr::ParamArray()
        .insert("radiance", textureInstanceName.asChar())
        .insert("radiance_multiplier", 1.0)
        .insert("horizontal_shift", -45.0f)
        .insert("vertical_shift", 10.0f)
        );

    asr::EnvironmentEDF *sky = project->get_scene()->environment_edfs().get_by_name("sky_edf");
    if (sky != 0)
        project->get_scene()->environment_edfs().remove(sky);
    project->get_scene()->environment_edfs().insert(environmentEDF);

    asr::EnvironmentShader *shader = project->get_scene()->environment_shaders().get_by_name("sky_shader");
    if (shader != 0)
        project->get_scene()->environment_shaders().remove(shader);

    project->get_scene()->environment_shaders().insert(
        asr::EDFEnvironmentShaderFactory().create(
        "sky_shader",
        asr::ParamArray()
        .insert("environment_edf", "sky_edf")));

    project->get_scene()->set_environment(
        asr::EnvironmentFactory::create(
        "sky",
        asr::ParamArray()
        .insert("environment_edf", "sky_edf")
        .insert("environment_shader", "sky_shader")));
}

void HypershadeRenderer::initProject()
{
    project = asr::ProjectFactory::create("mayaRendererProject");
    project->add_default_configurations();
    project->configurations().get_by_name("final")->get_parameters().insert("rendering_threads", 16);
    project->configurations().get_by_name("interactive")->get_parameters().insert("rendering_threads", 16);
#ifdef _DEBUG
    project->configurations().get_by_name("final")->get_parameters().insert_path("uniform_pixel_renderer.samples", "4");
#else
    project->configurations().get_by_name("final")->get_parameters().insert_path("uniform_pixel_renderer.samples", "16");
#endif
    defineScene(project.get());
    defineMasterAssembly(project.get());
    defineDefaultMaterial(project.get());

    asr::Configuration *cfg = project->configurations().get_by_name("interactive");
    asr::ParamArray &params = cfg->get_parameters();
    params.insert("sample_renderer", "generic");
    params.insert("sample_generator", "generic");
    params.insert("tile_renderer", "generic");
    params.insert("frame_renderer", "progressive");
    params.insert("lighting_engine", "pt");
    params.insert_path("progressive_frame_renderer.max_fps", "4");
    params.insert_path("progressive_frame_renderer.max_samples", "12000000");

    MColor color(0,0,0);
    defineColor(project.get(), "black", color, 1.0f);

    initEnv();

    width = height = initialSize;
    MString res = MString("") + width + " " + height;
    tileSize = 64;
    MString tileString = MString("") + tileSize + " " + tileSize;

    project->set_frame(
        asr::FrameFactory::create(
        "beauty",
        asr::ParamArray()
        .insert("resolution", res.asChar())
        .insert("tile_size", tileString.asChar())
        .insert("color_space", "linear_rgb")));

    project->get_frame()->get_parameters().insert("pixel_format", "float");

    tileCallbackFac.reset(new HypershadeTileCallbackFactory(this));

    mrenderer.reset(
        new asr::MasterRenderer(
            project.ref(),
            project->configurations().get_by_name("interactive")->get_inherited_parameters(),
            &controller,
            tileCallbackFac.get()));

    for (uint i = 0; i < getWorldPtr()->shaderSearchPath.length(); i++)
    {
        project->search_paths().push_back(getWorldPtr()->shaderSearchPath[i].asChar());
    }
}

bool HypershadeRenderer::isRunningAsync()
{
    return true;
}

void* HypershadeRenderer::creator()
{
    return new HypershadeRenderer();
}

void HypershadeRenderer::render()
{
    ProgressParams pp;
    pp.progress = 0.0;
    progress(pp);

    if (mrenderer.get())
        mrenderer->render();
    else
        return;

    pp.progress = 2.0;
    progress(pp);
}

static void startRenderThread(HypershadeRenderer* renderPtr)
{
    renderPtr->render();
}

MStatus HypershadeRenderer::startAsync(const JobParams& params)
{
    asyncStarted = true;
    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::stopAsync()
{
    controller.status = asr::IRendererController::AbortRendering;
    if (renderThread.joinable())
        renderThread.join();
    asyncStarted = false;
    return MS::kSuccess;
}

MStatus HypershadeRenderer::beginSceneUpdate()
{
    controller.status = asr::IRendererController::AbortRendering;
    if (renderThread.joinable())
        renderThread.join();
    if (project.get() == 0)
        initProject();
    project->get_frame()->clear_main_image();
    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::translateMesh(const MUuid& id, const MObject& node)
{
    MObject mobject = node;
    MFnDependencyNode depFn(mobject);
    MString meshName = depFn.name();
    MString meshIdName = meshName;
    MString meshInstName = meshIdName + "_instance";
    Logging::debug(MString("translateMesh ") + meshIdName);

    asr::Object *obj = GETASM()->objects().get_by_name(meshIdName.asChar());
    if (obj != 0)
    {
        GETASM()->objects().remove(obj);
        GETASM()->bump_version_id();
    }

    asf::auto_release_ptr<asr::MeshObject> mesh = createMesh(mobject);
    mesh->set_name(meshIdName.asChar());
    GETASM()->objects().insert(asf::auto_release_ptr<asr::Object>(mesh));

    IdNameStruct idName;
    idName.id = id;
    idName.name = meshIdName;
    idName.mobject = node;
    objectArray.push_back(idName);
    lastShapeId = id;
    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::translateLightSource(const MUuid& id, const MObject& node)
{
    MFnDependencyNode depFn(node);
    MString lightName = depFn.name();
    MString lightInstName = lightName + "_instance";
    MString lightIdName = lightName + "_" + id;

    Logging::debug(MString("translateLightSource: ") + depFn.name() + " from type: " + node.apiTypeStr());
    if (node.hasFn(MFn::kAreaLight))
    {
        asr::MeshObject *meshPtr = (asr::MeshObject *)GETASM()->objects().get_by_name(lightIdName.asChar());
        if (meshPtr != 0)
            GETASM()->objects().remove(meshPtr);

        asf::auto_release_ptr<asr::MeshObject> plane = defineStandardPlane();
        plane->set_name(lightIdName.asChar());
        GETASM()->objects().insert(asf::auto_release_ptr<asr::Object>(plane));

        meshPtr = (asr::MeshObject *)GETASM()->objects().get_by_name(lightIdName.asChar());

        MString areaLightMaterialName = lightIdName + "_material";
        MString physicalSurfaceName = lightIdName + "_physical_surface_shader";
        MString areaLightColorName = lightIdName + "_color";
        MString edfName = lightIdName + "_edf";
        asr::ParamArray edfParams;
        MColor color = getColorAttr("color", depFn);
        defineColor(project.get(), areaLightColorName.asChar(), color, getFloatAttr("intensity", depFn, 1.0f) * 1);
        edfParams.insert("radiance", areaLightColorName.asChar());

        asr::EDF *edfPtr = GETASM()->edfs().get_by_name(edfName.asChar());
        if (edfPtr != 0)
            GETASM()->edfs().remove(edfPtr);

        asf::auto_release_ptr<asr::EDF> edf = asr::DiffuseEDFFactory().create(edfName.asChar(), edfParams);
        GETASM()->edfs().insert(edf);

        asr::SurfaceShader *ss = GETASM()->surface_shaders().get_by_name(physicalSurfaceName.asChar());
        if (ss != 0)
            GETASM()->surface_shaders().remove(ss);

        GETASM()->surface_shaders().insert(
            asr::PhysicalSurfaceShaderFactory().create(
            physicalSurfaceName.asChar(),
            asr::ParamArray()));

        asr::Material *ma = GETASM()->materials().get_by_name(areaLightMaterialName.asChar());
        if (ma != 0)
            GETASM()->materials().remove(ma);

        GETASM()->materials().insert(
            asr::GenericMaterialFactory().create(
            areaLightMaterialName.asChar(),
            asr::ParamArray()
            .insert("surface_shader", physicalSurfaceName.asChar())
            .insert("edf", edfName.asChar())));

        IdNameStruct idName;
        idName.id = id;
        idName.name = lightIdName;
        idName.mobject = node;
        objectArray.push_back(idName);
        lastShapeId = id;
    }
    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::translateCamera(const MUuid& id, const MObject& node)
{
    Logging::debug("translateCamera");

    asr::Camera *camera = project->get_scene()->get_camera();
    MFnDependencyNode depFn(node);
    MString camName = depFn.name();

    float imageAspect = (float)width / (float)height;
    asr::ParamArray camParams;
    float horizontalFilmAperture = getFloatAttr("horizontalFilmAperture", depFn, 24.892f);
    float verticalFilmAperture = getFloatAttr("verticalFilmAperture", depFn, 18.669f);
    float focalLength = getFloatAttr("focalLength", depFn, 35.0f);
    float focusDistance = getFloatAttr("focusDistance", depFn, 10.0f);
    float fStop = getFloatAttr("fStop", depFn, 1000.0f);

    // maya aperture is given in inces so convert to cm and convert to meters
    horizontalFilmAperture = horizontalFilmAperture * 2.54f * 0.01f;
    verticalFilmAperture = verticalFilmAperture * 2.54f * 0.01f;
    verticalFilmAperture = horizontalFilmAperture / imageAspect;
    MString filmBack = MString("") + horizontalFilmAperture + " " + verticalFilmAperture;
    MString focalLen = MString("") + focalLength * 0.001f;

    camParams.insert("film_dimensions", filmBack.asChar());
    camParams.insert("focal_length", focalLen.asChar());
    camParams.insert("focal_distance", (MString("") + focusDistance).asChar());
    camParams.insert("f_stop", (MString("") + fStop).asChar());

    asf::auto_release_ptr<asr::Camera> appleCam = asr::PinholeCameraFactory().create(
        camName.asChar(),
        camParams);
    project->get_scene()->set_camera(appleCam);

    IdNameStruct idName;
    idName.id = id;
    idName.name = camName + "_" + id;
    idName.mobject = node;
    objectArray.push_back(idName);
    lastShapeId = id;
    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::translateEnvironment(const MUuid& id, EnvironmentType type)
{
    Logging::debug("translateEnvironment");
    IdNameStruct idName;
    idName.id = id;
    idName.name = MString("Environment");
    idName.mobject = MObject::kNullObj;
    objectArray.push_back(idName);
    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::translateTransform(const MUuid& id, const MUuid& childId, const MMatrix& matrix)
{
    Logging::debug(MString("translateTransform id: ") + id + " childId " + childId);
    MObject shapeNode;
    IdNameStruct idNameObj;
    std::vector<IdNameStruct>::iterator nsIt;
    for (nsIt = objectArray.begin(); nsIt != objectArray.end(); nsIt++)
    {
        if (nsIt->id == lastShapeId)
        {
            Logging::debug(MString("Found id object for transform: ") + nsIt->name);
            shapeNode = nsIt->mobject;
            idNameObj = *nsIt;
        }
    }
    MString elementInstName = idNameObj.name + "_instance";
    MString elementName = idNameObj.name;
    MMatrix mayaMatrix = matrix;
    asf::Matrix4d appleMatrix;
    MMatrixToAMatrix(mayaMatrix, appleMatrix);

    if (idNameObj.mobject.hasFn(MFn::kMesh))
    {
        asr::ObjectInstance *objInst = GETASM()->object_instances().get_by_name(elementInstName.asChar());
        if (objInst != 0)
        {
            Logging::debug(MString("Removing already existing inst object: ") + elementInstName);
            GETASM()->object_instances().remove(objInst);
        }
        GETASM()->object_instances().get_by_name(elementInstName.asChar());
        GETASM()->object_instances().insert(
            asr::ObjectInstanceFactory::create(
            elementInstName.asChar(),
            asr::ParamArray(),
            elementName.asChar(),
            asf::Transformd::from_local_to_parent(appleMatrix),
            asf::StringDictionary()
            .insert("slot0", lastMaterialName),
            asf::StringDictionary()
            .insert("slot0", lastMaterialName)));
    }

    if (idNameObj.mobject.hasFn(MFn::kCamera))
    {
        asr::Camera *cam = project->get_scene()->get_camera();
        cam->transform_sequence().set_transform(0, asf::Transformd::from_local_to_parent(appleMatrix));
    }

    if (idNameObj.mobject.hasFn(MFn::kAreaLight))
    {
        MTransformationMatrix tm;
        double rotate90Deg[3] = { -M_PI_2, 0, 0 };
        tm.setRotation(rotate90Deg, MTransformationMatrix::kXYZ);
        MMatrix lightMatrix = tm.asMatrix() * mayaMatrix;
        MMatrixToAMatrix(lightMatrix, appleMatrix);

        asr::ObjectInstance *objInst = GETASM()->object_instances().get_by_name(elementInstName.asChar());
        if (objInst != 0)
        {
            Logging::debug(MString("Removing already existing inst object: ") + elementInstName);
            GETASM()->object_instances().remove(objInst);
        }
        Logging::debug(MString("area light transform: ") + idNameObj.name);
        MString areaLightMaterialName = elementName + "_material";
        asr::ParamArray instParams;
        instParams.insert_path("visibility.camera", false); // set primary visibility to false for area lights

        GETASM()->object_instances().insert(
            asr::ObjectInstanceFactory::create(
            elementInstName.asChar(),
            instParams,
            elementName.asChar(),
            asf::Transformd::from_local_to_parent(appleMatrix),
            asf::StringDictionary()
            .insert("slot0", areaLightMaterialName.asChar()),
            asf::StringDictionary()
            .insert("slot0", areaLightMaterialName.asChar())
            ));
    }

    IdNameStruct idName;
    idName.id = id;
    idName.name = elementInstName;
    idName.mobject = shapeNode;
    objectArray.push_back(idName);

    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::translateShader(const MUuid& id, const MObject& node)
{
    MObject sgNode;
    // at the moment we only support surface shaders connected to a shadingEngine
    if (node.hasFn(MFn::kShadingEngine))
    {
        sgNode = node;
    }
    else
    {
        MPlugArray pa, paOut;
        MFnDependencyNode depFn(node);
        depFn.getConnections(pa);
        asr::Assembly *assembly = project->get_scene()->assemblies().get_by_name("swatchRenderer_world");
        for (uint i = 0; i < pa.length(); i++)
        {
            if (pa[i].isDestination())
                continue;
            pa[i].connectedTo(paOut, false, true);
            if (paOut.length() > 0)
            {
                for (uint k = 0; k < paOut.length(); k++)
                {
                    if (paOut[k].node().hasFn(MFn::kShadingEngine))
                    {
                        sgNode = paOut[k].node();
                        break;
                    }
                }
            }
        }
    }
    asr::Assembly *assembly = GETASM();
    updateMaterial(sgNode, assembly);

    MObject surfaceShaderNode = getConnectedInNode(sgNode, "surfaceShader");
    MString surfaceShaderName = getObjectName(surfaceShaderNode);
    MString shadingGroupName = getObjectName(sgNode);
    MString shaderGroupName = shadingGroupName + "_OSLShadingGroup";

    IdNameStruct idName;
    idName.id = id;
    idName.name = shadingGroupName;
    idName.mobject = node;
    objectArray.push_back(idName);
    lastMaterialName = shadingGroupName;
    return MStatus::kSuccess;
}

void HypershadeRenderer::updateMaterial(const MObject materialNode, const asr::Assembly *assembly)
{
	OSLUtilClass OSLShaderClass;
	MObject surfaceShaderNode = getConnectedInNode(materialNode, "surfaceShader");
	MString surfaceShaderName = getObjectName(surfaceShaderNode);
	MString shadingGroupName = getObjectName(materialNode);
	ShadingNetwork network(surfaceShaderNode);
	size_t numNodes = network.shaderList.size();

    if (assembly->get_name() == "swatchRenderer_world")
	{
		shadingGroupName = "previewSG";
	}

	MString shaderGroupName = shadingGroupName + "_OSLShadingGroup";

	asr::ShaderGroup *shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());

	if (shaderGroup != 0)
	{
		shaderGroup->clear();
	}
	else{
		asf::auto_release_ptr<asr::ShaderGroup> oslShadingGroup = asr::ShaderGroupFactory().create(shaderGroupName.asChar());
		assembly->shader_groups().insert(oslShadingGroup);
		shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());
	}

	OSLShaderClass.group = (OSL::ShaderGroup *)shaderGroup;

	MFnDependencyNode shadingGroupNode(materialNode);
	MPlug shaderPlug = shadingGroupNode.findPlug("surfaceShader");
	OSLShaderClass.createOSLProjectionNodes(shaderPlug);

	for (int shadingNodeId = 0; shadingNodeId < numNodes; shadingNodeId++)
	{
		ShadingNode snode = network.shaderList[shadingNodeId];
		if (shadingNodeId == (numNodes - 1))
			Logging::debug(MString("LastNode Surface Shader: ") + snode.fullName);
		OSLShaderClass.createOSLShadingNode(network.shaderList[shadingNodeId]);
	}

	OSLShaderClass.cleanupShadingNodeList();
	OSLShaderClass.createAndConnectShaderNodes();

	if (numNodes > 0)
	{
		ShadingNode snode = network.shaderList[numNodes - 1];
		MString layer = (snode.fullName + "_interface");
		asr::ShaderGroup *sg = (asr::ShaderGroup *)OSLShaderClass.group;
		sg->add_shader("surface", "surfaceShaderInterface", layer.asChar(), asr::ParamArray());
		const char *srcLayer = snode.fullName.asChar();
		const char *srcAttr = "outColor";
		const char *dstLayer = layer.asChar();
		const char *dstAttr = "inColor";
		sg->add_connection(srcLayer, srcAttr, dstLayer, dstAttr);
	}

	MString physicalSurfaceName = shadingGroupName + "_physical_surface_shader";

	if (assembly->surface_shaders().get_by_name(physicalSurfaceName.asChar()) == 0)
	{
		assembly->surface_shaders().insert(
			asr::PhysicalSurfaceShaderFactory().create(
			physicalSurfaceName.asChar(),
			asr::ParamArray()));
	}
	if (assembly->materials().get_by_name(shadingGroupName.asChar()) == 0)
	{
		assembly->materials().insert(
			asr::OSLMaterialFactory().create(
			shadingGroupName.asChar(),
			asr::ParamArray()
			.insert("surface_shader", physicalSurfaceName.asChar())
			.insert("osl_surface", shaderGroupName.asChar())));
	}
}

MStatus HypershadeRenderer::setProperty(const MUuid& id, const MString& name, bool value)
{
    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::setProperty(const MUuid& id, const MString& name, int value)
{
    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::setProperty(const MUuid& id, const MString& name, float value)
{
    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::setProperty(const MUuid& id, const MString& name, const MString& value)
{
    IdNameStruct idNameObj;
    std::vector<IdNameStruct>::iterator nsIt;
    for (nsIt = objectArray.begin(); nsIt != objectArray.end(); nsIt++)
    {
        if (nsIt->id == id)
        {
            if (nsIt->name == "Environment")
            {
                if (name == "imageFile")
                {
                    asr::Texture *tex = project->get_scene()->textures().get_by_name("envTex");
                    if (tex != 0)
                    {
                        project->get_scene()->textures().remove(tex);
                    }
                    MString imageFile = value;
                    if (value.length() == 0)
                    {
                        MString mayaRoot = getenv("MAYA_LOCATION");
                        imageFile = mayaRoot + "/presets/Assets/IBL/black.exr";
                    }

                    asr::ParamArray& pa = project->get_scene()->environment_edfs().get_by_name("sky_edf")->get_parameters();
                    if (MString(pa.get_path("radiance")) != "envTex_texInst")
                    {
                        pa.insert("radiance", "envTex_texInst");
                        project->get_scene()->environment_edfs().get_by_name("sky_edf")->bump_version_id();
                    }

                    asr::ParamArray fileparams;
                    fileparams.insert("filename", imageFile.asChar());
                    fileparams.insert("color_space", "linear_rgb");

                    asf::SearchPaths searchPaths;
                    asf::auto_release_ptr<asr::Texture> textureElement(
                        asr::DiskTexture2dFactory().create(
                        "envTex",
                        fileparams,
                        searchPaths));

                    project->get_scene()->textures().insert(textureElement);
                }
            }
        }
    }

    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::setShader(const MUuid& id, const MUuid& shaderId)
{
    Logging::debug("setShader");
    IdNameStruct objElement, shaderElement;
    std::vector<IdNameStruct>::iterator nsIt;
    for (nsIt = objectArray.begin(); nsIt != objectArray.end(); nsIt++)
    {
        Logging::debug(MString("Search for obj id: ") + id + " in " + nsIt->id + " name: " + nsIt->name);
        if (nsIt->id == id)
        {
            objElement = *nsIt;
            break;
        }
    }
    for (nsIt = objectArray.begin(); nsIt != objectArray.end(); nsIt++)
    {
        Logging::debug(MString("Search for shader id: ") + shaderId + " in " + nsIt->id + " name: " + nsIt->name);
        if (nsIt->id == shaderId)
        {
            shaderElement = *nsIt;
            break;
        }
    }
    if ((objElement.mobject == MObject::kNullObj) || (shaderElement.mobject == MObject::kNullObj))
    {
        Logging::error(MString("Unable to find obj or shader for assignment. ShaderName: ") + shaderElement.name + " obj name " + objElement.name);
        return MS::kFailure;
    }
    Logging::debug(MString("--------- Assign shader ") + shaderElement.name + " to object named " + objElement.name);


    asr::ObjectInstance *objInstance = GETASM()->object_instances().get_by_name(objElement.name.asChar());
    if (objInstance != 0)
        objInstance->get_front_material_mappings().insert("slot0", shaderElement.name.asChar());
    else
        Logging::debug(MString("unable to assign shader "));

    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::setResolution(unsigned int w, unsigned int h)
{
    Logging::debug(MString("setResolution to") + w + " " + h);
    width = w;
    height = h;

    // Update resolution buffer
    rb = (float*)realloc(rb, w*h*kNumChannels*sizeof(float));

    for (uint x = 0; x < width; x++)
    {
        for (uint y = 0; y < height; y++)
        {
            uint index = (y * width + x) * kNumChannels;
            rb[index + 0] = x / float(width);
            rb[index + 1] = 0.0f;
            rb[index + 2] = y / float(height);
            rb[index + 3] = 1.0f;
        }
    }

    MString res = MString("") + width + " " + height;
    MString tileString = MString("") + tileSize + " " + tileSize;

    asr::Camera *cam = project->get_scene()->get_camera();
    MString camName = "";
    if (cam != 0)
    {
        camName = project->get_scene()->get_camera()->get_name();
        asr::ParamArray &camParams = cam->get_parameters();
        MString dim = camParams.get_path("film_dimensions");
        MStringArray values;
        dim.split(' ', values);
        float filmWidth = values[0].asFloat();
        float filmHeight = filmWidth * h / (float)w;
        camParams.insert("film_dimensions", (MString("") + filmWidth + " " + filmHeight).asChar());
    }

    project->set_frame(
        asr::FrameFactory::create(
        "beauty",
        asr::ParamArray()
        .insert("camera", camName.asChar())
        .insert("resolution", res.asChar())
        .insert("tile_size", tileString.asChar())
        .insert("pixel_format", "float")
        .insert("color_space", "linear_rgb")));

    project->get_frame()->get_parameters().insert("pixel_format", "float");

    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::endSceneUpdate()
{
    Logging::debug("endSceneUpdate");
    controller.status = asr::IRendererController::ContinueRendering;
    ProgressParams progressParams;
    progressParams.progress = 0.0;
    progress(progressParams);

    if (asyncStarted)
    {
        renderThread = boost::thread(startRenderThread, this);
    }
    else
    {
        ProgressParams progressParams;
        progressParams.progress = 2.0f;
        progress(progressParams);
    }
    return MStatus::kSuccess;
}

MStatus HypershadeRenderer::destroyScene()
{
    Logging::debug("destroyScene");
    controller.status = asr::IRendererController::AbortRendering;
    if (renderThread.joinable())
        renderThread.join();

    ProgressParams progressParams;
    progressParams.progress = -1.0f;
    progress(progressParams);
    return MStatus::kSuccess;
}

bool HypershadeRenderer::isSafeToUnload()
{
    Logging::debug("isSafeToUnload");
    return true;
}

void HypershadeRenderer::copyTileToBuffer(asf::Tile& tile, int tile_x, int tile_y)
{
    size_t tw = tile.get_width();
    size_t th = tile.get_height();

    for (int y = 0; y < th; y++)
    {
        for (int x = 0; x < tw; x++)
        {
            int index = ((height - 1) - (tile_y * tileSize + y)) * width + (tile_x * tileSize) + x;
            index *= kNumChannels;

            rb[index] = tile.get_component<float>(x, y, 0);
            rb[index + 1] = tile.get_component<float>(x, y, 1);
            rb[index + 2] = tile.get_component<float>(x, y, 2);
            rb[index + 3] = tile.get_component<float>(x, y, 3);
        }
    }
    ////The image is R32G32B32A32_Float format
    refreshParams.bottom = 0;
    refreshParams.top = height - 1;
    refreshParams.bytesPerChannel = sizeof(float);
    refreshParams.channels = kNumChannels;
    refreshParams.left = 0;
    refreshParams.right = width - 1;
    refreshParams.width = width;
    refreshParams.height = height;
    refreshParams.data = rb;
    refresh(refreshParams);
}

void HypershadeRenderer::copyFrameToBuffer(float *frame, int w, int h)
{
    if ((w != width) || (h != height))
    {
        Logging::error("wh ungleich.");
        return;
    }

    int numBytes = width * height * kNumChannels * sizeof(float);
    memcpy(rb, frame, numBytes);

    refreshParams.bottom = 0;
    refreshParams.top = height - 1;
    refreshParams.bytesPerChannel = sizeof(float);
    refreshParams.channels = kNumChannels;
    refreshParams.left = 0;
    refreshParams.right = width - 1;
    refreshParams.width = width;
    refreshParams.height = height;
    refreshParams.data = rb;
    refresh(refreshParams);
}

void HypershadeTileCallback::post_render_tile(const asr::Frame* frame, const size_t tile_x, const size_t tile_y)
{
    Logging::debug("HypershadeTileCallback::post_render_tile");
    asf::Tile& tile = frame->image().tile(tile_x, tile_y);
    renderer->copyTileToBuffer(tile, tile_x, tile_y);
}

void HypershadeTileCallback::post_render(const asr::Frame* frame)
{
    Logging::debug("HypershadeTileCallback::post_render frame");

    const asf::Image& img = frame->image();
    const asf::CanvasProperties& frame_props = img.properties();
    const size_t tileSize = frame_props.m_tile_height;
    size_t numPixels = frame_props.m_canvas_width * frame_props.m_canvas_height;
    const size_t width = frame_props.m_canvas_width;
    const size_t height = frame_props.m_canvas_height;

    float* buffer = new float[numPixels * kNumChannels];

    for (size_t tile_y = 0; tile_y < frame_props.m_tile_count_y; tile_y++)
    {
        for (size_t tile_x = 0; tile_x < frame_props.m_tile_count_x; tile_x++)
        {
            const asf::Tile& tile = frame->image().tile(tile_x, tile_y);
            const size_t tw = tile.get_width();
            const size_t th = tile.get_height();

            for (size_t y = 0; y < th; y++)
            {
                for (size_t x = 0; x < tw; x++)
                {
                    const size_t index = (((height - 1) - (tile_y * tileSize + y)) * width + (tile_x * tileSize) + x) * kNumChannels;

                    buffer[index + 0] = tile.get_component<float>(x, y, 0);
                    buffer[index + 1] = tile.get_component<float>(x, y, 1);
                    buffer[index + 2] = tile.get_component<float>(x, y, 2);
                    buffer[index + 3] = tile.get_component<float>(x, y, 3);
                }
            }
        }
    }

    renderer->copyFrameToBuffer(buffer, width, height);

    delete [] buffer;
}

HypershadeTileCallback::HypershadeTileCallback(HypershadeRenderer *mrenderer)
  : renderer(mrenderer)
{
}

HypershadeTileCallback::~HypershadeTileCallback()
{
}

void HypershadeTileCallback::release()
{
}

void HypershadeTileCallback::pre_render(const size_t x, const size_t y, const size_t width, const size_t height)
{
}


HypershadeRenderController::HypershadeRenderController() 
  :status(asr::IRendererController::ContinueRendering)
{
}

HypershadeRenderController::~HypershadeRenderController()
{
}

void HypershadeRenderController::on_rendering_begin()
{
}

void HypershadeRenderController::on_rendering_success()
{
}

void HypershadeRenderController::on_rendering_abort()
{
}

void HypershadeRenderController::on_frame_begin()
{
}

void HypershadeRenderController::on_frame_end()
{
}

void HypershadeRenderController::on_progress()
{
}

void HypershadeRenderController::release()
{
}

asr::IRendererController::Status HypershadeRenderController::get_status() const
{
    return status;
}


HypershadeTileCallbackFactory::HypershadeTileCallbackFactory(HypershadeRenderer *renderer)
{
    tileCallback = new HypershadeTileCallback(renderer);
}

HypershadeTileCallbackFactory::~HypershadeTileCallbackFactory()
{
    delete tileCallback;
}

asr::ITileCallback* HypershadeTileCallbackFactory::create()
{
    return tileCallback;
}

void HypershadeTileCallbackFactory::release()
{ 
    delete this; 
}


#endif
