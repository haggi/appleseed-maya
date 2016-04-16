
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
#include "shadingtools/material.h"
#include "shadingtools/shaderdefinitions.h"
#include "shadingtools/shadingutils.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "utilities/meshtools.h"
#include "utilities/oslutils.h"
#include "utilities/pystring.h"
#include "utilities/tools.h"
#include "appleseedutils.h"
#include "mayascene.h"
#include "renderglobals.h"
#include "renderqueueworker.h"
#include "tilecallback.h"
#include "world.h"

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
    renderer::global_logger().set_format(foundation::LogMessage::Debug, "");
    log_target.reset(foundation::create_console_log_target(stdout));
    renderer::global_logger().add_target(log_target.get());
}

AppleseedRenderer::~AppleseedRenderer()
{
    if (log_target.get() != 0)
        renderer::global_logger().remove_target(log_target.get());
}

void AppleseedRenderer::initializeRenderer()
{
    project = renderer::ProjectFactory::create("mayaProject");

    std::string oslShaderPath = (getRendererHome() + "shaders").asChar();
    Logging::debug(MString("setting osl shader search path to: ") + oslShaderPath.c_str());
    project->search_paths().push_back(oslShaderPath.c_str());
    for (uint i = 0; i < getWorldPtr()->shaderSearchPath.length(); i++)
    {
        Logging::debug(MString("Search path: ") + getWorldPtr()->shaderSearchPath[i]);
        project->search_paths().push_back(getWorldPtr()->shaderSearchPath[i].asChar());
    }

    defineConfig();
    defineScene(project.get());
}

void AppleseedRenderer::unInitializeRenderer()
{
    getWorldPtr()->setRenderState(World::RSTATEDONE);
    getWorldPtr()->setRenderType(World::RTYPENONE);

    // todo: isn't this supposed to be reset()?
    project.release();
}

void AppleseedRenderer::defineProject()
{
    defineCamera();
    defineOutput(); // output accesses camera so define it after camera
    defineMasterAssembly(project.get());
    defineDefaultMaterial(project.get());
    defineEnvironment(); // define environment before lights because sun lights may use physical sky edf
    defineGeometry();
    defineLights();
}

void AppleseedRenderer::preFrame()
{
    defineProject();
}

void AppleseedRenderer::postFrame()
{
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->mRenderGlobals;
    renderGlobals->getImageName();
    MString filename = renderGlobals->imageOutputFile.asChar();
    Logging::debug(MString("Saving image as ") + renderGlobals->imageOutputFile);
    project->get_frame()->write_main_image(renderGlobals->imageOutputFile.asChar());

    // If we render the very last frame or if we are in UI where the last frame == first frame, then delete the master renderer before
    // the deletion of the assembly because otherwise it will be deleted automatically if the renderer instance is deleted what results in a crash
    // because the masterRenderer still have references to the shading groups which are defined in the world assembly. If the masterRenderer is deleted
    // AFTER the assembly it tries to access non existent shadingGroups.
    if (renderGlobals->currentFrameNumber == renderGlobals->frameList.back())
        masterRenderer.reset();

    foundation::UniqueID aiuid = project->get_scene()->assembly_instances().get_by_name("world_Inst")->get_uid();
    foundation::UniqueID auid = project->get_scene()->assemblies().get_by_name("world")->get_uid();
    project->get_scene()->assembly_instances().remove(aiuid);
    project->get_scene()->assemblies().remove(auid);
    renderer::Assembly *worldass = project->get_scene()->assemblies().get_by_name("world");
}

void AppleseedRenderer::render()
{
    if (!sceneBuilt)
    {
        tileCallbackFac.reset(new TileCallbackFactory());

        if (getWorldPtr()->getRenderType() == World::IPRRENDER)
        {
            masterRenderer.reset(
                new renderer::MasterRenderer(
                    project.ref(),
                    project->configurations().get_by_name("interactive")->get_inherited_parameters(),
                    &mRendererController,
                    tileCallbackFac.get()));
        }
        else
        {
            masterRenderer.reset(
                new renderer::MasterRenderer(
                    project.ref(),
                    project->configurations().get_by_name("final")->get_inherited_parameters(),
                    &mRendererController,
                    tileCallbackFac.get()));
        }

        const MFnDependencyNode renderGlobalsFn(getRenderGlobalsNode());
        const int exportMode = getEnumInt("exportMode", renderGlobalsFn);
        if (exportMode > 0)
        {
            const MString outputFile = getStringAttr("exportSceneFileName", renderGlobalsFn, "");
            renderer::ProjectFileWriter::write(project.ref(), outputFile.asChar());
            if (exportMode == 1) // export only, no rendering
                return;
        }

        sceneBuilt = true;
    }

    getWorldPtr()->setRenderState(World::RSTATERENDERING);
    mRendererController.set_status(renderer::IRendererController::ContinueRendering);
    masterRenderer->render();
}

