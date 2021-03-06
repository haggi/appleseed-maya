
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

#ifndef RENDERGLOBALS_H
#define RENDERGLOBALS_H

// Maya headers.
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MMatrix.h>
#include <maya/MDistance.h>

// Standard headers.
#include <vector>

// Some renderers offer different transform mb steps and geometry deformation steps (mantra)
// So I create this as as global framework option
class MbElement
{
  public:
    enum Type
    {
        MotionBlurXForm,
        MotionBlurGeo,
        MotionBlurBoth,
        MotionBlurNone
    };

    MbElement::Type elementType;
    double time;

    // these are for sorting the mb steps
    bool operator>(const MbElement& other) const
    {
        return time > other.time;
    }

    bool operator<(const MbElement& other) const
    {
        return time < other.time;
    }
};

class RenderGlobals
{
  public:
    MObject renderGlobalsMobject;

    bool doAnimation;
    float currentFrameNumber; // current real frame (with mb steps)
    float currentFrame; // current frame
    float startFrame;
    float endFrame;
    float byFrame;
    std::vector<float> frameList;

    bool inBatch;

    void setResolution(const int w, const int h)
    {
        imgWidth = w;
        imgHeight = h;
    }

    int getWidth() const { return imgWidth; }
    int getHeight() const { return imgHeight; }

    float updateFrameNumber();  // returns the current frame number and incements the currentFrameIndex
    float getFrameNumber();
    bool frameListDone();
    float pixelAspect;

    // sampling
    int minSamples;
    int maxSamples;
    // regular sampling
    int samplesX;
    int samplesY;
    bool adaptiveSampling;

    // filtering
    float filterSize;
    int filterType;
    MString filterTypeString;

    float gamma;

    bool doMb;
    float motionBlurRange;      // float range default = 0.4 ~ 144�
    int motionBlurType;         // Center, FrameStart, FrameEnd
    double mbStartTime;         // frame relative start time e.g. -0.2 frames
    double mbEndTime;           // frame relative end time e.g. 0.2 frames
    double mbLength;            // absolute length of motion blur, e.g. 0.4 frames
    bool doDof;
    int bitdepth;               // 8, 16, 16halfFloat, 32float

    int threads;
    int tilesize;
    int translatorVerbosity;
    int rendererVerbosity;

    // raytracing
    int maxTraceDepth;

    int xftimesamples;
    int geotimesamples;
    std::vector<MbElement> mbElementList;
    MbElement currentMbElement; // contains type and relative time
    int currentMbStep; // currend mb step id 0 - x
    bool isMbStartStep();

    bool createDefaultLight;

    MString basePath;
    public: MString imageOutputFile; // complete path to current image file

    MString preFrameScript;
    MString postFrameScript;
    MString preRenderLayerScript;
    MString postRenderLayerScript;

    void setUseRenderRegion(bool useRegion)
    {
        useRenderRegion = useRegion;
        // It is possible that Maya keeps old render region values if the image size changes.
        // If the region is outside the frame we disable region rendering.
        if (regionLeft > imgWidth ||
            regionRight > imgWidth ||
            regionBottom > imgHeight ||
            regionTop > imgHeight)
            useRenderRegion = false;
    };

    bool getUseRenderRegion() const { return useRenderRegion; }

    void getRenderRegion(int& left, int& bottom, int& right, int& top) const
    {
        left = regionLeft;
        bottom = regionBottom;
        right = regionRight;
        top = regionTop;
    }

    bool detectShapeDeform;
    bool exportSceneFile;
    MString exportSceneFileName;

    bool useSunLightConnection;

    MMatrix globalConversionMatrix; // for default unit conversion e.g. centimeter to meter
    MMatrix sceneScaleMatrix; // user defined scene scale
    float scaleFactor;

    float sceneScale;
    MString optimizedTexturePath;
    bool useOptimizedTextures;

    RenderGlobals();

    void getMbSteps();
    bool isTransformStep();
    bool isDeformStep();
    void getImageName();

  private:
    int     imgWidth;
    int     imgHeight;
    int     currentFrameIndex;
    bool    useRenderRegion;
    int     regionLeft;
    int     regionRight;
    int     regionBottom;
    int     regionTop;

    MString imagePath;
    MString imageName;
    int     imageFormatId;
    MString imageFormatString;

    void getDefaultGlobals();
};

#endif  // !RENDERGLOBALS_H
