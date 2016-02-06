
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

// appleseed-maya headers.
#include "swatchesrenderer/appleseedswatchrenderer.h"
#include "threads/renderqueueworker.h"
#include "utilities/logging.h"
#include "appleseedrenderer.h"
#include "mayascene.h"
#include "mayatoworld.h"

// Maya headers.
#include <maya/MSceneMessage.h>
#include <maya/MGlobal.h>

static MCallbackId timerCallbackId = 0;
static MCallbackId beforeExitId = 0;

MCallbackId MayaToWorld::afterNewCallbackId;

void *getObjPtr(MString name)
{
    if (worldPointer != 0)
        return getWorldPtr()->getObjPtr(name);
    return 0;
}

static void addObjectPtr(MString name, void *ptr)
{
    if (worldPointer != 0)
        worldPointer->addObjectPtr(name, ptr);
}

void deleteWorld()
{
    if (worldPointer != 0)
        delete worldPointer;
    worldPointer = 0;
}

void defineWorld()
{
    deleteWorld();
    worldPointer = new MayaToWorld();
}

MayaToWorld *getWorldPtr()
{
    return worldPointer;
}

void MayaToWorld::beforeExitCallback(void *ptr)
{
    if (timerCallbackId != 0)
        MTimerMessage::removeCallback(timerCallbackId);
    if (MayaToWorld::afterNewCallbackId != 0)
        MSceneMessage::removeCallback(MayaToWorld::afterNewCallbackId);
    if (beforeExitId != 0)
        MSceneMessage::removeCallback(beforeExitId);
    getWorldPtr()->cleanUp();
}

MayaToWorld::MayaToWorld()
{
    internalScaleFactor = 1.0f;
    rendererScaleFactor = 1.0f;
    scaleFactor = 1.0f;
    sceneScale = 1.0f;
    _canDoIPR = false;

    MStatus stat;

    // in batch mode we do not need any renderView callbacks, and timer callbacks do not work anyway in batch
    if (MGlobal::mayaState() != MGlobal::kBatch)
        timerCallbackId = MTimerMessage::addTimerCallback(0.001, RenderQueueWorker::renderQueueWorkerTimerCallback);
    MayaToWorld::afterNewCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, MayaToWorld::callAfterOpenCallback, 0, &stat);

    initialize();
    renderType = RTYPENONE;
    renderState = RSTATENONE;
    worldScenePtr.reset();
    worldRendererPtr.reset();
    worldRenderGlobalsPtr.reset();
    defineGlobalConversionMatrix();
}

MayaToWorld::~MayaToWorld()
{
    MayaToWorld::beforeExitCallback(0);
}

void MayaToWorld::initializeScene()
{
    getWorldPtr()->worldScenePtr.reset(new MayaScene());
}

void MayaToWorld::initializeRenderGlobals()
{
    getWorldPtr()->worldRenderGlobalsPtr.reset(new RenderGlobals());
}

void MayaToWorld::initializeRenderer()
{
    getWorldPtr()->worldRendererPtr.reset(new AppleseedRenderer());
}

void MayaToWorld::initializeRenderEnvironment()
{
    this->initializeRenderGlobals();
    this->initializeScene();
    this->initializeRenderer();
}

