
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
#include "renderglobals.h"

// appleseed-maya headers.
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "utilities/pystring.h"
#include "utilities/tools.h"

// Maya headers.
#include <maya/MAnimControl.h>
#include <maya/MCommonRenderSettingsData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnRenderLayer.h>
#include <maya/MGlobal.h>
#include <maya/MRenderUtil.h>
#include <maya/MSelectionList.h>

// Standard headers.
#include <algorithm>
#include <cmath>

RenderGlobals::RenderGlobals()
{
    static const char* imageFormats[] = { "png", "exr" };
    currentRenderPass = 0;
    currentRenderPassElementId = 0;
    maxTraceDepth = 4;
    doMb = false;
    doDof = false;
    mbStartTime = 0.0f;
    mbEndTime = 0.0f;
    currentFrame = 0.0f;
    currentFrameNumber = 0.0f;
    motionBlurRange = 0.4f;
    motionBlurType = 0; // center
    xftimesamples = 2;
    geotimesamples = 2;
    createDefaultLight = false;
    exportSceneFile = false;
    adaptiveSampling = false;
    imageName = "";
    basePath = "";
    exportSceneFileName = "";
    useSunLightConnection = false;
    imagePath = "";
    imageFormatString = imageFormats[getEnumInt("imageFormat", MFnDependencyNode(getRenderGlobalsNode()))];
    currentFrameIndex = 0;
    sceneScale = 1.0f;
    filterSize = 3.0f;

    getDefaultGlobals();

    float internalScaleFactor = 1.0f;   // in mm

    switch (MDistance::internalUnit())
    {
      case MDistance::kCentimeters:
        internalScaleFactor = 10.0f;
        break;
      case MDistance::kFeet:
        internalScaleFactor = 304.8f;
        break;
      case MDistance::kInches:
        internalScaleFactor = 25.4f;
        break;
      case MDistance::kKilometers:
        internalScaleFactor = 1000000.f;
        break;
      case MDistance::kMeters:
        internalScaleFactor = 1000.f;
        break;
      case MDistance::kMillimeters:
        internalScaleFactor = 1.f;
        break;
      case MDistance::kMiles:
        internalScaleFactor = 1609344.f;
        break;
      case MDistance::kYards:
        internalScaleFactor = 914.4f;
        break;
    };

    scaleFactor = internalScaleFactor / 1000.0f * sceneScale;

    MMatrix scaleMatrix;
    scaleMatrix.setToIdentity();
    scaleMatrix[0][0] = scaleFactor;
    scaleMatrix[1][1] = scaleFactor;
    scaleMatrix[2][2] = scaleFactor;

    globalConversionMatrix = scaleMatrix * sceneScaleMatrix;
}

bool RenderGlobals::frameListDone()
{
    return currentFrameIndex >= frameList.size();
}

float RenderGlobals::getFrameNumber()
{
    return currentFrameNumber;
}

float RenderGlobals::updateFrameNumber()
{
    if (currentFrameIndex < frameList.size())
    {
        currentFrameNumber = frameList[currentFrameIndex++];
        return currentFrameNumber;
    }
    else
    {
        return frameList.back();
    }
}

void RenderGlobals::getImageName()
{
    const double fn = getFrameNumber();
    MCommonRenderSettingsData data;
    MRenderUtil::getCommonRenderSettings(data);
    MObject renderLayer = MFnRenderLayer::currentLayer();
    MString ext = imageFormatString.toLowerCase();
    imageOutputFile = data.getImageName(data.kFullPathImage, fn, imageName, MString(""), ext, renderLayer);
}

bool RenderGlobals::isDeformStep()
{
    return ((currentMbElement.elementType == MbElement::MotionBlurGeo) || (currentMbElement.elementType == MbElement::MotionBlurBoth));
}

bool RenderGlobals::isTransformStep()
{
    return ((currentMbElement.elementType == MbElement::MotionBlurXForm) || (currentMbElement.elementType == MbElement::MotionBlurBoth));
}

// If motionblur is turned on, I need to evaluate the instances/geometry serveral times.
// To know at which point what type of export deform/transform is needed, I create a list of
// times that will be added to the current render frame e.g. frame 101 + mbStep1 (.20) = 101.20
void RenderGlobals::getMbSteps()
{
    mbElementList.clear();

    // if no motionblur, I add 0.0 to the list as dummy
    if (!doMb)
    {
        MbElement element;
        element.elementType = MbElement::MotionBlurBoth;
        element.m_time = 0.0;
        mbElementList.push_back(element);
        return;
    }

    // todo: make motion blur calculations time-dependent instead of frame-dependent.

    const double shutterDist = motionBlurRange;

    // get mb type
    // 0 = leading blur --> blur ends at frame
    double startStep = 0.0;
    switch (motionBlurType)
    {
      case 0: // center blur
        startStep = -shutterDist / 2.0;
        break;
      case 1: // frame start
        startStep = 0.0;
        break;
      case 2: // frame end
        startStep = -shutterDist;
        break;
      default:
        startStep = -shutterDist;
        break;
    };

    mbStartTime = startStep;
    mbEndTime = shutterDist - std::abs(startStep);
    mbLength = shutterDist;

    if (xftimesamples > 1)
    {
        const double xfStepSize = shutterDist / (xftimesamples - 1);
        double mbStepValue = startStep;
        for (int i = 0; i < xftimesamples; i++)
        {
            MbElement element;
            element.elementType = MbElement::MotionBlurXForm;
            element.m_time = mbStepValue;
            mbElementList.push_back(element);
            mbStepValue += xfStepSize;
        }
    }

    if (geotimesamples > 1)
    {
        const double geoStepSize = shutterDist / (geotimesamples - 1);
        double mbStepValue = startStep;
        for (int i = 0; i < geotimesamples; i++)
        {
            MbElement element;
            element.elementType = MbElement::MotionBlurGeo;
            element.m_time = mbStepValue;
            mbElementList.push_back(element);
            mbStepValue += geoStepSize;
        }
    }

    std::sort(mbElementList.begin(), mbElementList.end());

    // testing this is for non mb objects or changing topology objects
    MbElement element;
    element.elementType = MbElement::MotionBlurNone;
    element.m_time = 0;
    mbElementList.push_back(element);
}

