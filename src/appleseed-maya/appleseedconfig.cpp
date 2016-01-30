
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

#include "appleseed.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include <maya/MStringArray.h>
#include "renderglobals.h"
#include "world.h"

using namespace AppleRender;

const char *lightingEngines[] = {"pt", "drt", "sppm"};
const char *bucketOrders[] = { "linear", "spiral", "hilbert", "random" };
const char *photonTypes[] = { "mono", "poly" };
const char *dlTypes[] = { "rt", "sppm", "off" };

// all renderengines get the same values from render globals
void AppleseedRenderer::addRenderParams(asr::ParamArray& paramArray)
{
    MFnDependencyNode renderGlobalsFn(getRenderGlobalsNode());
    MString lightingEngine = lightingEngines[getEnumInt("lightingEngine", renderGlobalsFn)];
    MString bucketOrder = bucketOrders[getEnumInt("tile_ordering", renderGlobalsFn)];
    MString photonType = photonTypes[getEnumInt("photon_type", renderGlobalsFn)];
    MString dlType = dlTypes[getEnumInt("dl_mode", renderGlobalsFn)];
    sharedPtr<RenderGlobals> renderGlobals = MayaTo::getWorldPtr()->worldRenderGlobalsPtr;

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
    sharedPtr<RenderGlobals> renderGlobals = MayaTo::getWorldPtr()->worldRenderGlobalsPtr;

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