void AppleseedRenderer::abortRendering()
{
    mRendererController.set_status(renderer::IRendererController::AbortRendering);
}

void AppleseedRenderer::addRenderParams(renderer::ParamArray& paramArray)
{
    static const char* lightingEngines[] = { "pt", "drt", "sppm" };
    static const char* bucketOrders[] = { "linear", "spiral", "hilbert", "random" };
    static const char* photonTypes[] = { "mono", "poly" };
    static const char* dlTypes[] = { "rt", "sppm", "off" };
    static const char* samplingModes[] = { "qmc", "rng" };

    MFnDependencyNode renderGlobalsFn(getRenderGlobalsNode());
    MString lightingEngine = lightingEngines[getEnumInt("lightingEngine", renderGlobalsFn)];
    MString bucketOrder = bucketOrders[getEnumInt("tile_ordering", renderGlobalsFn)];
    MString photonType = photonTypes[getEnumInt("photon_type", renderGlobalsFn)];
    MString dlType = dlTypes[getEnumInt("dl_mode", renderGlobalsFn)];
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->mRenderGlobals;

    paramArray.insert_path("texture_store.max_size", getIntAttr("texCacheSize", renderGlobalsFn, 128) * 1024 * 1024); // at least 128 MB

    paramArray.insert("sampling_mode", samplingModes[getEnumInt("sampling_mode", renderGlobalsFn)]);
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
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->mRenderGlobals;

    project->add_default_configurations();
    addRenderParams(project->configurations().get_by_name("final")->get_parameters());
    addRenderParams(project->configurations().get_by_name("interactive")->get_parameters());

    int renderer = getEnumInt("pixel_renderer", renderGlobalsFn);
    const char *pixel_renderers[2] = { "adaptive", "uniform" };
    const char *pixel_renderer = pixel_renderers[renderer];

    project->configurations()
        .get_by_name("final")->get_parameters()
            .insert_path("pixel_renderer", pixel_renderer)
            .insert_path("generic_tile_renderer.sampler", pixel_renderer);

    project->configurations()
        .get_by_name("interactive")->get_parameters()
            .insert_path("generic_tile_renderer.sampler", pixel_renderer);

    if (renderGlobals->getUseRenderRegion())
    {
        const int imgWidth = renderGlobals->getWidth();
        const int imgHeight = renderGlobals->getHeight();

        int left, right, bottom, top;
        renderGlobals->getRenderRegion(left, bottom, right, top);

        const int ybot = imgHeight - bottom;
        const int ytop = imgHeight - top;
        const int ymin = ybot < ytop ? ybot : ytop;
        const int ymax = ybot > ytop ? ybot : ytop;

        const MString regionString = MString("") + left + " " + ymin + " " + right + " " + ymax;

        project->configurations()
            .get_by_name("final")->get_parameters()
            .insert_path("generic_tile_renderer.crop_window", regionString);
    }

    project->configurations()
        .get_by_name("interactive")->get_parameters()
            .insert_path("generic_tile_renderer.filter", renderGlobals->filterTypeString.toLowerCase().asChar())
            .insert_path("generic_tile_renderer.filter_size", renderGlobals->filterSize);

    renderer::Configuration *cfg = project->configurations().get_by_name("interactive");
    renderer::ParamArray &params = cfg->get_parameters();
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
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->mRenderGlobals;
    float horizontalFilmAperture = 24.892f;
    float verticalFilmAperture = 18.669f;

    const int width = renderGlobals->getWidth();
    const int height = renderGlobals->getHeight();

    float imageAspect = (float)width / (float)height;
    float mtap_cameraType = 0;
    int mtap_diaphragm_blades = 0;
    float mtap_diaphragm_tilt_angle = 0.0;
    float focusDistance = 0.0;
    float fStop = 0.0;

    float focalLength = 35.0f;
    MFnCamera camFn(cam->mobject, &stat);
    renderer::ParamArray camParams;

    getFloat(MString("horizontalFilmAperture"), camFn, horizontalFilmAperture);
    getFloat(MString("verticalFilmAperture"), camFn, verticalFilmAperture);
    getFloat(MString("focalLength"), camFn, focalLength);
    bool dof = getBoolAttr("depthOfField", camFn, false) && renderGlobals->doDof;

    getFloat(MString("focusDistance"), camFn, focusDistance);
    getFloat(MString("fStop"), camFn, fStop);
    getInt(MString("mtap_diaphragm_blades"), camFn, mtap_diaphragm_blades);
    getFloat(MString("mtap_diaphragm_tilt_angle"), camFn, mtap_diaphragm_tilt_angle);

    // This is a hack because this camera model does not support NON depth of field.
    if (!dof)
        fStop *= 10000.0f;

    // Maya's aperture is given in inces so convert to cm and convert to meters.
    horizontalFilmAperture = horizontalFilmAperture * 2.54f * 0.01f;
    verticalFilmAperture = verticalFilmAperture * 2.54f * 0.01f;
    verticalFilmAperture = horizontalFilmAperture / imageAspect;
    camParams.insert("film_dimensions", format("^1s ^2s", horizontalFilmAperture, verticalFilmAperture));
    camParams.insert("focal_length", focalLength * 0.001f);
    camParams.insert("focal_distance", focusDistance * renderGlobals->scaleFactor);
    camParams.insert("f_stop", fStop);
    camParams.insert("diaphragm_blades", mtap_diaphragm_blades);
    camParams.insert("diaphragm_tilt_angle", mtap_diaphragm_tilt_angle);
    foundation::auto_release_ptr<renderer::Camera> appleCam = renderer::ThinLensCameraFactory().create(
        cam->shortName.asChar(),
        camParams);
    renderer::Camera* camera = appleCam.get();
    project->get_scene()->set_camera(appleCam);
    fillMatrices(cam, camera->transform_sequence());
}