bool RenderGlobals::isMbStartStep()
{
    return currentMbStep == 0;
}

void RenderGlobals::getDefaultGlobals()
{
    MSelectionList defaultGlobals;
    defaultGlobals.add("defaultRenderGlobals");

    if (defaultGlobals.length() == 0)
    {
        Logging::debug("defaultRenderGlobals not found. Stopping.");
        return;
    }

    MCommonRenderSettingsData data;
    MRenderUtil::getCommonRenderSettings(data);

    MObject node;
    defaultGlobals.getDependNode(0, node);
    MFnDependencyNode fnRenderGlobals(node);

    MTime tv = MAnimControl::currentTime();
    currentFrameNumber = (float)(tv.value());
    tv = data.frameStart;
    startFrame = (float)(tv.value());
    tv = data.frameEnd;
    endFrame = (float)(tv.value());
    byFrame = data.frameBy;

    // check if we are in a batch render mode or if we are rendering from UI
    if (MGlobal::mayaState() == MGlobal::kBatch)
    {
        inBatch = true;
        if (data.isAnimated())
        {
            Logging::debug(MString("animation on, rendering frame sequence from ") + startFrame + " to " + endFrame + " by " + byFrame);
            // these are the frames that are supposed to be rendered in batch mode
            doAnimation = true;
            for (double frame = startFrame; frame <= endFrame; frame += byFrame)
                frameList.push_back((float)frame);
        }
        else
        {
            Logging::debug(MString("animation off, rendering current frame"));
            frameList.push_back(currentFrameNumber);
            doAnimation = false;
        }
    }
    else
    {
        // we are rendering from the UI so only render the current frame
        inBatch = false;
        frameList.push_back(currentFrameNumber);
        doAnimation = false; // at the moment, if rendering comes from UI dont do animation
    }

    imgHeight = data.height;
    imgWidth = data.width;
    pixelAspect = data.pixelAspectRatio;

    regionLeft = 0;
    regionRight = imgWidth;
    regionBottom = 0;
    regionTop = imgHeight;

    regionLeft = getIntAttr("left", fnRenderGlobals, 0);
    regionRight = getIntAttr("rght", fnRenderGlobals, imgWidth);
    regionBottom = getIntAttr("bot", fnRenderGlobals, 0);
    regionTop = getIntAttr("top", fnRenderGlobals, imgHeight);

    getBool(MString("enableDefaultLight"), fnRenderGlobals, createDefaultLight);

    getString(MString("preRenderMel"), fnRenderGlobals, preFrameScript);
    getString(MString("postRenderMel"), fnRenderGlobals, postFrameScript);
    getString(MString("preRenderLayerMel"), fnRenderGlobals, preRenderLayerScript);
    getString(MString("postRenderLayerMel"), fnRenderGlobals, postRenderLayerScript);

    MFnDependencyNode depFn(getRenderGlobalsNode());
    maxTraceDepth = getIntAttr("maxTraceDepth", depFn, 4);
    doMb = getBoolAttr("doMotionBlur", depFn, false);
    doDof = getBoolAttr("doDof", depFn, false);
    motionBlurRange = getFloatAttr("motionBlurRange", depFn, 0.4f);
    motionBlurType = 0; // center
    xftimesamples = getIntAttr("xftimesamples", depFn, 2);
    geotimesamples = getIntAttr("geotimesamples", depFn, 2);
    createDefaultLight = false;
    exportSceneFile = getBoolAttr("exportSceneFile", depFn, false);
    adaptiveSampling = getBoolAttr("adaptiveSampling", depFn, false);
    imageName = getStringAttr("imageName", depFn, "");
    basePath = getStringAttr("basePath", depFn, "");
    exportSceneFileName = getStringAttr("exportSceneFileName", depFn, "");
    imagePath = getStringAttr("imagePath", depFn, "");
    threads = getIntAttr("threads", depFn, 4);
    translatorVerbosity = getEnumInt("translatorVerbosity", depFn);
    rendererVerbosity = getEnumInt("rendererVerbosity", depFn);
    useSunLightConnection = getBoolAttr("useSunLightConnection", depFn, false);
    tilesize = getIntAttr("tileSize", depFn, 64);
    sceneScale = getFloatAttr("sceneScale", depFn, 1.0f);
    filterSize = getFloatAttr("filterSize", depFn, 3.0f);
}
