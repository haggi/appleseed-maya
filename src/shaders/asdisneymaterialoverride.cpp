
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

#include <maya/MGlobal.h>
#include "asdisneymaterialoverride.h"
#include <maya/MFragmentManager.h>
#include "mayatoworld.h"

MHWRender::MPxSurfaceShadingNodeOverride* asDisneyMaterialOverride::creator(const MObject& obj)
{
    return new asDisneyMaterialOverride(obj);
}

asDisneyMaterialOverride::asDisneyMaterialOverride(const MObject& obj)
: MPxSurfaceShadingNodeOverride(obj)
{
}

asDisneyMaterialOverride::~asDisneyMaterialOverride()
{
}

MHWRender::DrawAPI asDisneyMaterialOverride::supportedDrawAPIs() const
{
    // works in both gl and dx
    return MHWRender::kOpenGL | MHWRender::kDirectX11;
}

MString asDisneyMaterialOverride::fragmentName() const
{
    // Just reuse Maya's lambert surface shader
    return "mayaLambertSurface";
}

void asDisneyMaterialOverride::getCustomMappings(
    MHWRender::MAttributeParameterMappingList& mappings)
{
    // The "color", "transparency" and "incandescence" attributes are all
    // named the same as the corresponding parameters on the fragment so they
    // will map automatically. Need to remap diffuseReflectivity and
    // translucence though
    MHWRender::MAttributeParameterMapping diffuseMapping(
        "diffuse", "diffuseReflectivity", true, true);
    mappings.append(diffuseMapping);

    MHWRender::MAttributeParameterMapping translucenceMapping(
        "translucence", "translucenceCoeff", true, true);
    mappings.append(translucenceMapping);
}

MString asDisneyMaterialOverride::primaryColorParameter() const
{
    // Use the color parameter from the lambert fragment as the primary color
    return "color";
}

MString asDisneyMaterialOverride::transparencyParameter() const
{
    // Use the "transparency" parameter from the lambert fragment for transparency
    return "transparency";
}

MString asDisneyMaterialOverride::bumpAttribute() const
{
    // Use the "normalCamera" attribute to recognize bump connections
    return "normalCamera";
}

//#endif
