
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

// Interface header.
#include "appleseedrenderer.h"

// appleseed-maya headers.
#include "osl/oslutils.h"
#include "shadingtools/material.h"
#include "shadingtools/shaderdefinitions.h"
#include "shadingtools/shadingutils.h"
#include "threads/renderqueueworker.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "utilities/pystring.h"
#include "utilities/tools.h"
#include "appleseedutils.h"
#include "mayascene.h"
#include "mtap_tilecallback.h"
#include "renderglobals.h"
#include "mayatoworld.h"

// appleseed.renderer headers.
#include "renderer/global/globallogger.h"
#include "renderer/api/environment.h"
#include "renderer/api/environmentedf.h"
#include "renderer/api/texture.h"
#include "renderer/api/environmentshader.h"
#include "renderer/api/edf.h"
#include "renderer/api/shadergroup.h"
#include "renderer/modeling/environmentedf/sphericalcoordinates.h"

// appleseed.foundation headers.
#include "foundation/platform/thread.h"

// Maya headers.
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPointArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnCamera.h>
#include <maya/MGlobal.h>
#include <maya/MRenderView.h>
#include <maya/MFileIO.h>
#include <maya/MItDag.h>
#include <maya/MFnTransform.h>

AppleseedRenderer::AppleseedRenderer()
  : sceneBuilt(false)
{
    asr::global_logger().set_format(asf::LogMessage::Debug, "");
    log_target.reset(asf::create_console_log_target(stdout));
    asr::global_logger().add_target(log_target.get());
}

AppleseedRenderer::~AppleseedRenderer()
{
    if (log_target.get() != 0)
        asr::global_logger().remove_target(log_target.get());
}

void AppleseedRenderer::initializeRenderer()
{
    this->project = asr::ProjectFactory::create("mtap_project");

    std::string oslShaderPath = (getRendererHome() + "shaders").asChar();
    Logging::debug(MString("setting osl shader search path to: ") + oslShaderPath.c_str());
    project->search_paths().push_back(oslShaderPath.c_str());
    for (uint i = 0; i < getWorldPtr()->shaderSearchPath.length(); i++)
    {
        Logging::debug(MString("Search path: ") + getWorldPtr()->shaderSearchPath[i]);
        project->search_paths().push_back(getWorldPtr()->shaderSearchPath[i].asChar());
    }
    defineConfig();
    defineScene(this->project.get());
}

void AppleseedRenderer::unInitializeRenderer()
{
    getWorldPtr()->setRenderState(MayaToWorld::RSTATEDONE);
    getWorldPtr()->setRenderType(MayaToWorld::RTYPENONE);

    // todo: isn't this supposed to be reset()?
    this->project.release();
}

void AppleseedRenderer::defineProject()
{
    defineCamera(); // first camera
    defineOutput(); // output accesses camera so define it after camera
    defineMasterAssembly(this->project.get());
    defineDefaultMaterial(this->project.get());
    defineEnvironment();
    defineGeometry();
    defineLights();
}

void AppleseedRenderer::preFrame()
{
    defineProject();
}

void AppleseedRenderer::postFrame()
{
    // Save the frame to disk.
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;
    renderGlobals->getImageName();
    MString filename = renderGlobals->imageOutputFile.asChar();
    Logging::debug(MString("Saving image as ") + renderGlobals->imageOutputFile);
    project->get_frame()->write_main_image(renderGlobals->imageOutputFile.asChar());

    // if we render the very last frame or if we are in UI where the last frame == first frame, then delete the master renderer before
    // the deletion of the assembly because otherwise it will be deleted automatically if the renderer instance is deleted what results in a crash
    // because the masterRenderer still have references to the shading groups which are defined in the world assembly. If the masterRenderer is deleted
    // AFTER the assembly it tries to access non existent shadingGroups.
    if (renderGlobals->currentFrameNumber == renderGlobals->frameList.back())
        masterRenderer.reset();

    asf::UniqueID aiuid = project->get_scene()->assembly_instances().get_by_name("world_Inst")->get_uid();
    asf::UniqueID auid = project->get_scene()->assemblies().get_by_name("world")->get_uid();
    project->get_scene()->assembly_instances().remove(aiuid);
    project->get_scene()->assemblies().remove(auid);
    asr::Assembly *worldass = project->get_scene()->assemblies().get_by_name("world");
}

void AppleseedRenderer::render()
{
    Logging::debug("AppleseedRenderer::render");

    if (!sceneBuilt)
    {
        this->tileCallbackFac.reset(new mtap_TileCallbackFactory());

        if (getWorldPtr()->getRenderType() == MayaToWorld::IPRRENDER)
        {
            masterRenderer.reset(
                new asr::MasterRenderer(
                    this->project.ref(),
                    this->project->configurations().get_by_name("interactive")->get_inherited_parameters(),
                    &mtap_controller,
                    this->tileCallbackFac.get()));
        }
        else
        {
            masterRenderer.reset(
                new asr::MasterRenderer(
                    this->project.ref(),
                    this->project->configurations().get_by_name("final")->get_inherited_parameters(),
                    &mtap_controller,
                    this->tileCallbackFac.get()));
        }

        if (getWorldPtr()->getRenderType() == MayaToWorld::IPRRENDER)
        {
            Event e;
            e.type = Event::ADDIPRCALLBACKS;
            theRenderEventQueue()->push(e);

            while (!RenderQueueWorker::iprCallbacksDone())
                asf::sleep(10);
        }

        sceneBuilt = true;
    }

    getWorldPtr()->setRenderState(MayaToWorld::RSTATERENDERING);
    mtap_controller.set_status(asr::IRendererController::ContinueRendering);

    masterRenderer->render();
}

void AppleseedRenderer::abortRendering()
{
    mtap_controller.set_status(asr::IRendererController::AbortRendering);
}