void AppleseedRenderer::defineCamera()
{
    MStatus stat;
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->mRenderGlobals;

    std::vector<boost::shared_ptr<MayaObject> >::iterator oIt;
    for (oIt = mayaScene->camList.begin(); oIt != mayaScene->camList.end(); oIt++)
    {
        boost::shared_ptr<MayaObject> cam = *oIt;
        if (!isCameraRenderable(cam->mobject) && (!(cam->dagPath == mayaScene->uiCamera)))
        {
            continue;
        }

        defineCamera(cam);

        // Appleseed does not support more than one camera at the moment, so break after the first one.
        break;
    }
}

void AppleseedRenderer::defineOutput()
{
    if (project->get_frame() == 0)
    {
        static const char* colorSpaces[] = { "linear_rgb", "srgb", "ciexyz" };
        MFnDependencyNode depFn(getRenderGlobalsNode());
        boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->mRenderGlobals;

        const int width = renderGlobals->getWidth();
        const int height = renderGlobals->getHeight();

        project->set_frame(
            renderer::FrameFactory::create(
                "beauty",
                renderer::ParamArray()
                    .insert("camera", project->get_scene()->get_camera()->get_name())
                    .insert("resolution", MString("") + width + " " + height)
                    .insert("tile_size", MString("") + renderGlobals->tilesize + " " + renderGlobals->tilesize)
                    .insert("color_space", colorSpaces[getEnumInt("colorSpace", depFn)])));
    }
}

