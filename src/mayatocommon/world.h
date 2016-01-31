
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

#ifndef MAYATO_WORLD_H
#define MAYATO_WORLD_H

#include <vector>
#include <memory>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MImage.h>
#include <maya/MTimerMessage.h>
#include <maya/MDistance.h>
#include "utilities/tools.h"
#include "rendering/renderer.h"

class MayaScene;
class RenderGlobals;

class MayaToWorld
{
  public:
    MayaToWorld();
    virtual ~MayaToWorld();

    enum WorldRenderType
    {
        RTYPENONE = 0,
        SWATCHRENDER,
        UIRENDER,
        BATCHRENDER,
        IPRRENDER
    };

    enum WorldRenderState
    {
        RSTATENONE = 0,
        RSTATERENDERING,
        RSTATESWATCHRENDERING,
        RSTATESTOPPED,
        RSTATEERROR,
        RSTATEDONE,
        RSTATETRANSLATING
    };

    WorldRenderType renderType;
    WorldRenderState renderState;

    enum RendererUpAxis
    {
        XUp,
        YUp,
        ZUp
    };

    MDistance::Unit internalUnit;
    MDistance::Unit rendererUnit;
    RendererUpAxis internalAxis;
    RendererUpAxis rendererAxis;

    float internalScaleFactor;
    float rendererScaleFactor;
    float toMillimeters(float mm);
    MMatrix globalConversionMatrix; // for default unit conversion e.g. centimeter to meter
    MMatrix sceneScaleMatrix; // user defined scene scale
    float scaleFactor;
    float sceneScale;

    void defineGlobalConversionMatrix();

    virtual void setRendererUnit();
    virtual void setRendererAxis();

    sharedPtr<MayaScene> worldScenePtr;
    sharedPtr<Renderer> worldRendererPtr;
    sharedPtr<RenderGlobals> worldRenderGlobalsPtr;

    MStringArray objectNames;
    std::vector<void *> objectPtr;
    MImage previousRenderedImage;
    bool _canDoIPR;
    bool canDoIPR(){ return _canDoIPR; }
    void setCanDoIPR(bool yesOrNo) { _canDoIPR = yesOrNo; }

    MStringArray shaderSearchPath;

    void *getObjPtr(MString name)
    {
        for (uint i = 0; i < objectNames.length(); i++)
            if (objectNames[i] == name)
                return objectPtr[i];
        return 0;
    }

    void addObjectPtr(MString name, void *ptr)
    {
        objectNames.append(name);
        objectPtr.push_back(ptr);
    }

    void initialize();
    void initializeScene();
    void initializeRenderer();
    void initializeRenderGlobals();
    void initializeRenderEnvironment();

    void setRenderType(WorldRenderType type)
    {
        this->renderType = type;
    }

    WorldRenderType getRenderType()
    {
        return this->renderType;
    }

    void setRenderState(WorldRenderState state)
    {
        this->renderState = state;
    }

    WorldRenderState getRenderState()
    {
        return this->renderState;
    }

    void cleanUp();
    void cleanUpAfterRender();
    void afterOpenScene();
    void afterNewScene();

    static void beforeExitCallback(void *);
    static void callAfterOpenCallback(void *);
    static void callAfterNewCallback(void *);
    static MCallbackId afterOpenCallbackId;
    static MCallbackId afterNewCallbackId;
};

void *getObjPtr(MString name);
static void addObjectPtr(MString name, void *ptr);

void deleteWorld();
void defineWorld();
MayaToWorld *getWorldPtr();

static MayaToWorld *worldPointer = 0;

struct CmdArgs
{
    CmdArgs()
    {
        userDataInt = 0;
        userEvent = -1;
        userDataFloat = 0.0;

        MFnDependencyNode defaultGlobals(objectFromName("defaultResolution"));
        width = defaultGlobals.findPlug("width").asInt();
        height = defaultGlobals.findPlug("height").asInt();
        renderType = MayaToWorld::UIRENDER;
        useRenderRegion = false;
    }

    int width;
    int height;
    bool useRenderRegion;
    MDagPath cameraDagPath;
    MayaToWorld::WorldRenderType renderType;
    int userDataInt;
    int userEvent;
    double userDataFloat;
    MString userDataString;
};

#endif // !MAYATO_WORLD_H