void AppleseedRenderer::addRenderParams(asr::ParamArray& paramArray)
{
    static const char* lightingEngines[] = { "pt", "drt", "sppm" };
    static const char* bucketOrders[] = { "linear", "spiral", "hilbert", "random" };
    static const char* photonTypes[] = { "mono", "poly" };
    static const char* dlTypes[] = { "rt", "sppm", "off" };

    MFnDependencyNode renderGlobalsFn(getRenderGlobalsNode());
    MString lightingEngine = lightingEngines[getEnumInt("lightingEngine", renderGlobalsFn)];
    MString bucketOrder = bucketOrders[getEnumInt("tile_ordering", renderGlobalsFn)];
    MString photonType = photonTypes[getEnumInt("photon_type", renderGlobalsFn)];
    MString dlType = dlTypes[getEnumInt("dl_mode", renderGlobalsFn)];
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;

    paramArray.insert_path("texture_store.max_size", getIntAttr("texCacheSize", renderGlobalsFn, 128) * 1024 * 1024); // at least 128 MB

    paramArray.insert("sampling_mode", getEnumString("sampling_mode", renderGlobalsFn));
    paramArray.insert("pixel_renderer", "uniform");
    paramArray.insert_path("uniform_pixel_renderer.decorrelate_pixels", true);
    paramArray.insert_path("uniform_pixel_renderer.force_antialiasing", false);
    paramArray.insert_path("uniform_pixel_renderer.samples", getIntAttr("maxSamples", renderGlobalsFn, 16));

    paramArray.insert("pixel_renderer", "adaptive");
    paramArray.insert_path("adaptive_pixel_renderer.enable_diagnostics", getBoolAttr("enable_diagnostics", renderGlobalsFn, true));
    paramArray.insert_path("adaptive_pixel_renderer.min_samples", getIntAttr("minSamples", renderGlobalsFn, 1));
    paramArray.insert_path("adaptive_pixel_renderer.max_samples", getIntAttr("maxSamples", renderGlobalsFn, 16));
    paramArray.insert_path("adaptive_pixel_renderer.quality", getFloatAttr("adaptiveQuality", renderGlobalsFn, 3.0f));

    paramArray.insert_path("generic_frame_renderer.passes", getIntAttr("frameRendererPasses", renderGlobalsFn, 1));
    paramArray.insert_path("generic_frame_renderer.tile_ordering", bucketOrder.asChar());

    paramArray.insert("lighting_engine", lightingEngine.asChar());
    paramArray.insert_path((lightingEngine + ".enable_ibl").asChar(), getBoolAttr("enable_ibl", renderGlobalsFn, true));
    paramArray.insert_path((lightingEngine + ".enable_dl").asChar(), getBoolAttr("enable_dl", renderGlobalsFn, true));
    paramArray.insert_path((lightingEngine + ".dl_light_samples").asChar(), getIntAttr("directLightSamples", renderGlobalsFn, 0));
    paramArray.insert_path((lightingEngine + ".ibl_env_samples").asChar(), getIntAttr("environmentSamples", renderGlobalsFn, 0));
    paramArray.insert_path((lightingEngine + ".next_event_estimation").asChar(), getBoolAttr("next_event_estimation", renderGlobalsFn, true));
    paramArray.insert_path((lightingEngine + ".enable_caustics").asChar(), getBoolAttr("enable_caustics", renderGlobalsFn, true));
    paramArray.insert_path((lightingEngine + ".alpha").asChar(), getFloatAttr("sppmAlpha", renderGlobalsFn, .8f));
    paramArray.insert_path((lightingEngine + ".dl_type").asChar(), dlType.asChar());
    paramArray.insert_path((lightingEngine + ".env_photons_per_pass").asChar(), getIntAttr("env_photons_per_pass", renderGlobalsFn, 100000));
    paramArray.insert_path((lightingEngine + ".initial_radius").asChar(), getFloatAttr("initial_radius", renderGlobalsFn, .5f));
    paramArray.insert_path((lightingEngine + ".light_photons_per_pass").asChar(), getIntAttr("light_photons_per_pass", renderGlobalsFn, 100000));
    paramArray.insert_path((lightingEngine + ".max_photons_per_estimate").asChar(), getIntAttr("max_photons_per_estimate", renderGlobalsFn, 100));
    paramArray.insert_path((lightingEngine + ".photons_per_pass").asChar(), getIntAttr("photons_per_pass", renderGlobalsFn, 100000));

    if (getFloatAttr("max_ray_intensity", renderGlobalsFn, .5f) > 0.0)
        paramArray.insert_path((lightingEngine + ".max_ray_intensity").asChar(), getFloatAttr("max_ray_intensity", renderGlobalsFn, .5f));
    paramArray.insert_path((lightingEngine + ".photon_type").asChar(), photonType.asChar());
    paramArray.insert_path((lightingEngine + ".max_path_length").asChar(), getFloatAttr("max_path_length", renderGlobalsFn, 8.0f));
    paramArray.insert_path((lightingEngine + ".rr_min_path_length").asChar(), getFloatAttr("rr_min_path_length", renderGlobalsFn, 3.0f));
    paramArray.insert_path((lightingEngine + ".path_tracing_max_path_length").asChar(), getFloatAttr("path_tracing_max_path_length", renderGlobalsFn, 8.0f));
    paramArray.insert_path((lightingEngine + ".path_tracing_rr_min_path_length").asChar(), getFloatAttr("path_tracing_rr_min_path_length", renderGlobalsFn, 3.0f));
    paramArray.insert_path((lightingEngine + ".photon_tracing_max_path_length").asChar(), getFloatAttr("photon_tracing_max_path_length", renderGlobalsFn, 8.0f));
    paramArray.insert_path((lightingEngine + ".photon_tracing_rr_min_path_length").asChar(), getFloatAttr("photon_tracing_rr_min_path_length", renderGlobalsFn, 3.0f));
}

void AppleseedRenderer::defineConfig()
{
    Logging::debug("AppleseedRenderer::defineConfig");
    MFnDependencyNode renderGlobalsFn(getRenderGlobalsNode());
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;

    project->add_default_configurations();
    addRenderParams(this->project->configurations().get_by_name("final")->get_parameters());
    addRenderParams(this->project->configurations().get_by_name("interactive")->get_parameters());

    int renderer = getEnumInt("pixel_renderer", renderGlobalsFn);
    const char *pixel_renderers[2] = { "adaptive", "uniform" };
    const char *pixel_renderer = pixel_renderers[renderer];

    this->project->configurations()
        .get_by_name("final")->get_parameters()
        .insert_path("pixel_renderer", pixel_renderer);

    this->project->configurations()
        .get_by_name("final")->get_parameters()
        .insert_path("generic_tile_renderer.sampler", pixel_renderer);
    this->project->configurations()
        .get_by_name("interactive")->get_parameters()
        .insert_path("generic_tile_renderer.sampler", pixel_renderer);

    if (renderGlobals->getUseRenderRegion())
    {
        int left, right, bottom, top;
        int imgWidth, imgHeight;
        renderGlobals->getWidthHeight(imgWidth, imgHeight);
        renderGlobals->getRenderRegion(left, bottom, right, top);
        int ybot = (imgHeight - bottom);
        int ytop = (imgHeight - top);
        int ymin = ybot <  ytop ? ybot :  ytop;
        int ymax = ybot >  ytop ? ybot :  ytop;
        MString regionString = MString("") + left + " " + ymin + " " + right + " " + ymax;
        Logging::debug("Render region is turned on rendering: " + regionString);
        this->project->configurations()
        .get_by_name("final")->get_parameters()
        .insert_path("generic_tile_renderer.crop_window", regionString.asChar());
    }

    this->project->configurations()
    .get_by_name("interactive")->get_parameters()
    .insert_path("generic_tile_renderer.filter", renderGlobals->filterTypeString.toLowerCase().asChar())
    .insert_path("generic_tile_renderer.filter_size", renderGlobals->filterSize);

#ifdef _DEBUG
    project->configurations().get_by_name("final")->get_parameters().insert_path("uniform_pixel_renderer.samples", "4");
#endif
    asr::Configuration *cfg = project->configurations().get_by_name("interactive");
    asr::ParamArray &params = cfg->get_parameters();
    params.insert_path("generic_tile_renderer.filter", renderGlobals->filterTypeString.toLowerCase().asChar());
    params.insert_path("generic_tile_renderer.filter_size", renderGlobals->filterSize);
    params.insert("sample_renderer", "generic");
    params.insert("sample_generator", "generic");
    params.insert("tile_renderer", "generic");
    params.insert("frame_renderer", "progressive");
    params.insert("lighting_engine", "pt");
    params.insert_path("progressive_frame_renderer.max_fps", "4");
    params.insert_path("progressive_frame_renderer.max_samples", "12000000");
}

