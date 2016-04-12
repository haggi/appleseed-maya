
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

#ifndef SHADERS_ASDISNEYMATERIAL_H
#define SHADERS_ASDISNEYMATERIAL_H

#include <maya/MPxNode.h>
#include <maya/MTypeId.h>

#ifdef HAS_OVERRIDE
#include "asdisneymaterialoverride.h"
#endif

// Plugin asDisneyMaterial Shader Class //


// This class will create a new shader. Shaders are custom dependency
// graph objects so we will derive this class from the basic DG node
// type MPxNode
//

class asDisneyMaterial : public MPxNode
{
  public:
                    asDisneyMaterial();
    virtual         ~asDisneyMaterial();

    static  void *  creator();
    virtual MStatus compute(const MPlug&, MDataBlock&);
    static  MStatus initialize();


    // postConstructor:
    // The postConstructor method allows us to call MPxNode member
    // functions during initialization. Internally maya creates two
    // objects when a user defined node is created, the internal MObject
    // and the user derived object. The association between the these
    // two objects is not made until after the MPxNode constructor is
    // called. This implies that no MPxNode member function can be called
    // from the MPxNode constructor. The postConstructor will get called
    // immediately after the constructor when it is safe to call any
    // MPxNode member function.
    //

    virtual void    postConstructor();

    static  MTypeId   id;  // The IFF type id

  protected:
    static MObject  BaseColor;
    static MObject  Subsurface;
    static MObject  Metallic;
    static MObject  Specular;
    static MObject  SpecularTint;
    static MObject  Anisotropic;
    static MObject  Roughness;
    static MObject  Sheen;
    static MObject  SheenTint;
    static MObject  Clearcoat;
    static MObject  ClearcoatGloss;

   static MObject  aTranslucenceCoeff;
   static MObject  aDiffuseReflectivity;
   static MObject  aColor;              // surface color
   static MObject  aIncandescence;
   static MObject  aInTransparency;     // surface transparency
   static MObject  aOutColor;
   static MObject  aOutTransparency;
   static MObject  aNormalCameraX;
   static MObject  aNormalCameraY;
   static MObject  aNormalCameraZ;
   static MObject  aNormalCamera;       // surface normal
   static MObject  aLightDirectionX;
   static MObject  aLightDirectionY;
   static MObject  aLightDirectionZ;
   static MObject  aLightDirection;
   static MObject  aLightIntensityR;
   static MObject  aLightIntensityG;
   static MObject  aLightIntensityB;
   static MObject  aLightIntensity;
   static MObject  aLightAmbient;
   static MObject  aLightDiffuse;
   static MObject  aLightSpecular;
   static MObject   aLightShadowFraction;
   static MObject   aPreShadowIntensity;
   static MObject   aLightBlindData;
   static MObject   aLightData;
};

#endif  // !SHADERS_ASDISNEYMATERIAL_H
