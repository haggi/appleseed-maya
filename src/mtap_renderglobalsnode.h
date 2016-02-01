
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

#ifndef MTAP_GLOBALS_H
#define MTAP_GLOBALS_H

#include "mayarendernodes/renderglobalsnode.h"

class MayaToAppleseedGlobals : public MayaRenderGlobalsNode
{
  public:
                        MayaToAppleseedGlobals();
    virtual             ~MayaToAppleseedGlobals();

    static  void*       creator();
    static  MStatus     initialize();

    static  MTypeId     id;

  private:
    static    MObject bitdepth;
    static    MObject colorSpace;
    static    MObject clamping;
    static    MObject maxError;
    static    MObject adaptiveQuality;
    static    MObject enable_caustics;
    static    MObject enable_ibl;
    static    MObject enable_dl;
    static    MObject enable_diagnostics;
    static    MObject diffuseDepth;
    static    MObject glossyDepth;
    static    MObject directLightSamples;
    static    MObject environmentSamples;
    static    MObject bsdfSamples;
    static    MObject next_event_estimation;
    static    MObject rr_min_path_length;
    static    MObject max_path_length;
    static    MObject path_tracing_max_path_length;
    static    MObject path_tracing_rr_min_path_length;
    static    MObject photon_tracing_max_path_length;
    static    MObject photon_tracing_rr_min_path_length;
    static    MObject tile_ordering;
    static    MObject sampling_mode;
    static    MObject max_ray_intensity;
    static    MObject assemblySBVH;
    static    MObject pixel_renderer;
    static    MObject frameRendererPasses;

    static    MObject texCacheSize;
    static    MObject lightingEngine;

    static    MObject assemblyExportType;
    static    MObject assemblyPolyTheshold;

    static    MObject environmentColor;
    static    MObject gradientHorizon;
    static    MObject gradientZenit;
    static    MObject environmentMap;
    static    MObject environmentType;
    static    MObject environmentIntensity;
    static    MObject latlongHoShift;
    static    MObject latlongVeShift;

    //sky shader
    static    MObject skyModel;
    static    MObject ground_albedo;
    static    MObject horizon_shift;
    static    MObject luminance_multiplier;
    static    MObject saturation_multiplier;
    static    MObject sun_phi;
    static    MObject sun_theta;
    static    MObject turbidity;
    static    MObject turbidity_max;
    static    MObject turbidity_min;
    static    MObject physicalSun;
    static    MObject physicalSunConnection;
    static    MObject sunTurbidity;
    static    MObject sunExitanceMultiplier;
    static    MObject AOVs;

    //sppm
    static    MObject sppmAlpha;
    static    MObject dl_mode;
    static    MObject env_photons_per_pass;
    static    MObject initial_radius;
    static    MObject light_photons_per_pass;
    static    MObject max_photons_per_estimate;
    static    MObject photons_per_pass;
    static    MObject photon_type;
};

#endif