void AppleseedRenderer::defineCamera(boost::shared_ptr<MayaObject> cam)
{
    // Cameras are a mixture between assembly and object. They have a transformation like an assembly but
    // attributes like an object.

    MStatus stat;
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;

    asr::Camera *camera = project->get_scene()->get_camera();
    if (camera != 0)
        Logging::debug("Camera is not null - we already have a camera -> update it.");

    // update the complete camera and place it into the scene
    Logging::debug(MString("Creating camera shape: ") + cam->shortName);
    float horizontalFilmAperture = 24.892f;
    float verticalFilmAperture = 18.669f;
    int width, height;
    renderGlobals->getWidthHeight(width, height);
    float imageAspect = (float)width / (float)height;
    bool dof = renderGlobals->doDof;
    float mtap_cameraType = 0;
    int mtap_diaphragm_blades = 0;
    float mtap_diaphragm_tilt_angle = 0.0;
    float focusDistance = 0.0;
    float fStop = 0.0;

    float focalLength = 35.0f;
    MFnCamera camFn(cam->mobject, &stat);
    asr::ParamArray camParams;

    getFloat(MString("horizontalFilmAperture"), camFn, horizontalFilmAperture);
    getFloat(MString("verticalFilmAperture"), camFn, verticalFilmAperture);
    getFloat(MString("focalLength"), camFn, focalLength);
    getBool(MString("depthOfField"), camFn, dof);
    getFloat(MString("focusDistance"), camFn, focusDistance);
    getFloat(MString("fStop"), camFn, fStop);
    getInt(MString("mtap_diaphragm_blades"), camFn, mtap_diaphragm_blades);
    getFloat(MString("mtap_diaphragm_tilt_angle"), camFn, mtap_diaphragm_tilt_angle);

    // this is a hack because this camera model does not support NON depth of field
    if (!dof)
        fStop *= 10000.0f;

    focusDistance *= renderGlobals->scaleFactor;

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
    camParams.insert("diaphragm_blades", (MString("") + mtap_diaphragm_blades).asChar());
    camParams.insert("diaphragm_tilt_angle", (MString("") + mtap_diaphragm_tilt_angle).asChar());

    if (!camera)
    {
        asf::auto_release_ptr<asr::Camera> appleCam = asr::ThinLensCameraFactory().create(
            cam->shortName.asChar(),
            camParams);
        project->get_scene()->set_camera(appleCam);
        camera = project->get_scene()->get_camera();
    }

    fillMatrices(cam, camera->transform_sequence());
}

void AppleseedRenderer::defineCamera()
{
    MStatus stat;
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;

    std::vector<boost::shared_ptr<MayaObject> >::iterator oIt;
    for (oIt = mayaScene->camList.begin(); oIt != mayaScene->camList.end(); oIt++)
    {
        boost::shared_ptr<MayaObject> cam = *oIt;
        if (!isCameraRenderable(cam->mobject) && (!(cam->dagPath == mayaScene->uiCamera)))
        {
            continue;
        }

        defineCamera(cam);

        // appleseed does not support more than one camera at the moment, so break after the first one.
        break;
    }
}

void AppleseedRenderer::defineOutput()
{
    asr::Frame *frame = project->get_frame();
    if (frame == 0)
    {
        MFnDependencyNode depFn(getRenderGlobalsNode());
        boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;
        Logging::debug("AppleseedRenderer::defineOutput");
        int width, height;
        renderGlobals->getWidthHeight(width, height);
        MString res = MString("") + width + " " + height;
        MString colorSpaceString = getEnumString("colorSpace", depFn);
        MString tileSize =  MString("") + renderGlobals->tilesize + " " + renderGlobals->tilesize;
        project->set_frame(
            asr::FrameFactory::create(
            "beauty",
            asr::ParamArray()
            .insert("camera", project->get_scene()->get_camera()->get_name())
            .insert("resolution", res.asChar())
            .insert("tile_size", tileSize.asChar())
            .insert("color_space", colorSpaceString.asChar())));
    }
}

