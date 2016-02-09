
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
#include "world.h"

// appleseed-maya headers.
#include "utilities/logging.h"
#include "appleseedrenderer.h"
#include "appleseedswatchrenderer.h"
#include "mayascene.h"
#include "renderqueueworker.h"

// Maya headers.
#include <maya/MGlobal.h>

static World* worldPointer = 0;
static MCallbackId timerCallbackId = 0;

void deleteWorld()
{
    delete worldPointer;
    worldPointer = 0;
}

void defineWorld()
{
    delete worldPointer;
    worldPointer = new World();
}

World* getWorldPtr()
{
    return worldPointer;
}

World::World()
  : mRenderType(RTYPENONE)
  , mRenderState(RSTATENONE)
{
    // in batch mode we do not need any renderView callbacks, and timer callbacks do not work anyway in batch
    if (MGlobal::mayaState() != MGlobal::kBatch)
        timerCallbackId = MTimerMessage::addTimerCallback(0.001, RenderQueueWorker::renderQueueWorkerTimerCallback);

    std::string oslShaderPath = (getRendererHome() + "shaders").asChar();
    Logging::debug(MString("setting osl shader search path to: ") + oslShaderPath.c_str());

    MStringArray oslDirs;
    MGlobal::executePythonCommand("import renderer.osltools as osl; osl.getOSODirs();", oslDirs, false, false);
    MGlobal::displayInfo(MString("Found ") + oslDirs.length() + " OSL directories.");

    for (uint i = 0; i < oslDirs.length(); i++)
        shaderSearchPath.append(oslDirs[i].asChar());

    mSwatchRenderer.reset(new AppleseedSwatchRenderer());
}

World::~World()
{
    if (timerCallbackId != 0)
        MTimerMessage::removeCallback(timerCallbackId);
}

void World::initializeRenderEnvironment()
{
    worldPointer->mRenderGlobals.reset(new RenderGlobals());
    worldPointer->mScene.reset(new MayaScene());
    worldPointer->mRenderer.reset(new AppleseedRenderer());
}

void World::destroyMayaScene()
{
    worldPointer->mScene.reset();
}

void World::setRenderType(RenderType type)
{
    mRenderType = type;
}

World::RenderType World::getRenderType()
{
    return mRenderType;
}

void World::setRenderState(RenderState state)
{
    mRenderState = state;
}

World::RenderState World::getRenderState()
{
    return mRenderState;
}

AppleseedSwatchRenderer* World::getSwatchRenderer()
{
    return mSwatchRenderer.get();
}
