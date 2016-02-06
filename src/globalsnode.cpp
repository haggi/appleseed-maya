
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
#include "globalsnode.h"

// appleseed.foundation headers.
#include "foundation/platform/system.h"

// Maya headers.
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

MTypeId GlobalsNode::id(0x0011CF40);

GlobalsNode::Attributes GlobalsNode::attr;

void* GlobalsNode::creator()
{
    return new GlobalsNode();
}

MStatus GlobalsNode::initialize()
{
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;
    MFnGenericAttribute gAttr;
    MFnEnumAttribute eAttr;
    MFnMessageAttribute mAttr;
    MStatus stat = MStatus::kSuccess;

    attr.filtertype = eAttr.create("filtertype", "filtertype", 0, &stat);
    CHECK_MSTATUS(addAttribute(attr.filtertype));

    attr.imageFormat = eAttr.create("imageFormat", "imageFormat", 0, &stat);
    CHECK_MSTATUS(addAttribute(attr.imageFormat));

    attr.sceneScale = nAttr.create("sceneScale", "sceneScale",  MFnNumericData::kFloat, 1.0f);
    CHECK_MSTATUS(addAttribute(attr.sceneScale));

    attr.useSunLightConnection = nAttr.create("useSunLightConnection", "useSunLightConnection",  MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.useSunLightConnection));

    attr.exportSceneFile = nAttr.create("exportSceneFile", "exportSceneFile",  MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.exportSceneFile));

    attr.exportSceneFileName = tAttr.create("exportSceneFileName", "exportSceneFileName",  MFnNumericData::kString);
    tAttr.setUsedAsFilename(true);
    CHECK_MSTATUS(addAttribute(attr.exportSceneFileName));

    // sampling adaptive
    attr.minSamples = nAttr.create("minSamples", "minSamples", MFnNumericData::kInt, 1);
    CHECK_MSTATUS(addAttribute(attr.minSamples));
    attr.maxSamples = nAttr.create("maxSamples", "maxSamples", MFnNumericData::kInt, 16);
    CHECK_MSTATUS(addAttribute(attr.maxSamples));

    // sampling raster based
    attr.samplesX = nAttr.create("samplesX", "samplesX", MFnNumericData::kInt, 3);
    CHECK_MSTATUS(addAttribute(attr.samplesX));
    attr.samplesY = nAttr.create("samplesY", "samplesY", MFnNumericData::kInt, 3);
    CHECK_MSTATUS(addAttribute(attr.samplesY));

    attr.doMotionBlur = nAttr.create("doMotionBlur", "doMotionBlur", MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.doMotionBlur));
    attr.motionBlurRange = nAttr.create("motionBlurRange", "motionBlurRange", MFnNumericData::kFloat, 0.4);
    CHECK_MSTATUS(addAttribute(attr.motionBlurRange));

    attr.motionBlurType = eAttr.create("motionBlurType", "motionBlurType", 0, &stat);
    stat = eAttr.addField("Center", 0);
    stat = eAttr.addField("FrameStart", 1);
    stat = eAttr.addField("FrameEnd", 2);
    CHECK_MSTATUS(addAttribute(attr.motionBlurType));

    attr.doDof = nAttr.create("doDof", "doDof", MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.doDof));

    attr.xftimesamples = nAttr.create("xftimesamples", "xftimesamples", MFnNumericData::kInt, 2);
    CHECK_MSTATUS(addAttribute(attr.xftimesamples));
    attr.geotimesamples = nAttr.create("geotimesamples", "geotimesamples", MFnNumericData::kInt, 2);
    CHECK_MSTATUS(addAttribute(attr.geotimesamples));

    attr.threads = nAttr.create("threads", "threads", MFnNumericData::kInt, static_cast<int>(foundation::System::get_logical_cpu_core_count()));
    nAttr.setMin(1);
    CHECK_MSTATUS(addAttribute(attr.threads));

    attr.translatorVerbosity = eAttr.create("translatorVerbosity", "translatorVerbosity", 2, &stat);
    stat = eAttr.addField("Info", 0);
    stat = eAttr.addField("Error", 1);
    stat = eAttr.addField("Warning", 2);
    stat = eAttr.addField("Progress", 3);
    stat = eAttr.addField("Debug", 4);
    stat = eAttr.addField("None", 5);
    CHECK_MSTATUS(addAttribute(attr.translatorVerbosity));

    attr.rendererVerbosity = nAttr.create("rendererVerbosity", "rendererVerbosity", MFnNumericData::kInt, 2);
    CHECK_MSTATUS(addAttribute(attr.rendererVerbosity));

    attr.detectShapeDeform = nAttr.create("detectShapeDeform", "detectShapeDeform", MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(addAttribute(attr.detectShapeDeform));

    attr.filtersize = nAttr.create("filtersize", "filtersize", MFnNumericData::kInt, 3);
    CHECK_MSTATUS(addAttribute(attr.filtersize));

    attr.tilesize = nAttr.create("tilesize", "tilesize", MFnNumericData::kInt, 64);
    CHECK_MSTATUS(addAttribute(attr.tilesize));

    attr.basePath = tAttr.create("basePath", "basePath", MFnNumericData::kString);
    CHECK_MSTATUS(addAttribute(attr.basePath));

    attr.imagePath = tAttr.create("imagePath", "imagePath", MFnNumericData::kString);
    CHECK_MSTATUS(addAttribute(attr.imagePath));

    attr.imageName = tAttr.create("imageName", "imageName", MFnNumericData::kString);
    CHECK_MSTATUS(addAttribute(attr.imageName));

    attr.gamma = nAttr.create("gamma", "gamma", MFnNumericData::kFloat, 1.0f);
    CHECK_MSTATUS(addAttribute(attr.gamma));

    attr.maxTraceDepth = nAttr.create("maxTraceDepth", "maxTraceDepth", MFnNumericData::kInt, 4);
    CHECK_MSTATUS(addAttribute(attr.maxTraceDepth));

    attr.sunLightConnection = mAttr.create("sunLightConnection", "sunLightConnection");
    CHECK_MSTATUS(addAttribute(attr.sunLightConnection));

    attr.adaptiveSampling = nAttr.create("adaptiveSampling", "adaptiveSampling", MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.adaptiveSampling));

    attr.optimizedTexturePath = tAttr.create("optimizedTexturePath", "optimizedTexturePath",  MFnNumericData::kString);
    tAttr.setUsedAsFilename(true);
    CHECK_MSTATUS(addAttribute(attr.optimizedTexturePath));

    attr.useOptimizedTextures = nAttr.create("useOptimizedTextures", "useOptimizedTextures", MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.useOptimizedTextures));

    attr.exrDataTypeHalf = nAttr.create("exrDataTypeHalf", "exrDataTypeHalf", MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.exrDataTypeHalf));

    attr.exrMergeChannels = nAttr.create("exrMergeChannels", "exrMergeChannels", MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(addAttribute(attr.exrMergeChannels));

    attr.sampling_mode = eAttr.create("sampling_mode", "sampling_mode", 0, &stat);
    stat = eAttr.addField("qmc", 0);
    stat = eAttr.addField("rng", 1);
    CHECK_MSTATUS(addAttribute(attr.sampling_mode));

    attr.tile_ordering = eAttr.create("tile_ordering", "tile_ordering", 2, &stat);
    stat = eAttr.addField("linear", 0);
    stat = eAttr.addField("spiral", 1);
    stat = eAttr.addField("hilbert", 2);
    stat = eAttr.addField("random", 3);
    CHECK_MSTATUS(addAttribute(attr.tile_ordering));

    attr.bitdepth = eAttr.create("bitdepth", "bitdepth", 3, &stat);
    stat = eAttr.addField("8bit  Integer", 0);
    stat = eAttr.addField("16bit Integer", 1);
    stat = eAttr.addField("32bit Integer", 2);
    stat = eAttr.addField("16bit Float(Half)", 3);
    stat = eAttr.addField("32bit Float", 4);
    stat = eAttr.addField("64bit Double", 5);
    CHECK_MSTATUS(addAttribute(attr.bitdepth));

    attr.pixel_renderer = eAttr.create("pixel_renderer", "pixel_renderer", 0, &stat);
    stat = eAttr.addField("adaptive", 0);
    stat = eAttr.addField("uniform", 1);
    CHECK_MSTATUS(addAttribute(attr.pixel_renderer));

    attr.colorSpace = eAttr.create("colorSpace", "colorSpace", 0, &stat);
    stat = eAttr.addField("linear_rgb", 0);
    stat = eAttr.addField("srgb", 1);
    stat = eAttr.addField("ciexyz", 2);
    CHECK_MSTATUS(addAttribute(attr.colorSpace));

    attr.lightingEngine = eAttr.create("lightingEngine", "lightingEngine", 0, &stat);
    stat = eAttr.addField("Unidirectional Path tracing", 0);
    stat = eAttr.addField("Stochastic Progressive Photon Mapping", 1);
    CHECK_MSTATUS(addAttribute(attr.lightingEngine));

    attr.clamping = nAttr.create("clamping", "clamping",  MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.clamping));

    attr.maxError = nAttr.create("maxError", "maxError",  MFnNumericData::kFloat, 0.01f);
    CHECK_MSTATUS(addAttribute(attr.maxError));

    attr.adaptiveQuality = nAttr.create("adaptiveQuality", "adaptiveQuality",  MFnNumericData::kFloat, 3.0f);
    CHECK_MSTATUS(addAttribute(attr.adaptiveQuality));

    attr.enable_caustics = nAttr.create("enable_caustics", "enable_caustics",  MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.enable_caustics));

    attr.enable_dl = nAttr.create("enable_dl", "enable_ibl",  MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(addAttribute(attr.enable_dl));

    attr.enable_diagnostics = nAttr.create("enable_diagnostics", "enable_diagnostics",  MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.enable_diagnostics));

    attr.diffuseDepth = nAttr.create("diffuseDepth", "diffuseDepth",  MFnNumericData::kInt, 4);
    CHECK_MSTATUS(addAttribute(attr.diffuseDepth));

    attr.texCacheSize = nAttr.create("texCacheSize", "texCacheSize",  MFnNumericData::kInt, 512);
    CHECK_MSTATUS(addAttribute(attr.texCacheSize));

    attr.frameRendererPasses = nAttr.create("frameRendererPasses", "frameRendererPasses",  MFnNumericData::kInt, 1);
    CHECK_MSTATUS(addAttribute(attr.frameRendererPasses));

    attr.glossyDepth = nAttr.create("glossyDepth", "glossyDepth",  MFnNumericData::kInt, 4);
    CHECK_MSTATUS(addAttribute(attr.glossyDepth));

    attr.environmentSamples = nAttr.create("environmentSamples", "environmentSamples",  MFnNumericData::kInt, 1);
    CHECK_MSTATUS(addAttribute(attr.environmentSamples));

    attr.bsdfSamples = nAttr.create("bsdfSamples", "bsdfSamples",  MFnNumericData::kInt, 1);
    CHECK_MSTATUS(addAttribute(attr.bsdfSamples));

    attr.next_event_estimation = nAttr.create("next_event_estimation", "next_event_estimation",  MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(addAttribute(attr.next_event_estimation));

    attr.assemblySBVH = nAttr.create("assemblySBVH", "assemblySBVH",  MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.assemblySBVH));

    attr.max_path_length = nAttr.create("max_path_length", "max_path_length", MFnNumericData::kFloat, 8.0f);
    CHECK_MSTATUS(addAttribute(attr.max_path_length));

    attr.rr_min_path_length = nAttr.create("rr_min_path_length", "rr_min_path_length",  MFnNumericData::kFloat, 3.0f);
    CHECK_MSTATUS(addAttribute(attr.rr_min_path_length));

    attr.path_tracing_max_path_length = nAttr.create("path_tracing_max_path_length", "path_tracing_max_path_length", MFnNumericData::kFloat, 0.0f);
    CHECK_MSTATUS(addAttribute(attr.path_tracing_max_path_length));

    attr.path_tracing_rr_min_path_length = nAttr.create("path_tracing_rr_min_path_length", "path_tracing_rr_min_path_length", MFnNumericData::kFloat, 8.0f);
    CHECK_MSTATUS(addAttribute(attr.path_tracing_rr_min_path_length));

    attr.photon_tracing_max_path_length = nAttr.create("photon_tracing_max_path_length", "photon_tracing_max_path_length", MFnNumericData::kFloat, 8.0f);
    CHECK_MSTATUS(addAttribute(attr.photon_tracing_max_path_length));

    attr.photon_tracing_rr_min_path_length = nAttr.create("photon_tracing_rr_min_path_length", "photon_tracing_rr_min_path_length", MFnNumericData::kFloat, 3.0f);
    CHECK_MSTATUS(addAttribute(attr.photon_tracing_rr_min_path_length));

    attr.max_ray_intensity = nAttr.create("max_ray_intensity", "max_ray_intensity",  MFnNumericData::kFloat, 0.0f);
    CHECK_MSTATUS(addAttribute(attr.max_ray_intensity));

    // reduced to auto because we do not need the others (I hope) remove the whole attribute in the next release
    attr.assemblyExportType = eAttr.create("assemblyExportType", "assemblyExportType", 0, &stat);
    stat = eAttr.addField("Auto", 0);
    CHECK_MSTATUS(addAttribute(attr.assemblyExportType));

    attr.assemblyPolyTheshold = nAttr.create("assemblyPolyTheshold", "assemblyPolyTheshold",  MFnNumericData::kInt, 10000);
    CHECK_MSTATUS(addAttribute(attr.assemblyPolyTheshold));

    attr.environmentType = eAttr.create("environmentType", "environmentType", 0, &stat);
    stat = eAttr.addField("Constant", 0);
    stat = eAttr.addField("ConstantHemisphere", 1);
    stat = eAttr.addField("Gradient", 2);
    stat = eAttr.addField("Latitude Longitude", 3);
    stat = eAttr.addField("Mirror Ball", 4);
    stat = eAttr.addField("Physical Sky", 5);
    stat = eAttr.addField("OSL Environment", 6);
    CHECK_MSTATUS(addAttribute(attr.environmentType));

    attr.environmentColor = nAttr.createColor("environmentColor", "environmentColor");
    nAttr.setDefault(0.6f, 0.7f, 0.9f);
    nAttr.setConnectable(false);
    CHECK_MSTATUS(addAttribute(attr.environmentColor));

    attr.gradientHorizon = nAttr.createColor("gradientHorizon", "gradientHorizon");
    nAttr.setDefault(0.8f, 0.8f, 0.9f);
    nAttr.setConnectable(false);
    CHECK_MSTATUS(addAttribute(attr.gradientHorizon));

    attr.gradientZenit = nAttr.createColor("gradientZenit", "gradientZenit");
    nAttr.setDefault(0.2f, 0.3f, 0.6f);
    nAttr.setConnectable(false);
    CHECK_MSTATUS(addAttribute(attr.gradientZenit));

    attr.environmentMap = nAttr.createColor("environmentMap", "environmentMap");
    nAttr.setDefault(0.6f, 0.7f, 0.9f);
    CHECK_MSTATUS(addAttribute(attr.environmentMap));

    attr.environmentIntensity = nAttr.create("environmentIntensity", "environmentIntensity",  MFnNumericData::kFloat, 1.0f);
    nAttr.setConnectable(false);
    CHECK_MSTATUS(addAttribute(attr.environmentIntensity));

    attr.directLightSamples = nAttr.create("directLightSamples", "directLightSamples",  MFnNumericData::kFloat, 1.0f);
    CHECK_MSTATUS(addAttribute(attr.directLightSamples));

    attr.latlongHoShift = nAttr.create("latlongHoShift", "latlongHoShift",  MFnNumericData::kFloat, .0f);
    CHECK_MSTATUS(addAttribute(attr.latlongHoShift));

    attr.latlongVeShift = nAttr.create("latlongVeShift", "latlongVeShift",  MFnNumericData::kFloat, .0f);
    CHECK_MSTATUS(addAttribute(attr.latlongVeShift));

    attr.AOVs = mAttr.create("AOVs", "AOVs");
    mAttr.setArray(true);
    mAttr.indexMatters();
    CHECK_MSTATUS(addAttribute(attr.AOVs));

    attr.ground_albedo = nAttr.create("ground_albedo", "ground_albedo",  MFnNumericData::kFloat, .0f);
    CHECK_MSTATUS(addAttribute(attr.ground_albedo));

    attr.horizon_shift = nAttr.create("horizon_shift", "horizon_shift",  MFnNumericData::kFloat, -0.05f);
    CHECK_MSTATUS(addAttribute(attr.horizon_shift));

    attr.luminance_multiplier = nAttr.create("luminance_multiplier", "luminance_multiplier",  MFnNumericData::kFloat, 1.0f);
    CHECK_MSTATUS(addAttribute(attr.luminance_multiplier));

    attr.saturation_multiplier = nAttr.create("saturation_multiplier", "saturation_multiplier",  MFnNumericData::kFloat, 1.0f);
    CHECK_MSTATUS(addAttribute(attr.saturation_multiplier));

    attr.sun_phi = nAttr.create("sun_phi", "sun_phi",  MFnNumericData::kFloat, .0f);
    CHECK_MSTATUS(addAttribute(attr.sun_phi));

    attr.sun_theta = nAttr.create("sun_theta", "sun_theta",  MFnNumericData::kFloat, 60.0f);
    CHECK_MSTATUS(addAttribute(attr.sun_theta));

    attr.turbidity = nAttr.create("turbidity", "turbidity",  MFnNumericData::kFloat, 3.0f);
    CHECK_MSTATUS(addAttribute(attr.turbidity));

    attr.turbidity_max = nAttr.create("turbidity_max", "turbidity_max",  MFnNumericData::kFloat, 3.0f);
    CHECK_MSTATUS(addAttribute(attr.turbidity_max));

    attr.turbidity_min = nAttr.create("turbidity_min", "turbidity_min",  MFnNumericData::kFloat, 3.0f);
    CHECK_MSTATUS(addAttribute(attr.turbidity_min));

    attr.skyModel = eAttr.create("skyModel", "skyModel", 0, &stat);
    stat = eAttr.addField("Preetham", 0);
    stat = eAttr.addField("Hosek", 1);
    CHECK_MSTATUS(addAttribute(attr.skyModel));

    attr.physicalSun = nAttr.create("physicalSun", "physicalSun",  MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(addAttribute(attr.physicalSun));

    attr.physicalSunConnection = mAttr.create("physicalSunConnection", "physicalSunConnection");
    CHECK_MSTATUS(addAttribute(attr.physicalSunConnection));

    attr.sunTurbidity = nAttr.create("sunTurbidity", "sunTurbidity",  MFnNumericData::kFloat, 3.0f);
    CHECK_MSTATUS(addAttribute(attr.sunTurbidity));

    attr.sunExitanceMultiplier = nAttr.create("sunExitanceMultiplier", "sunExitanceMultiplier",  MFnNumericData::kFloat, 1.0f);
    CHECK_MSTATUS(addAttribute(attr.sunExitanceMultiplier));

    attr.sppmAlpha = nAttr.create("sppmAlpha", "sppmAlpha",  MFnNumericData::kFloat, .8f);
    CHECK_MSTATUS(addAttribute(attr.sppmAlpha));

    attr.dl_mode = eAttr.create("dl_mode", "dl_mode", 0, &stat);
    stat = eAttr.addField("RT Direct Lighting", 0);
    stat = eAttr.addField("SPPM Direct Lighting", 1);
    stat = eAttr.addField("No Direct Lighting", 2);
    CHECK_MSTATUS(addAttribute(attr.dl_mode));

    attr.env_photons_per_pass = nAttr.create("env_photons_per_pass", "env_photons_per_pass",  MFnNumericData::kInt, 100000);
    CHECK_MSTATUS(addAttribute(attr.env_photons_per_pass));

    attr.initial_radius = nAttr.create("initial_radius", "initial_radius",  MFnNumericData::kFloat, .5f);
    CHECK_MSTATUS(addAttribute(attr.initial_radius));

    attr.light_photons_per_pass = nAttr.create("light_photons_per_pass", "light_photons_per_pass",  MFnNumericData::kInt, 100000);
    CHECK_MSTATUS(addAttribute(attr.light_photons_per_pass));

    attr.max_photons_per_estimate = nAttr.create("max_photons_per_estimate", "max_photons_per_estimate",  MFnNumericData::kInt, 100);
    CHECK_MSTATUS(addAttribute(attr.max_photons_per_estimate));

    attr.photons_per_pass = nAttr.create("photons_per_pass", "photons_per_pass",  MFnNumericData::kInt, 100000);
    CHECK_MSTATUS(addAttribute(attr.photons_per_pass));

    attr.photon_type = eAttr.create("photon_type", "photon_type", 1, &stat);
    stat = eAttr.addField("Monochromatic", 0);
    stat = eAttr.addField("Polychromatic", 1);
    CHECK_MSTATUS(addAttribute(attr.photon_type));

    return stat;
}

void GlobalsNode::postConstructor()
{
    MObject thisObj = thisMObject();

    MPlug imgFormatPlug(thisObj, attr.imageFormat);
    MFnEnumAttribute imgFormatAttribute(imgFormatPlug.attribute());
    imgFormatAttribute.addField("Png", 0);
    imgFormatAttribute.addField("Exr", 1);

    MPlug filtertypePlug(thisObj, attr.filtertype);
    MFnEnumAttribute filtertypeAttribute(filtertypePlug.attribute());
    filtertypeAttribute.addField("Box", 0);
    filtertypeAttribute.addField("Blackman-Harris", 1);
    filtertypeAttribute.addField("Catmull-Rom", 2);
    filtertypeAttribute.addField("Mitchell", 3);
    filtertypeAttribute.addField("Gauss", 4);
    filtertypeAttribute.addField("Triangle", 5);
    filtertypeAttribute.setDefault(0);
}