void AppleseedRenderer::defineEnvironment()
{
    asr::Scene *scene = project->get_scene();

    MFnDependencyNode appleseedGlobals(getRenderGlobalsNode());
    MString textInstName = "bg_texture_inst";
    MString envColorName = "environmentColor";
    MString envMapName = "environmentMap";
    MString gradHorizName = "gradientHorizon";
    MString gradZenitName = "gradientZenit";
    MStatus stat;
    float latlongHoShift = getFloatAttr("latlongHoShift", appleseedGlobals, 0.0f);
    float latlongVeShift = getFloatAttr("latlongVeShift", appleseedGlobals, 0.0f);
    int skyModel = getEnumInt("skyModel", appleseedGlobals);
    int environmentType = getEnumInt("environmentType", appleseedGlobals);
    float ground_albedo = getFloatAttr("ground_albedo", appleseedGlobals, 0.0f);
    float horizon_shift = getFloatAttr("horizon_shift", appleseedGlobals, 0.0f);
    float luminance_multiplier = getFloatAttr("luminance_multiplier", appleseedGlobals, 1.0f);
    float saturation_multiplier = getFloatAttr("saturation_multiplier", appleseedGlobals, 1.0f);
    float turbidity = getFloatAttr("turbidity", appleseedGlobals, 2.0f);
    float turbidity_max = getFloatAttr("turbidity_max", appleseedGlobals, 3.0f);
    float turbidity_min = getFloatAttr("turbidity_min", appleseedGlobals, 3.0f);
    int environmentSamples = getIntAttr("environmentSamples", appleseedGlobals, 1);
    bool physicalSun = getBoolAttr("physicalSun", appleseedGlobals, false);
    float sunTurbidity = getFloatAttr("sunTurbidity", appleseedGlobals, 3.0f);
    float sunExitanceMultiplier = getFloatAttr("sunExitanceMultiplier", appleseedGlobals, 1.0f);
    MColor environmentColorColor = getColorAttr("environmentColor", appleseedGlobals);
    MColor gradientHorizonColor = getColorAttr("gradientHorizon", appleseedGlobals);
    MColor gradientZenitColor = getColorAttr("gradientZenit", appleseedGlobals);
    float environmentIntensity = getFloatAttr("environmentIntensity", appleseedGlobals, 1.0f);

    defineColor(project.get(), envColorName.asChar(), environmentColorColor, environmentIntensity);
    defineColor(project.get(), gradHorizName.asChar(), gradientHorizonColor, environmentIntensity);
    defineColor(project.get(), gradZenitName.asChar(), gradientZenitColor, environmentIntensity);

    MString envMapAttrName = colorOrMap(project.get(), appleseedGlobals, envMapName);

    asf::auto_release_ptr<asr::EnvironmentEDF> environmentEDF;

    bool updateSkyShader = (scene->environment_shaders().get_by_name("sky_shader") != 0);
    bool updateSkyEdf = (scene->environment_edfs().get_by_name("sky_edf") != 0);

    switch (environmentType)
    {
    case 0: //constant
    {
        environmentEDF = asr::ConstantEnvironmentEDFFactory().create(
                "sky_edf",
                asr::ParamArray()
                .insert("radiance", envColorName));
        break;
    }
    case 1: //ConstantHemisphere
    {
        environmentEDF = asr::ConstantHemisphereEnvironmentEDFFactory().create(
            "sky_edf",
            asr::ParamArray()
            .insert("radiance", envColorName)
            .insert("upper_hemi_radiance", gradHorizName)
            .insert("lower_hemi_radiance", gradHorizName));
        break;
    }
    case 2: //Gradient
    {
        environmentEDF = asr::GradientEnvironmentEDFFactory().create(
                "sky_edf",
                asr::ParamArray()
                .insert("horizon_radiance", gradHorizName)
                .insert("zenith_radiance", gradZenitName)
                );
        break;
    }
    case 3: //Latitude Longitude
    {
        environmentEDF = asr::LatLongMapEnvironmentEDFFactory().create(
                "sky_edf",
                asr::ParamArray()
                .insert("radiance", envMapAttrName.asChar())
                .insert("radiance_multiplier", environmentIntensity)
                .insert("horizontal_shift", latlongHoShift)
                .insert("vertical_shift", latlongVeShift)
                );
        break;
    }
    case 4: //Mirror Ball
    {
        environmentEDF = asr::MirrorBallMapEnvironmentEDFFactory().create(
                "sky_edf",
                asr::ParamArray()
                .insert("radiance", envMapAttrName.asChar())
                .insert("radiance_multiplier", environmentIntensity));
        break;
    }
    case 5: //Physical Sky
    {
        asf::Vector3d unitVector(0.0, 0.0, 0.0);
        float sun_theta = getFloatAttr("sun_theta", appleseedGlobals, 1.0f);
        float sun_phi = getFloatAttr("sun_phi", appleseedGlobals, 1.0f);
        bool usePhysicalSun = getBoolAttr("physicalSun", appleseedGlobals, true);
        double theta = 90.0f - sun_theta, phi = sun_phi;

        if (usePhysicalSun)
        {
            // get the connected sun light
            // physicalSunConnection
            MObject connectedNode = getConnectedInNode(getRenderGlobalsNode(), "physicalSunConnection");
            if (connectedNode != MObject::kNullObj)
            {
                MFnTransform tn(connectedNode);
                MMatrix tm = tn.transformationMatrix(&stat);
                if (stat)
                {
                    MVector sunOrient(0,0,1);
                    sunOrient *= tm;
                    sunOrient.normalize();
                    unitVector.x = sunOrient.x;
                    unitVector.y = sunOrient.y;
                    unitVector.z = sunOrient.z;
                    asr::unit_vector_to_angles(unitVector, theta, phi);
                    theta = 90.0f - asf::rad_to_deg(theta);
                    theta = theta;
                    sun_phi = asf::rad_to_deg(phi);
                }
            }
            else
            {
                Logging::warning("physicalSunConnection plug has no connection, but use physical sun is turned on. Please correct.");
            }
        }

        if (skyModel == 0) // preetham
        {
            environmentEDF = asr::PreethamEnvironmentEDFFactory().create(
                    "sky_edf",
                    asr::ParamArray()
                    .insert("horizon_shift", horizon_shift)
                    .insert("luminance_multiplier", luminance_multiplier)
                    .insert("saturation_multiplier", saturation_multiplier)
                    .insert("sun_phi", sun_phi)
                    .insert("sun_theta", 90.0f - sun_theta)
                    .insert("turbidity", turbidity)
                    .insert("turbidity_max", turbidity_max)
                    .insert("turbidity_min", turbidity_min));
        }
        else
        { // hosek
            environmentEDF = asr::HosekEnvironmentEDFFactory().create(
                    "sky_edf",
                    asr::ParamArray()
                    .insert("ground_albedo", ground_albedo)
                    .insert("horizon_shift", horizon_shift)
                    .insert("luminance_multiplier", luminance_multiplier)
                    .insert("saturation_multiplier", saturation_multiplier)
                    .insert("sun_phi", sun_phi)
                    .insert("sun_theta", 90.0f - sun_theta)
                    .insert("turbidity", turbidity)
                    .insert("turbidity_max", turbidity_max)
                    .insert("turbidity_min", turbidity_min));
        }

        break;
    }
    case 6: //OSL Sky
    {
        asf::auto_release_ptr<asr::ShaderGroup> oslShaderGroup = asr::ShaderGroupFactory().create("OSL_Sky");
        oslShaderGroup->add_shader("surface", "testBG", "BGLayer", asr::ParamArray());
        scene->shader_groups().insert(oslShaderGroup);
        environmentEDF = asr::OSLEnvironmentEDFFactory().create("sky_edf", asr::ParamArray()
            .insert("osl_background", "OSL_Sky")
            .insert("radiance_multiplier", environmentIntensity));

        break;
    }
    };

    asr::EnvironmentEDF *sky = scene->environment_edfs().get_by_name("sky_edf");
    if (sky != 0)
        scene->environment_edfs().remove(sky);

    scene->environment_edfs().insert(environmentEDF);

    // Create an environment shader called "sky_shader" and insert it into the scene.
    if (scene->environment_shaders().get_by_name("sky_shader") != 0)
    {
        asr::EnvironmentShader *skyShader = scene->environment_shaders().get_by_name("sky_shader");
        scene->environment_shaders().remove(skyShader);
    }
    scene->environment_shaders().insert(
        asr::EDFEnvironmentShaderFactory().create(
            "sky_shader",
            asr::ParamArray()
                .insert("environment_edf", "sky_edf")));

    // Create an environment called "sky" and bind it to the scene.
    scene->set_environment(
        asr::EnvironmentFactory::create(
            "sky",
            asr::ParamArray()
                .insert("environment_edf", "sky_edf")
                .insert("environment_shader", "sky_shader")));
}