void MayaToWorld::defineGlobalConversionMatrix()
{
    globalConversionMatrix.setToIdentity();
    MMatrix scaleMatrix;
    scaleMatrix.setToIdentity();
    this->internalUnit = MDistance::internalUnit();
    this->internalAxis = MGlobal::isYAxisUp() ? YUp : ZUp;
    setRendererUnit();
    setRendererAxis();

    internalScaleFactor = 1.0f; // internal in mm
    rendererScaleFactor = 1.0f; // external in mm

    this->scaleFactor = 1.0f;

    switch (this->internalUnit)
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

    switch (this->rendererUnit)
    {
    case MDistance::kCentimeters:
        rendererScaleFactor = 10.0f;
        break;
    case MDistance::kFeet:
        rendererScaleFactor = 304.8f;
        break;
    case MDistance::kInches:
        rendererScaleFactor = 25.4f;
        break;
    case MDistance::kKilometers:
        rendererScaleFactor = 1000000.f;
        break;
    case MDistance::kMeters:
        rendererScaleFactor = 1000.f;
        break;
    case MDistance::kMillimeters:
        rendererScaleFactor = 1.f;
        break;
    case MDistance::kMiles:
        rendererScaleFactor = 1609344.f;
        break;
    case MDistance::kYards:
        rendererScaleFactor = 914.4f;
        break;
    };

    scaleFactor = internalScaleFactor / rendererScaleFactor * sceneScale;
    scaleMatrix[0][0] = scaleMatrix[1][1] = scaleMatrix[2][2] = scaleFactor;

    scaleMatrix = scaleMatrix * sceneScaleMatrix;

    MMatrix YtoZ;
    YtoZ.setToIdentity();
    YtoZ[0][0] = 1;
    YtoZ[0][1] = 0;
    YtoZ[0][2] = 0;
    YtoZ[0][3] = 0;

    YtoZ[1][0] = 0;
    YtoZ[1][1] = 0;
    YtoZ[1][2] = 1;
    YtoZ[1][3] = 0;

    YtoZ[2][0] = 0;
    YtoZ[2][1] = -1;
    YtoZ[2][2] = 0;
    YtoZ[2][3] = 0;

    YtoZ[3][0] = 0;
    YtoZ[3][1] = 0;
    YtoZ[3][2] = 0;
    YtoZ[3][3] = 1;

    if ((this->internalAxis == YUp) && (this->rendererAxis == ZUp))
    {
        globalConversionMatrix = YtoZ;
    }

    globalConversionMatrix *= scaleMatrix;
}

void MayaToWorld::cleanUp()
{
    AppleseedSwatchRenderer * appleSwRndr = (AppleseedSwatchRenderer *)this->getObjPtr("appleseedSwatchesRenderer");
    if (appleSwRndr)
        delete appleSwRndr;
}

void MayaToWorld::cleanUpAfterRender()
{
    // After a normal rendering we do not need the Maya scene data any more. Remove it to save memory.
    getWorldPtr()->worldScenePtr.reset();
}

void MayaToWorld::initialize()
{
    std::string oslShaderPath = (getRendererHome() + "shaders").asChar();
    Logging::debug(MString("setting osl shader search path to: ") + oslShaderPath.c_str());
    MString cmd = MString("import renderer.osltools as osl;osl.getOSODirs();");
    MStringArray oslDirs;
    MGlobal::executePythonCommand(cmd, oslDirs, false, false);
    MGlobal::displayInfo(MString("found ") + oslDirs.length() + " osl dirs.");
    for (uint i = 0; i < oslDirs.length(); i++)
    {
        this->shaderSearchPath.append(oslDirs[i].asChar());
    }
    AppleseedSwatchRenderer *appleSwRndr = new AppleseedSwatchRenderer();

    this->addObjectPtr("appleseedSwatchesRenderer", appleSwRndr);
    boost::thread swatchRenderThread(AppleseedSwatchRenderer::startAppleseedSwatchRender, appleSwRndr);
    swatchRenderThread.detach();
}

void MayaToWorld::afterOpenScene()
{
    Logging::debug("MayaToWorld::afterOpenScene");
}

void MayaToWorld::afterNewScene()
{
    Logging::debug("MayaToWorld::afterNewScene");
}

void MayaToWorld::callAfterOpenCallback(void *)
{
    getWorldPtr()->afterOpenScene();
}

void MayaToWorld::callAfterNewCallback(void *)
{
    getWorldPtr()->afterNewScene();
}

void MayaToWorld::setRendererUnit()
{
    this->rendererUnit = MDistance::kMeters;
}

void MayaToWorld::setRendererAxis()
{
    this->rendererAxis = ZUp;
}
