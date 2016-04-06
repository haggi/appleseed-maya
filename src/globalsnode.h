
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

#ifndef GLOBALSNODE_H
#define GLOBALSNODE_H

// appleseed.foundation headers.
#include "foundation/platform/compiler.h"

// Maya headers.
#include <maya/MPxNode.h>
#include <maya/MStringArray.h>
#include <maya/MTypeId.h>

// Forward declarations.
class MStatus;

class GlobalsNode
  : public MPxNode
{
  public:
    static MTypeId id;

    static void* creator();
    static MStatus initialize();

  private:
    struct Attributes
    {
        // System.
        MObject threads;
        MObject translatorVerbosity;
        MObject rendererVerbosity;

        // Adaptive image sampling.
        MObject adaptiveSampling;
        MObject minSamples;
        MObject maxSamples;

        // sampling raster based
        MObject samplesX;
        MObject samplesY;

        // Motion blur.
        MObject doMotionBlur;
        MObject doDof;
        MObject xftimesamples;
        MObject geotimesamples;
        MObject motionBlurRange;
        MObject motionBlurType;

        MObject detectShapeDeform;

        // Pixel filtering.
        MObject filtertype;
        MObject filtersize;
        MObject tilesize;

        MObject gamma;

        MObject basePath;
        MObject imagePath;
        MObject imageName;

        MObject exrDataTypeHalf;
        MObject exrMergeChannels;

        // Raytracing.
        MObject maxTraceDepth;

        // Sun light.
        MObject sunLightConnection;
        MObject useSunLightConnection;

        MObject exportMode;
        MObject exportSceneFileName;
        MObject sceneScale;
        MObject imageFormat;
        MObject optimizedTexturePath;
        MObject useOptimizedTextures;

        MObject bitdepth;
        MObject colorSpace;
        MObject clamping;
        MObject maxError;
        MObject adaptiveQuality;
        MObject enable_caustics;
        MObject enable_ibl;
        MObject enable_dl;
        MObject enable_diagnostics;
        MObject diffuseDepth;
        MObject glossyDepth;
        MObject directLightSamples;
        MObject environmentSamples;
        MObject bsdfSamples;
        MObject next_event_estimation;
        MObject rr_min_path_length;
        MObject max_path_length;
        MObject path_tracing_max_path_length;
        MObject path_tracing_rr_min_path_length;
        MObject photon_tracing_max_path_length;
        MObject photon_tracing_rr_min_path_length;
        MObject tile_ordering;
        MObject sampling_mode;
        MObject max_ray_intensity;
        MObject assemblySBVH;
        MObject pixel_renderer;
        MObject frameRendererPasses;

        MObject texCacheSize;
        MObject lightingEngine;

        MObject assemblyExportType;
        MObject assemblyPolyTheshold;

        // Environment.
        MObject environmentColor;
        MObject gradientHorizon;
        MObject gradientZenit;
        MObject environmentMap;
        MObject environmentType;
        MObject environmentIntensity;
        MObject latlongHoShift;
        MObject latlongVeShift;

        // Physical sky.
        MObject skyModel;
        MObject ground_albedo;
        MObject horizon_shift;
        MObject luminance_multiplier;
        MObject saturation_multiplier;
        MObject sun_phi;
        MObject sun_theta;
        MObject turbidity;
        MObject turbidity_max;
        MObject turbidity_min;
        MObject physicalSun;
        MObject physicalSunConnection;
        MObject sunTurbidity;
        MObject sunExitanceMultiplier;
        MObject AOVs;

        MObject environmentOSL;

        // SPPM.
        MObject sppmAlpha;
        MObject dl_mode;
        MObject env_photons_per_pass;
        MObject initial_radius;
        MObject light_photons_per_pass;
        MObject max_photons_per_estimate;
        MObject photons_per_pass;
        MObject photon_type;
    };

    static Attributes attr;
};

#endif  // !GLOBALSNODE_H