namespace
{
    asr::GVector3 MPointToAppleseed(const MPoint& p)
    {
        return
            asr::GVector3(
                static_cast<float>(p.x),
                static_cast<float>(p.y),
                static_cast<float>(p.z));
    }
}

asf::auto_release_ptr<asr::MeshObject> AppleseedRenderer::defineStandardPlane(bool area)
{
    asf::auto_release_ptr<asr::MeshObject> object(asr::MeshObjectFactory::create("stdPlane", asr::ParamArray()));

    if (area)
    {
        // Vertices.
        object->push_vertex(asr::GVector3(-1.0f, -1.0f, 0.0f));
        object->push_vertex(asr::GVector3(-1.0f, 1.0f, 0.0f));
        object->push_vertex(asr::GVector3(1.0f, 1.0f, 0.0f));
        object->push_vertex(asr::GVector3(1.0f, -1.0f, 0.0f));
    }
    else
    {
        // Vertices.
        object->push_vertex(asr::GVector3(-1.0f, 0.0f, -1.0f));
        object->push_vertex(asr::GVector3(-1.0f, 0.0f, 1.0f));
        object->push_vertex(asr::GVector3(1.0f, 0.0f, 1.0f));
        object->push_vertex(asr::GVector3(1.0f, 0.0f, -1.0f));
    }
    // Vertex normals.
    if (area)
    {
        object->push_vertex_normal(asr::GVector3(0.0f, 0.0f, -1.0f));
    }
    else
    {
        object->push_vertex_normal(asr::GVector3(0.0f, 1.0f, 0.0f));
    }
    object->push_tex_coords(asr::GVector2(0.0, 0.0));
    object->push_tex_coords(asr::GVector2(1.0, 0.0));
    object->push_tex_coords(asr::GVector2(1.0, 1.0));
    object->push_tex_coords(asr::GVector2(0.0, 1.0));

    // Triangles.
    object->push_triangle(asr::Triangle(0, 1, 2, 0, 0, 0, 0, 1, 2, 0));
    object->push_triangle(asr::Triangle(0, 2, 3, 0, 0, 0, 0, 2, 3, 0));

    return object;
}

void AppleseedRenderer::createMesh(boost::shared_ptr<MayaObject> obj)
{
    // If the mesh has an attribute called "mtap_standin_path" and it contains a valid entry, then try to read the
    // content from the path.
    // The other way is to have a standInMeshNode which is connected to the inMesh of the mesh node.
    // In this case, get the standin node, read the path of the binmesh file and load it.

    MStatus stat = MStatus::kSuccess;
    MFnMesh meshFn(obj->mobject, &stat);
    CHECK_MSTATUS(stat);

    MPointArray points;
    MFloatVectorArray normals;
    MFloatArray uArray, vArray;
    MIntArray triPointIds, triNormalIds, triUvIds, triMatIds;
    Logging::debug("defineMesh pre getMeshData");
    obj->getMeshData(points, normals, uArray, vArray, triPointIds, triNormalIds, triUvIds, triMatIds);

    Logging::debug(MString("Translating mesh object ") + meshFn.name().asChar());
    MString meshFullName = makeGoodString(meshFn.fullPathName());

    // Create a new mesh object.
    asf::auto_release_ptr<asr::MeshObject> mesh = asr::MeshObjectFactory::create(meshFullName.asChar(), asr::ParamArray());

    Logging::debug(MString("//object ") + meshFn.name());
    for (uint vtxId = 0; vtxId < points.length(); vtxId++)
    {
        mesh->push_vertex(MPointToAppleseed(points[vtxId]));
    }

    for (uint nId = 0; nId < normals.length(); nId++)
    {
        MVector n = normals[nId];
        MStatus s = n.normalize();
        if (n.length() < .3)
            n.y = .1;
        n.normalize();
        mesh->push_vertex_normal(MPointToAppleseed(n));
    }

    for (uint tId = 0; tId < uArray.length(); tId++)
    {
        mesh->push_tex_coords(asr::GVector2((float)uArray[tId], (float)vArray[tId]));
    }

    mesh->reserve_material_slots(obj->shadingGroups.length());
    for (uint sgId = 0; sgId < obj->shadingGroups.length(); sgId++)
    {
        MString slotName = MString("slot") + sgId;
        mesh->push_material_slot(slotName.asChar());
    }

    const uint numTris = triPointIds.length() / 3;

    for (uint triId = 0; triId < numTris; triId++)
    {
        const uint index = triId * 3;
        const int perFaceShadingGroup = triMatIds[triId];
        const int vtxId0 = triPointIds[index];
        const int vtxId1 = triPointIds[index + 1];
        const int vtxId2 = triPointIds[index + 2];
        const int normalId0 = triNormalIds[index];
        const int normalId1 = triNormalIds[index + 1];
        const int normalId2 = triNormalIds[index + 2];
        const int uvId0 = triUvIds[index];
        const int uvId1 = triUvIds[index + 1];
        const int uvId2 = triUvIds[index + 2];
        mesh->push_triangle(asr::Triangle(vtxId0, vtxId1, vtxId2,  normalId0, normalId1, normalId2, uvId0, uvId1, uvId2, perFaceShadingGroup));
    }

    MayaObject *assemblyObject = getAssemblyMayaObject(obj.get());
    asr::Assembly *ass = getCreateObjectAssembly(obj);

    Logging::debug(MString("Placing mesh ") + mesh->get_name() + " into assembly " + ass->get_name());

    asr::MeshObject *meshPtr = (asr::MeshObject *)ass->objects().get_by_name(meshFullName.asChar());

    if (meshPtr != 0)
    {
        Logging::debug(MString("Mesh object ") + meshFullName + " is already defined, removing...");
        ass->objects().remove(meshPtr);
        ass->bump_version_id();
    }

    ass->objects().insert(asf::auto_release_ptr<asr::Object>(mesh));
    meshPtr = (asr::MeshObject *)ass->objects().get_by_name(meshFullName.asChar());

    MString objectInstanceName = getObjectInstanceName(obj.get());

    MMatrix assemblyObjectMatrix = assemblyObject->dagPath.inclusiveMatrix();
    MMatrix objectMatrix = obj->dagPath.inclusiveMatrix();
    MMatrix diffMatrix = objectMatrix * assemblyObjectMatrix.inverse();
    asf::Matrix4d appleMatrix;
    MMatrixToAMatrix(diffMatrix, appleMatrix);

    asr::ParamArray objInstanceParamArray;
    addVisibilityFlags(obj, objInstanceParamArray);

    asr::ObjectInstance *oinst = ass->object_instances().get_by_name(objectInstanceName.asChar());
    if (oinst != 0)
    {
        Logging::debug(MString("Mesh object instance ") + objectInstanceName + " is already defined, removing...");
        ass->object_instances().remove(oinst);
        ass->bump_version_id();
    }
    ass->object_instances().insert(
        asr::ObjectInstanceFactory::create(
        objectInstanceName.asChar(),
        objInstanceParamArray,
        meshPtr->get_name(),
        asf::Transformd::from_local_to_parent(appleMatrix),
        asf::StringDictionary()
        .insert("slot0", "default")));
}

