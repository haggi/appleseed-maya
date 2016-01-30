
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

#ifndef MT_GLOBALS_H
#define MT_GLOBALS_H

#include <maya/MTypeId.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MPxNode.h>


class MayaRenderGlobalsNode : public MPxNode
{
public:
                        MayaRenderGlobalsNode();
    virtual             ~MayaRenderGlobalsNode();

    virtual MStatus     compute( const MPlug& plug, MDataBlock& data );
    static  void*       creator();
    virtual void        postConstructor();
    static  MStatus     initialize();

    static  MTypeId     id;
    MStringArray    imageFormatList;
    MStringArray    filterTypeList;

private:

    static    MObject adaptiveSampling;
    // sampling adaptive
    static    MObject minSamples;
    static    MObject maxSamples;

    // sampling raster based
    static    MObject samplesX;
    static    MObject samplesY;

    static    MObject doMotionBlur;
    static    MObject doDof;
    static    MObject xftimesamples;
    static    MObject geotimesamples;
    static    MObject motionBlurRange;
    static    MObject motionBlurType;
    //
    static    MObject threads;
    static    MObject translatorVerbosity;
    static    MObject rendererVerbosity;

    static    MObject detectShapeDeform;

    // pixel filtering
    static    MObject filtertype;
    static    MObject filtersize;
    static    MObject tilesize;

    static    MObject gamma;


    static    MObject basePath;
    static    MObject imagePath;
    static    MObject imageName;

    static    MObject exrDataTypeHalf;
    static    MObject exrMergeChannels;

    // raytracing
    static    MObject maxTraceDepth;

    // sun light
    // if a renderer support physical sun or simply sun,
    // you always have only one sun which can be plugged in here.
    static    MObject sunLightConnection;
    static    MObject useSunLightConnection;

    static    MObject exportSceneFile;
    static    MObject exportSceneFileName;
    static    MObject sceneScale;
    static    MObject imageFormat;
    static    MObject optimizedTexturePath;
    static    MObject useOptimizedTextures;

public:
    int      defaultEnumFilterType;

};

#endif