void AppleseedRenderer::defineEnvironment()
{
    renderer::Scene* scene = project->get_scene();

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

    foundation::auto_release_ptr<renderer::EnvironmentEDF> environmentEDF;

    switch (environmentType)
    {
        // Constant.
        case 0:
        {
            environmentEDF =
                renderer::ConstantEnvironmentEDFFactory().create(
                    "sky_edf",
                    renderer::ParamArray()
                        .insert("radiance", envColorName));
            break;
        }

        // ConstantHemisphere.
        case 1:
        {
            environmentEDF =
                renderer::ConstantHemisphereEnvironmentEDFFactory().create(
                    "sky_edf",
                    renderer::ParamArray()
                        .insert("radiance", envColorName)
                        .insert("upper_hemi_radiance", gradHorizName)
                        .insert("lower_hemi_radiance", gradZenitName));
            break;
        }

        // Gradient.
        case 2:
        {
            environmentEDF =
                renderer::GradientEnvironmentEDFFactory().create(
                    "sky_edf",
                    renderer::ParamArray()
                        .insert("horizon_radiance", gradHorizName)
                        .insert("zenith_radiance", gradZenitName));
            break;
        }

        // Latitude-Longitude.
        case 3:
        {
            environmentEDF =
                renderer::LatLongMapEnvironmentEDFFactory().create(
                    "sky_edf",
                    renderer::ParamArray()
                        .insert("radiance", envMapAttrName.asChar())
                        .insert("radiance_multiplier", environmentIntensity)
                        .insert("horizontal_shift", latlongHoShift)
                        .insert("vertical_shift", latlongVeShift));
            break;
        }

        // Mirror Ball.
        case 4:
        {
            environmentEDF =
                renderer::MirrorBallMapEnvironmentEDFFactory().create(
                    "sky_edf",
                    renderer::ParamArray()
                        .insert("radiance", envMapAttrName.asChar())
                        .insert("radiance_multiplier", environmentIntensity));
            break;
        }

        // Physical Sky.
        case 5:
        {
            double sunTheta = getDoubleAttr("sun_theta", appleseedGlobals, 30.0);
            double sunPhi = getDoubleAttr("sun_phi", appleseedGlobals, 60.0);

            const bool usePhysicalSun = getBoolAttr("physicalSun", appleseedGlobals, true);
            if (usePhysicalSun)
            {
                // Get the connected sun light.
                MObject connectedNode = getConnectedInNode(getRenderGlobalsNode(), "physicalSunConnection");
                if (connectedNode != MObject::kNullObj)
                {
                    MFnTransform tn(connectedNode);
                    MMatrix tm = tn.transformationMatrix(&stat);
                    if (stat)
                    {
                        MVector sunOrient(0.0, 0.0, 1.0);
                        sunOrient *= tm;
                        sunOrient.normalize();
                        const foundation::Vector3d unitVector(sunOrient.x, sunOrient.y, sunOrient.z);
                        renderer::unit_vector_to_angles(unitVector, sunTheta, sunPhi);
                        sunTheta = foundation::rad_to_deg(sunTheta);
                        sunPhi = foundation::rad_to_deg(sunPhi);
                    }
                }
                else
                {
                    Logging::warning("physicalSunConnection plug has no connection, but use physical sun is turned on. Please correct.");
                }
            }

            if (skyModel == 0) // Preetham
            {
                environmentEDF =
                    renderer::PreethamEnvironmentEDFFactory().create(
                        "sky_edf",
                        renderer::ParamArray()
                            .insert("horizon_shift", horizon_shift)
                            .insert("luminance_multiplier", luminance_multiplier)
                            .insert("saturation_multiplier", saturation_multiplier)
                            .insert("sun_phi", sunPhi)
                            .insert("sun_theta", sunTheta)
                            .insert("turbidity", turbidity)
                            .insert("turbidity_max", turbidity_max)
                            .insert("turbidity_min", turbidity_min));
            }
            else // Hosek
            { 
                environmentEDF =
                    renderer::HosekEnvironmentEDFFactory().create(
                        "sky_edf",
                        renderer::ParamArray()
                            .insert("ground_albedo", ground_albedo)
                            .insert("horizon_shift", horizon_shift)
                            .insert("luminance_multiplier", luminance_multiplier)
                            .insert("saturation_multiplier", saturation_multiplier)
                            .insert("sun_phi", sunPhi)
                            .insert("sun_theta", sunTheta)
                            .insert("turbidity", turbidity)
                            .insert("turbidity_max", turbidity_max)
                            .insert("turbidity_min", turbidity_min));
            }

            break;
        }

        // OSL Sky.
        case 6:
        {
            foundation::auto_release_ptr<renderer::ShaderGroup> oslShaderGroup = renderer::ShaderGroupFactory().create("OSL_Sky");
            oslShaderGroup->add_shader("surface", "testBG", "BGLayer", renderer::ParamArray());
            scene->shader_groups().insert(oslShaderGroup);
            environmentEDF = renderer::OSLEnvironmentEDFFactory().create("sky_edf", renderer::ParamArray()
                .insert("osl_background", "OSL_Sky")
                .insert("radiance_multiplier", environmentIntensity));

            break;
        }
    };

    renderer::EnvironmentEDF *sky = scene->environment_edfs().get_by_name("sky_edf");
    if (sky != 0)
        scene->environment_edfs().remove(sky);

    scene->environment_edfs().insert(environmentEDF);

    // Create an environment shader called "sky_shader" and insert it into the scene.
    if (scene->environment_shaders().get_by_name("sky_shader") != 0)
    {
        renderer::EnvironmentShader *skyShader = scene->environment_shaders().get_by_name("sky_shader");
        scene->environment_shaders().remove(skyShader);
    }
    scene->environment_shaders().insert(
        renderer::EDFEnvironmentShaderFactory().create(
            "sky_shader",
            renderer::ParamArray()
                .insert("environment_edf", "sky_edf")));

    // Create an environment called "sky" and bind it to the scene.
    scene->set_environment(
        renderer::EnvironmentFactory::create(
            "sky",
            renderer::ParamArray()
                .insert("environment_edf", "sky_edf")
                .insert("environment_shader", "sky_shader")));
}

namespace
{
    renderer::GVector3 MPointToAppleseed(const MPoint& p)
    {
        return
            renderer::GVector3(
                static_cast<float>(p.x),
                static_cast<float>(p.y),
                static_cast<float>(p.z));
    }
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
    obj->getShadingGroups();
    obj->getMeshData(points, normals, uArray, vArray, triPointIds, triNormalIds, triUvIds, triMatIds);