void AppleseedRenderer::updateGeometry(boost::shared_ptr<MayaObject> mobj)
{
    if (!mobj->isObjVisible())
        return;

    if (!mobj->mobject.hasFn(MFn::kMesh))
        return;

    if (mobj->instanceNumber == 0)
    {
        createMesh(mobj);
        defineMaterial(mobj);
    }
}

void AppleseedRenderer::updateInstance(boost::shared_ptr<MayaObject> mobj)
{
    if (mobj->dagPath.node().hasFn(MFn::kWorld))
        return;

    if (!mobj->isObjVisible())
        return;

    if (mobj->instanceNumber > 0)
    {
        MayaObject *assemblyObject = getAssemblyMayaObject(mobj.get());
        if (assemblyObject == 0)
        {
            Logging::debug("create mesh assemblyPtr == null");
            return;
        }
        MString assemblyName = getAssemblyName(assemblyObject);
        MString assemblyInstanceName = getAssemblyInstanceName(mobj.get());

        asf::auto_release_ptr<asr::AssemblyInstance> assemblyInstance(
            asr::AssemblyInstanceFactory::create(
            assemblyInstanceName.asChar(),
            asr::ParamArray(),
            assemblyName.asChar()));
        asr::TransformSequence &ts = assemblyInstance->transform_sequence();
        fillMatrices(mobj, ts);
        getMasterAssemblyFromProject(this->project.get())->assembly_instances().insert(assemblyInstance);
    }
}

void AppleseedRenderer::defineGeometry()
{
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;
    std::vector<boost::shared_ptr<MayaObject> >::iterator oIt;
    for (oIt = mayaScene->objectList.begin(); oIt != mayaScene->objectList.end(); oIt++)
    {
        boost::shared_ptr<MayaObject> mobj = *oIt;
        updateGeometry(mobj);
    }

    // create assembly instances
    for (oIt = mayaScene->objectList.begin(); oIt != mayaScene->objectList.end(); oIt++)
    {
        boost::shared_ptr<MayaObject> mobj = *oIt;
        updateInstance(mobj);
    }

    for (oIt = mayaScene->instancerNodeElements.begin(); oIt != mayaScene->instancerNodeElements.end(); oIt++)
    {
        boost::shared_ptr<MayaObject> mobj = *oIt;

        if (mobj->dagPath.node().hasFn(MFn::kWorld))
            continue;
        if (mobj->instanceNumber == 0)
            continue;

        MayaObject* assemblyObject = getAssemblyMayaObject(mobj.get());
        if (assemblyObject == 0)
        {
            Logging::debug("create mesh assemblyPtr == null");
            continue;
        }

        MString assemblyName = getAssemblyName(assemblyObject);
        MString assemblyInstanceName = getAssemblyInstanceName(mobj.get());

        asf::auto_release_ptr<asr::AssemblyInstance> assemblyInstance(
            asr::AssemblyInstanceFactory::create(
            assemblyInstanceName.asChar(),
            asr::ParamArray(),
            assemblyName.asChar()));
        asr::TransformSequence &ts = assemblyInstance->transform_sequence();
        fillMatrices(mobj, ts);
        getMasterAssemblyFromProject(this->project.get())->assembly_instances().insert(assemblyInstance);
    }
}

void AppleseedRenderer::doInteractiveUpdate()
{
    Logging::debug("AppleseedRenderer::doInteractiveUpdate");
    if (interactiveUpdateList.empty())
        return;
    std::vector<InteractiveElement *>::iterator iaIt;
    for (iaIt = interactiveUpdateList.begin(); iaIt != interactiveUpdateList.end(); iaIt++)
    {
        InteractiveElement *iElement = *iaIt;
        if (iElement->node.hasFn(MFn::kShadingEngine))
        {
            if (iElement->obj)
            {
                Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate - found shadingEngine.") + iElement->name);
                updateMaterial(iElement->node);
            }
        }
        if (iElement->node.hasFn(MFn::kCamera))
        {
            Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate - found camera.") + iElement->name);
            if (iElement->obj)
                defineCamera(iElement->obj);
        }
        if (iElement->node.hasFn(MFn::kLight))
        {
            Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate - found light.") + iElement->name);
            if (iElement->obj)
            {
                defineLight(iElement->obj);
            }
        }
        // shading nodes
        if (iElement->node.hasFn(MFn::kPluginDependNode) || iElement->node.hasFn(MFn::kLambert))
        {
            MFnDependencyNode depFn(iElement->node);
            std::vector<MString> shaderNames;
            shaderNames.push_back("asLayeredShader");
            shaderNames.push_back("uberShader");
            shaderNames.push_back("asDisneyMaterial");

            MString typeName = depFn.typeName();
            for (uint si = 0; si < shaderNames.size(); si++)
            {
                if (typeName == shaderNames[si])
                {
                    Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate - found shader.") + iElement->name);
                    this->defineMaterial(iElement->obj);
                }
            }
        }
        if (iElement->node.hasFn(MFn::kMesh))
        {
            if (iElement->obj->removed)
            {
                continue;
            }

            if (iElement->triggeredFromTransform)
            {
                Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate mesh ") + iElement->name + " ieNodeName " + getObjectName(iElement->node) + " objDagPath " + iElement->obj->dagPath.fullPathName());
                MStatus stat;

                asr::AssemblyInstance *assInst = getExistingObjectAssemblyInstance(iElement->obj.get());
                if (assInst == 0)
                    continue;

                MMatrix m = iElement->obj->dagPath.inclusiveMatrix(&stat);
                if (!stat)
                    Logging::debug(MString("Error ") + stat.errorString());
                assInst->transform_sequence().clear();
                fillTransformMatrices(m, assInst);
                assInst->bump_version_id();
            }
            else
            {
                if (iElement->obj->instanceNumber == 0)
                    updateGeometry(iElement->obj);
                if (iElement->obj->instanceNumber > 0)
                    updateInstance(iElement->obj);
            }
        }
    }
    interactiveUpdateList.clear();
}

