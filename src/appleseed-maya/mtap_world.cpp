
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

#include "world.h"
#include "mayascenefactory.h"
#include "renderglobalsfactory.h"
#include "utilities/logging.h"
#include <maya/MGlobal.h>
#include "swatchesrenderer/appleseedswatchrenderer.h"
#include "swatchesrenderer/swatchesevent.h"

static Logging logger;

void MayaToWorld::cleanUp()
{
    AppleseedSwatchRenderer * appleSwRndr = (AppleseedSwatchRenderer *)this->getObjPtr("appleseedSwatchesRenderer");
    if (appleSwRndr)
        delete appleSwRndr;
}

void MayaToWorld::cleanUpAfterRender()
{
    // after a normal rendering we do not need the maya scene data any more
    // remove it to save memory
    MayaSceneFactory().deleteMayaScene();
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
    threadObject swatchRenderThread(AppleseedSwatchRenderer::startAppleseedSwatchRender, appleSwRndr);
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