    Logging::debug(MString("Translating mesh object ") + meshFn.name().asChar());
    MString meshFullName = makeGoodString(meshFn.fullPathName());

    // Create a new mesh object.
    foundation::auto_release_ptr<renderer::MeshObject> mesh = renderer::MeshObjectFactory::create(meshFullName.asChar(), renderer::ParamArray());

    for (uint vtxId = 0; vtxId < points.length(); vtxId++)
        mesh->push_vertex(MPointToAppleseed(points[vtxId]));

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
        mesh->push_tex_coords(renderer::GVector2((float)uArray[tId], (float)vArray[tId]));

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
        mesh->push_triangle(renderer::Triangle(vtxId0, vtxId1, vtxId2,  normalId0, normalId1, normalId2, uvId0, uvId1, uvId2, perFaceShadingGroup));
    }

    MayaObject* assemblyObject = getAssemblyMayaObject(obj.get());
    renderer::Assembly *ass = getCreateObjectAssembly(obj);

    Logging::debug(MString("Placing mesh ") + mesh->get_name() + " into assembly " + ass->get_name());

    renderer::MeshObject *meshPtr = (renderer::MeshObject *)ass->objects().get_by_name(meshFullName.asChar());

    if (meshPtr != 0)
    {
        Logging::debug(MString("Mesh object ") + meshFullName + " is already defined, removing...");
        ass->objects().remove(meshPtr);
        ass->bump_version_id();
    }

    ass->objects().insert(foundation::auto_release_ptr<renderer::Object>(mesh));
    meshPtr = (renderer::MeshObject *)ass->objects().get_by_name(meshFullName.asChar());

    MString objectInstanceName = getObjectInstanceName(obj.get());

    MMatrix assemblyObjectMatrix = assemblyObject->dagPath.inclusiveMatrix();
    MMatrix objectMatrix = obj->dagPath.inclusiveMatrix();
    MMatrix diffMatrix = objectMatrix * assemblyObjectMatrix.inverse();
    foundation::Matrix4d appleMatrix;
    MMatrixToAMatrix(diffMatrix, appleMatrix);

    renderer::ParamArray objInstanceParamArray;
    addVisibilityFlags(obj, objInstanceParamArray);

    renderer::ObjectInstance *oinst = ass->object_instances().get_by_name(objectInstanceName.asChar());
    if (oinst != 0)
    {
        Logging::debug(MString("Mesh object instance ") + objectInstanceName + " is already defined, removing...");
        ass->object_instances().remove(oinst);
        ass->bump_version_id();
    }
    ass->object_instances().insert(
        renderer::ObjectInstanceFactory::create(
            objectInstanceName.asChar(),
            objInstanceParamArray,
            meshPtr->get_name(),
            foundation::Transformd::from_local_to_parent(appleMatrix),
            foundation::StringDictionary()));
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

        foundation::auto_release_ptr<renderer::AssemblyInstance> assemblyInstance(
            renderer::AssemblyInstanceFactory::create(
                assemblyInstanceName.asChar(),
                renderer::ParamArray(),
                assemblyName.asChar()));
        renderer::TransformSequence &ts = assemblyInstance->transform_sequence();
        fillMatrices(mobj, ts);
        getMasterAssemblyFromProject(project.get())->assembly_instances().insert(assemblyInstance);
    }
}

void AppleseedRenderer::defineGeometry()
{
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->mRenderGlobals;
    std::vector<boost::shared_ptr<MayaObject> >::iterator oIt;
    for (oIt = mayaScene->objectList.begin(); oIt != mayaScene->objectList.end(); oIt++)
    {
        boost::shared_ptr<MayaObject> mobj = *oIt;
        updateGeometry(mobj);
    }

    // Create assembly instances.
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

        foundation::auto_release_ptr<renderer::AssemblyInstance> assemblyInstance(
            renderer::AssemblyInstanceFactory::create(
            assemblyInstanceName.asChar(),
            renderer::ParamArray(),
            assemblyName.asChar()));
        renderer::TransformSequence &ts = assemblyInstance->transform_sequence();
        fillMatrices(mobj, ts);
        getMasterAssemblyFromProject(project.get())->assembly_instances().insert(assemblyInstance);
    }
}