#define colorAttr(c) (MString("") + c.r + " " + c.g + " " + c.g)

void AppleseedRenderer::defineLight(boost::shared_ptr<MayaObject> obj)
{
    asr::Assembly *lightAssembly = getCreateObjectAssembly(obj);
    asr::AssemblyInstance *lightAssemblyInstance = getExistingObjectAssemblyInstance(obj.get());
    asr::Light *light = lightAssembly->lights().get_by_name(obj->shortName.asChar());
    MFnDependencyNode depFn(obj->mobject);

    if (obj->mobject.hasFn(MFn::kPointLight))
    {
        bool cast_indirect_light = getBoolAttr("mtap_cast_indirect_light", depFn, true);
        float importance_multiplier = getFloatAttr("mtap_importance_multiplier", depFn, 1.0f);
        MColor col = getColorAttr("color", depFn);
        float intensity = getFloatAttr("intensity", depFn, 1.0f);
        MString colorAttribute = obj->shortName + "_intensity";
        defineColor(project.get(), colorAttribute.asChar(), col, intensity);
        int decay = getEnumInt("decayRate", depFn);
        if (light == 0)
        {
            asf::auto_release_ptr<asr::Light> lp = asf::auto_release_ptr<asr::Light>(
                asr::PointLightFactory().create(
                obj->shortName.asChar(),
                asr::ParamArray()));
            lightAssembly->lights().insert(lp);
            light = lightAssembly->lights().get_by_name(obj->shortName.asChar());
        }
        asr::ParamArray& params = light->get_parameters();
        params.insert("intensity", colorAttribute);
        params.insert("intensity_multiplier", intensity);
        params.insert("importance_multiplier", importance_multiplier);
        params.insert("cast_indirect_light", cast_indirect_light);
        fillTransformMatrices(obj, light);
    }
    if (obj->mobject.hasFn(MFn::kSpotLight))
    {
        // redefinition because it is possible that this value is textured
        bool cast_indirect_light = getBoolAttr("mtap_cast_indirect_light", depFn, true);
        float importance_multiplier = getFloatAttr("mtap_importance_multiplier", depFn, 1.0f);
        MColor col = getColorAttr("color", depFn);
        float intensity = getFloatAttr("intensity", depFn, 1.0f);
        MString colorAttribute = obj->shortName + "_intensity";
        defineColor(project.get(), colorAttribute.asChar(), col, intensity);
        Logging::debug(MString("Creating spotLight: ") + depFn.name());
        float coneAngle = getDegree("coneAngle", depFn);
        float penumbraAngle = getDegree("penumbraAngle", depFn);
        float inner_angle = coneAngle;
        float outer_angle = coneAngle + penumbraAngle;

        if (light == 0)
        {
            asf::auto_release_ptr<asr::Light> lp = asr::SpotLightFactory().create(
                obj->shortName.asChar(),
                asr::ParamArray());
            lightAssembly->lights().insert(lp);
            light = lightAssembly->lights().get_by_name(obj->shortName.asChar());
        }

        asr::ParamArray& params = light->get_parameters();
        params.insert("radiance", colorAttribute);
        params.insert("radiance_multiplier", intensity);
        params.insert("inner_angle", inner_angle);
        params.insert("outer_angle", outer_angle);
        params.insert("importance_multiplier", importance_multiplier);
        params.insert("cast_indirect_light", cast_indirect_light);
        MMatrix matrix = obj->transformMatrices[0];
        fillTransformMatrices(obj, light);
    }
    if (obj->mobject.hasFn(MFn::kDirectionalLight))
    {
        bool cast_indirect_light = getBoolAttr("mtap_cast_indirect_light", depFn, true);
        float importance_multiplier = getFloatAttr("mtap_importance_multiplier", depFn, 1.0f);
        MVector lightDir(0, 0, -1);
        MVector lightDirTangent(1, 0, 0);
        MVector lightDirBiTangent(0, 1, 0);
        MColor col = getColorAttr("color", depFn);
        float intensity = getFloatAttr("intensity", depFn, 1.0f);
        MString colorAttribute = obj->shortName + "_intensity";
        defineColor(project.get(), colorAttribute.asChar(), col, intensity);

        if (!isSunLight(obj->mobject))
        {
            if (light == 0)
            {
                asf::auto_release_ptr<asr::Light> lp = asr::DirectionalLightFactory().create(
                    obj->shortName.asChar(),
                    asr::ParamArray());
                lightAssembly->lights().insert(lp);
                light = lightAssembly->lights().get_by_name(obj->shortName.asChar());
            }
            asr::ParamArray& params = light->get_parameters();
            params.insert("irradiance", colorAttribute);
            params.insert("irradiance_multiplier", intensity);
            params.insert("importance_multiplier", importance_multiplier);
            params.insert("cast_indirect_light", cast_indirect_light);
        }
        fillTransformMatrices(obj, light);
    }

    if (obj->mobject.hasFn(MFn::kAreaLight))
    {
        MString areaLightName = obj->fullNiceName;
        asf::auto_release_ptr<asr::MeshObject> plane = defineStandardPlane(true);
        plane->set_name(areaLightName.asChar());
        MayaObject *assemblyObject = getAssemblyMayaObject(obj.get());
        asr::Assembly *ass = getCreateObjectAssembly(obj);
        ass->objects().insert(asf::auto_release_ptr<asr::Object>(plane));
        asr::MeshObject *meshPtr = (asr::MeshObject *)ass->objects().get_by_name(areaLightName.asChar());
        MString objectInstanceName = getObjectInstanceName(obj.get());
        MMatrix assemblyObjectMatrix = assemblyObject->dagPath.inclusiveMatrix();
        // rotate the defalt up pointing standard plane by 90 degree to match the area light direction
        MTransformationMatrix tm;
        double rotate90Deg[3] = { -M_PI_2, 0, 0 };
        tm.setRotation(rotate90Deg, MTransformationMatrix::kXYZ);
        MMatrix objectMatrix = tm.asMatrix();
        MMatrix diffMatrix = objectMatrix;
        asf::Matrix4d appleMatrix;
        asf::Matrix4d::identity();
        MMatrixToAMatrix(diffMatrix, appleMatrix);
        appleMatrix = asf::Matrix4d::identity();
        MString areaLightMaterialName = areaLightName + "_material";

        MString physicalSurfaceName = areaLightName + "_physical_surface_shader";
        MString areaLightColorName = areaLightName + "_color";
        MString edfName = areaLightName + "_edf";
        asr::ParamArray edfParams;
        MString lightColor = lightColorAsString(depFn);
        MColor color = getColorAttr("color", depFn);
        defineColor(project.get(), areaLightColorName.asChar(), color, getFloatAttr("intensity", depFn, 1.0f));
        edfParams.insert("radiance", areaLightColorName.asChar());

        asf::auto_release_ptr<asr::EDF> edf = asr::DiffuseEDFFactory().create(edfName.asChar(), edfParams);
        ass->edfs().insert(edf);

        ass->surface_shaders().insert(
            asr::PhysicalSurfaceShaderFactory().create(
            physicalSurfaceName.asChar(),
            asr::ParamArray()));

        ass->materials().insert(
            asr::GenericMaterialFactory().create(
            areaLightMaterialName.asChar(),
            asr::ParamArray()
            .insert("surface_shader", physicalSurfaceName.asChar())
            .insert("edf", edfName.asChar())));

        asr::ParamArray objInstanceParamArray;
        addVisibilityFlags(obj, objInstanceParamArray);

        ass->object_instances().insert(
            asr::ObjectInstanceFactory::create(
            objectInstanceName.asChar(),
            objInstanceParamArray,
            meshPtr->get_name(),
            asf::Transformd::from_local_to_parent(appleMatrix),
            asf::StringDictionary()
            .insert("slot0", areaLightMaterialName.asChar()),
            asf::StringDictionary()
            .insert("slot0", "default")));

        if (lightAssemblyInstance != 0)
            fillMatrices(obj, lightAssemblyInstance->transform_sequence());
    }
}