void AppleseedRenderer::applyInteractiveUpdates(const std::vector<InteractiveElement*>& modifiedElementList)
{
    std::vector<InteractiveElement *>::const_iterator iaIt;
    for (iaIt = modifiedElementList.begin(); iaIt != modifiedElementList.end(); iaIt++)
    {
        InteractiveElement* element = *iaIt;

        if (element->node.hasFn(MFn::kShadingEngine))
        {
            if (element->obj)
            {
                Logging::debug(MString("AppleseedRenderer::applyInteractiveUpdates() - found shadingEngine.") + element->name);
                updateMaterial(element->node);
            }
        }

        if (element->node.hasFn(MFn::kCamera))
        {
            Logging::debug(MString("AppleseedRenderer::applyInteractiveUpdates() - found camera.") + element->name);
            if (element->obj)
                defineCamera(element->obj);
        }

        if (element->node.hasFn(MFn::kLight))
        {
            Logging::debug(MString("AppleseedRenderer::applyInteractiveUpdates() - found light.") + element->name);
            if (element->obj)
                defineLight(element->obj);
        }

        // appleseedGlobals node.
        if (MFnDependencyNode(element->node).typeId().id() == 0x0011CF40)
        {
            defineEnvironment();
        }

        if (element->node.hasFn(MFn::kMesh))
        {
            if (element->obj->removed)
                continue;

            if (element->triggeredFromTransform)
            {
                Logging::debug(MString("AppleseedRenderer::applyInteractiveUpdates() mesh ") + element->name + " ieNodeName " + getObjectName(element->node) + " objDagPath " + element->obj->dagPath.fullPathName());

                renderer::AssemblyInstance *assInst = getExistingObjectAssemblyInstance(element->obj.get());
                if (assInst == 0)
                    continue;

                MStatus stat;
                MMatrix m = element->obj->dagPath.inclusiveMatrix(&stat);
                if (!stat)
                    Logging::debug(MString("Error ") + stat.errorString());
                assInst->transform_sequence().clear();
                fillTransformMatrices(m, assInst);
                assInst->bump_version_id();
            }
            else
            {
                if (element->obj->instanceNumber == 0)
                    updateGeometry(element->obj);
                if (element->obj->instanceNumber > 0)
                    updateInstance(element->obj);
            }
        }
    }
}