void AppleseedRenderer::defineLights()
{
    MStatus stat;
    MFnDependencyNode rGlNode(getRenderGlobalsNode());
    // first get the globals node and serach for a directional light connection
    MObject coronaGlobals = getRenderGlobalsNode();
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;

    std::vector<boost::shared_ptr<MayaObject> >::iterator oIt;
    for (oIt = mayaScene->lightList.begin(); oIt != mayaScene->lightList.end(); oIt++)
    {
        boost::shared_ptr<MayaObject> mobj = *oIt;

        if (!mobj->visible)
            continue;

        if (isSunLight(mobj->mobject))
            continue;

        defineLight(mobj);
    }
}

void AppleseedRenderer::updateMaterial(MObject materialNode)
{
    OSLUtilClass OSLShaderClass;
    MObject surfaceShaderNode = getConnectedInNode(materialNode, "surfaceShader");
    MString surfaceShaderName = getObjectName(surfaceShaderNode);
    MString shadingGroupName = getObjectName(materialNode);
    MString shaderGroupName = shadingGroupName + "_OSLShadingGroup";
    ShadingNetwork network(surfaceShaderNode);
    size_t numNodes = network.shaderList.size();
    asr::Assembly *assembly = getMasterAssemblyFromProject(this->project.get());
    asr::ShaderGroup *shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());

    if (shaderGroup != 0)
    {
        shaderGroup->clear();
    }
    else
    {
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
        Logging::debug(MString("ShadingNode Id: ") + shadingNodeId + " ShadingNode name: " + snode.fullName);
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
        Logging::debug(MString("Adding interface shader: ") + layer);
        asr::ShaderGroup *sg = (asr::ShaderGroup *)OSLShaderClass.group;
        sg->add_shader("surface", "surfaceShaderInterface", layer.asChar(), asr::ParamArray());
        const char *srcLayer = snode.fullName.asChar();
        const char *srcAttr = "outColor";
        const char *dstLayer = layer.asChar();
        const char *dstAttr = "inColor";
        Logging::debug(MString("Connecting interface shader: ") + srcLayer + "." + srcAttr + " -> " + dstLayer + "." + dstAttr);
        sg->add_connection(srcLayer, srcAttr, dstLayer, dstAttr);
    }

    MString physicalSurfaceName = shadingGroupName + "_physical_surface_shader";
    // add shaders only if they do not yet exist
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

asf::StringArray AppleseedRenderer::defineMaterial(boost::shared_ptr<MayaObject> obj)
{
    MStatus status;
    asf::StringArray materialNames;
    getObjectShadingGroups(obj->dagPath, obj->perFaceAssignments, obj->shadingGroups, false);
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;

    for (uint sgId = 0; sgId < obj->shadingGroups.length(); sgId++)
    {
        MObject materialNode = obj->shadingGroups[sgId];
        MString surfaceShader;
        MObject surfaceShaderNode = getConnectedInNode(materialNode, "surfaceShader");
        MString surfaceShaderName = getObjectName(surfaceShaderNode);
        MFnDependencyNode depFn(surfaceShaderNode);
        MString typeName = depFn.typeName();

        // if the connected surface shader is not supported, use a default material because otherwise osl will crash
        if (!ShaderDefinitions::shadingNodeSupported(typeName))
        {
            Logging::warning(MString("Surface shader type: ") + typeName + " is not supported, using default material.");
            MString objectInstanceName = getObjectInstanceName(obj.get());
            asr::Assembly *ass = getCreateObjectAssembly(obj);
            MString shadingGroupName = getObjectName(materialNode);
            asr::ObjectInstance *objInstance = ass->object_instances().get_by_name(objectInstanceName.asChar());
            objInstance->get_front_material_mappings().insert("slot0", "default");
            objInstance->get_back_material_mappings().insert("slot0", "default");
            continue;
        }

        // if we are in IPR mode, save all translated shading nodes to the interactive update list
        if (getWorldPtr()->renderType == MayaToWorld::IPRRENDER)
        {
            if (mayaScene)
            {
                InteractiveElement iel;
                iel.mobj = surfaceShaderNode;
                iel.obj = obj;
                iel.name = surfaceShaderName;
                iel.node = materialNode;
                mayaScene->interactiveUpdateMap[mayaScene->interactiveUpdateMap.size()] = iel;

                if (getWorldPtr()->renderState == MayaToWorld::RSTATERENDERING)
                {
                    RenderQueueWorker::IPRUpdateCallbacks();
                }
            }
        }

        updateMaterial(materialNode);

        MString objectInstanceName = getObjectInstanceName(obj.get());
        asr::Assembly *ass = getCreateObjectAssembly(obj);
        MString shadingGroupName = getObjectName(materialNode);
        asr::ObjectInstance *objInstance = ass->object_instances().get_by_name(objectInstanceName.asChar());
        objInstance->get_front_material_mappings().insert("slot0", shadingGroupName.asChar());
        objInstance->get_back_material_mappings().insert("slot0", shadingGroupName.asChar());
    }

    return materialNames;
}

void AppleseedRenderer::updateTransform(boost::shared_ptr<MayaObject> obj)
{
    Logging::debug(MString("AppleseedRenderer::updateTransform: ") + obj->shortName);
}

void AppleseedRenderer::updateShape(boost::shared_ptr<MayaObject> obj)
{
    Logging::debug(MString("AppleseedRenderer::updateShape: ") + obj->shortName);
}