void AppleseedRenderer::defineLight(boost::shared_ptr<MayaObject> obj)
{
    renderer::Assembly *lightAssembly = getCreateObjectAssembly(obj);
    renderer::AssemblyInstance *lightAssemblyInstance = getExistingObjectAssemblyInstance(obj.get());
    renderer::Light *light = lightAssembly->lights().get_by_name(obj->shortName.asChar());
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
            foundation::auto_release_ptr<renderer::Light> lp = foundation::auto_release_ptr<renderer::Light>(
                renderer::PointLightFactory().create(
                obj->shortName.asChar(),
                renderer::ParamArray()));
            light = lp.get();
            lightAssembly->lights().insert(lp);
        }
        renderer::ParamArray& params = light->get_parameters();
        params.insert("intensity", colorAttribute);
        params.insert("intensity_multiplier", intensity);
        params.insert("importance_multiplier", importance_multiplier);
        params.insert("cast_indirect_light", cast_indirect_light);
        fillTransformMatrices(obj, light);
    }
    if (obj->mobject.hasFn(MFn::kSpotLight))
    {
        // Redefinition because it is possible that this value is textured.
        bool cast_indirect_light = getBoolAttr("mtap_cast_indirect_light", depFn, true);
        float importance_multiplier = getFloatAttr("mtap_importance_multiplier", depFn, 1.0f);
        MColor col = getColorAttr("color", depFn);
        float intensity = getFloatAttr("intensity", depFn, 1.0f);
        MString colorAttribute = obj->shortName + "_intensity";
        defineColor(project.get(), colorAttribute.asChar(), col, intensity);
        Logging::debug(MString("Creating spotLight: ") + depFn.name());
        const double coneAngle = getDegrees("coneAngle", depFn);
        const double penumbraAngle = getDegrees("penumbraAngle", depFn);
        const double innerAngle = coneAngle;
        const double outerAngle = coneAngle + penumbraAngle;

        if (light == 0)
        {
            foundation::auto_release_ptr<renderer::Light> lp =
                renderer::SpotLightFactory().create(
                    obj->shortName.asChar(),
                    renderer::ParamArray());
            light = lp.get();
            lightAssembly->lights().insert(lp);
        }

        renderer::ParamArray& params = light->get_parameters();
        params.insert("radiance", colorAttribute);
        params.insert("radiance_multiplier", intensity);
        params.insert("inner_angle", innerAngle);
        params.insert("outer_angle", outerAngle);
        params.insert("importance_multiplier", importance_multiplier);
        params.insert("cast_indirect_light", cast_indirect_light);
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
        bool isSunlight = isSunLight(obj->mobject);

        if (light == 0)
        {
            if (isSunlight)
            {
                foundation::auto_release_ptr<renderer::Light> lp = renderer::SunLightFactory().create(
                    obj->shortName.asChar(),
                    renderer::ParamArray());
                light = lp.get();
                lightAssembly->lights().insert(lp);
            }
            else
            {
                foundation::auto_release_ptr<renderer::Light> lp = renderer::DirectionalLightFactory().create(
                    obj->shortName.asChar(),
                    renderer::ParamArray());
                light = lp.get();
                lightAssembly->lights().insert(lp);
            }
        }

        renderer::ParamArray& params = light->get_parameters();
        if (isSunlight)
        {
            defineEnvironment(); // update environment
            float turbidity = getFloatAttr("mtap_turbidity", depFn, 4.0f);
            params.insert("turbidity", turbidity);
            params.insert("radiance_multiplier", intensity);
            params.insert("environment_edf", "sky_edf");
            params.insert("irradiance", colorAttribute);
            params.insert("irradiance_multiplier", intensity);
        }
        else
        {
            params.insert("irradiance", colorAttribute);
            params.insert("irradiance_multiplier", intensity);
        }
        params.insert("importance_multiplier", importance_multiplier);
        params.insert("cast_indirect_light", cast_indirect_light);

        fillTransformMatrices(obj, light);
    }

    if (obj->mobject.hasFn(MFn::kAreaLight))
    {
        MString areaLightName = obj->fullNiceName;
        foundation::auto_release_ptr<renderer::MeshObject> plane = defineStandardPlane();
        plane->set_name(areaLightName.asChar());
        MayaObject* assemblyObject = getAssemblyMayaObject(obj.get());
        renderer::Assembly* ass = getCreateObjectAssembly(obj);
        ass->objects().insert(foundation::auto_release_ptr<renderer::Object>(plane));
        renderer::MeshObject* meshPtr = (renderer::MeshObject *)ass->objects().get_by_name(areaLightName.asChar());
        MString objectInstanceName = getObjectInstanceName(obj.get());
        MMatrix assemblyObjectMatrix = assemblyObject->dagPath.inclusiveMatrix();
        // Rotate the defalt up pointing standard plane by 90 degree to match the area light direction.
        MTransformationMatrix tm;
        double rotate90Deg[3] = { -M_PI_2, 0, 0 };
        tm.setRotation(rotate90Deg, MTransformationMatrix::kXYZ);
        MMatrix objectMatrix = tm.asMatrix();
        MMatrix diffMatrix = objectMatrix;
        foundation::Matrix4d appleMatrix;
        foundation::Matrix4d::identity();
        MMatrixToAMatrix(diffMatrix, appleMatrix);
        appleMatrix = foundation::Matrix4d::identity();
        MString areaLightMaterialName = areaLightName + "_material";

        MString physicalSurfaceName = areaLightName + "_physical_surface_shader";
        MString areaLightColorName = areaLightName + "_color";
        MString edfName = areaLightName + "_edf";
        renderer::ParamArray edfParams;
        MColor color = getColorAttr("color", depFn);
        defineColor(project.get(), areaLightColorName.asChar(), color, getFloatAttr("intensity", depFn, 1.0f));
        edfParams.insert("radiance", areaLightColorName.asChar());

        foundation::auto_release_ptr<renderer::EDF> edf = renderer::DiffuseEDFFactory().create(edfName.asChar(), edfParams);
        ass->edfs().insert(edf);

        ass->surface_shaders().insert(
            renderer::PhysicalSurfaceShaderFactory().create(
                physicalSurfaceName.asChar(),
                renderer::ParamArray()));

        ass->materials().insert(
            renderer::GenericMaterialFactory().create(
                areaLightMaterialName.asChar(),
                renderer::ParamArray()
                    .insert("surface_shader", physicalSurfaceName.asChar())
                    .insert("edf", edfName.asChar())));

        renderer::ParamArray objInstanceParamArray;
        addVisibilityFlags(obj, objInstanceParamArray);

        ass->object_instances().insert(
            renderer::ObjectInstanceFactory::create(
                objectInstanceName.asChar(),
                objInstanceParamArray,
                meshPtr->get_name(),
                foundation::Transformd::from_local_to_parent(appleMatrix),
                foundation::StringDictionary()
                    .insert("slot0", areaLightMaterialName.asChar()),
                foundation::StringDictionary()
                    .insert("slot0", "default")));

        if (lightAssemblyInstance != 0)
            fillMatrices(obj, lightAssemblyInstance->transform_sequence());
    }
}

void AppleseedRenderer::defineLights()
{
    MStatus stat;
    MFnDependencyNode rGlNode(getRenderGlobalsNode());
    // First get the globals node and serach for a directional light connection.
    MObject coronaGlobals = getRenderGlobalsNode();
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->mRenderGlobals;
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

    std::vector<boost::shared_ptr<MayaObject> >::iterator oIt;
    for (oIt = mayaScene->lightList.begin(); oIt != mayaScene->lightList.end(); oIt++)
    {
        boost::shared_ptr<MayaObject> mobj = *oIt;

        if (!mobj->visible)
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
    renderer::Assembly *assembly = getMasterAssemblyFromProject(project.get());
    renderer::ShaderGroup *shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());

    if (shaderGroup != 0)
    {
        shaderGroup->clear();
    }
    else
    {
        foundation::auto_release_ptr<renderer::ShaderGroup> oslShadingGroup = renderer::ShaderGroupFactory().create(shaderGroupName.asChar());
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
        renderer::ShaderGroup *sg = (renderer::ShaderGroup *)OSLShaderClass.group;
        sg->add_shader("surface", "surfaceShaderInterface", layer.asChar(), renderer::ParamArray());
        const char *srcLayer = snode.fullName.asChar();
        const char *srcAttr = "outColor";
        const char *dstLayer = layer.asChar();
        const char *dstAttr = "inColor";
        Logging::debug(MString("Connecting interface shader: ") + srcLayer + "." + srcAttr + " -> " + dstLayer + "." + dstAttr);
        sg->add_connection(srcLayer, srcAttr, dstLayer, dstAttr);
    }

    MString physicalSurfaceName = shadingGroupName + "_physical_surface_shader";
    // Add shaders only if they do not yet exist.
    if (assembly->surface_shaders().get_by_name(physicalSurfaceName.asChar()) == 0)
    {
        assembly->surface_shaders().insert(
            renderer::PhysicalSurfaceShaderFactory().create(
                physicalSurfaceName.asChar(),
                renderer::ParamArray()));
    }
    if (assembly->materials().get_by_name(shadingGroupName.asChar()) == 0)
    {
        assembly->materials().insert(
            renderer::OSLMaterialFactory().create(
                shadingGroupName.asChar(),
                renderer::ParamArray()
                    .insert("surface_shader", physicalSurfaceName.asChar())
                    .insert("osl_surface", shaderGroupName.asChar())));
    }
}

foundation::StringArray AppleseedRenderer::defineMaterial(boost::shared_ptr<MayaObject> obj)
{
    MStatus status;
    foundation::StringArray materialNames;
    getObjectShadingGroups(obj->dagPath, obj->perFaceAssignments, obj->shadingGroups, false);
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

    for (uint sgId = 0; sgId < obj->shadingGroups.length(); sgId++)
    {
        MObject materialNode = obj->shadingGroups[sgId];
        MString surfaceShader;
        MObject surfaceShaderNode = getConnectedInNode(materialNode, "surfaceShader");
        MString surfaceShaderName = getObjectName(surfaceShaderNode);
        MFnDependencyNode depFn(surfaceShaderNode);
        MString typeName = depFn.typeName();

        // If the connected surface shader is not supported, use a default material because otherwise osl will crash.
        if (!ShaderDefinitions::shadingNodeSupported(typeName))
        {
            Logging::warning(MString("Surface shader type: ") + typeName + " is not supported, using default material.");
            MString objectInstanceName = getObjectInstanceName(obj.get());
            renderer::Assembly *ass = getCreateObjectAssembly(obj);
            MString shadingGroupName = getObjectName(materialNode);
            renderer::ObjectInstance *objInstance = ass->object_instances().get_by_name(objectInstanceName.asChar());
            objInstance->get_front_material_mappings().insert("slot0", "default");
            objInstance->get_back_material_mappings().insert("slot0", "default");
            continue;
        }

        // If we are in IPR mode, save all translated shading nodes to the interactive update list.
        if (getWorldPtr()->getRenderType() == World::IPRRENDER)
        {
            if (mayaScene)
            {
                InteractiveElement iel;
                iel.mobj = surfaceShaderNode;
                iel.obj = obj;
                iel.name = surfaceShaderName;
                iel.node = materialNode;
                mayaScene->interactiveUpdateMap[mayaScene->interactiveUpdateMap.size()] = iel;

                if (getWorldPtr()->getRenderState() == World::RSTATERENDERING)
                    RenderQueueWorker::IPRUpdateCallbacks();
            }
        }

        updateMaterial(materialNode);

        MString objectInstanceName = getObjectInstanceName(obj.get());
        renderer::Assembly *ass = getCreateObjectAssembly(obj);
        MString shadingGroupName = getObjectName(materialNode);
        renderer::ObjectInstance *objInstance = ass->object_instances().get_by_name(objectInstanceName.asChar());

        MString slotName = MString("slot") + sgId;
        objInstance->get_front_material_mappings().insert(slotName.asChar(), shadingGroupName.asChar());
        objInstance->get_back_material_mappings().insert(slotName.asChar(), shadingGroupName.asChar());
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
