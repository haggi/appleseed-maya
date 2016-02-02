
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

#include "renderprocess.h"
#include <maya/MGlobal.h>
#include <maya/MRenderView.h>
#include "mayatoworld.h"
#include "renderglobals.h"
#include "mayascene.h"
#include "utilities/logging.h"
#include "threads/renderqueueworker.h"

void doPreRenderJobs()
{
}

// not sure if this is really the best way to do it.
// the renderer should be able to access all scene element lists and these are creating in parseScene()
// but the preFrameScripts should be called before the whole parsing is done because it is possible that this script
// updates or creates geometry.
void doRenderPreFrameJobs()
{
    getWorldPtr()->worldRendererPtr->preFrame();
}

void doPreFrameJobs()
{
    MString result;
    MGlobal::executeCommand(getWorldPtr()->worldRenderGlobalsPtr->preFrameScript, result, true);
}

void doPostFrameJobs()
{
    MString result;
    MGlobal::executeCommand(getWorldPtr()->worldRenderGlobalsPtr->postFrameScript, result, true);
    getWorldPtr()->worldRendererPtr->postFrame();
}

void doPostRenderJobs()
{
}

void doPrepareFrame()
{
    float currentFrame = getWorldPtr()->worldRenderGlobalsPtr->getFrameNumber();
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;
    Logging::progress(MString("\n========== doPrepareFrame ") + currentFrame + " ==============\n");

    mayaScene->parseScene(); // all lists are cleaned and refilled with the current scene content
    std::vector<boost::shared_ptr<MayaObject> >::iterator oIt;
    for (oIt = mayaScene->camList.begin(); oIt != mayaScene->camList.end(); oIt++)
    {
        boost::shared_ptr<MayaObject> camera = *oIt;
        if (!isCameraRenderable(camera->mobject) && (!(camera->dagPath == mayaScene->uiCamera)))
        {
            Logging::debug(MString("Camera ") + camera->shortName + " is not renderable, skipping.");
            continue;
        }
        Logging::info(MString("Rendering camera ") + camera->shortName);
        if (MGlobal::mayaState() != MGlobal::kBatch)
        {
            MRenderView::setCurrentCamera(camera->dagPath);
        }
    }

    getWorldPtr()->worldRenderGlobalsPtr->getMbSteps();

    if (getWorldPtr()->worldRenderGlobalsPtr->mbElementList.size() == 0)
    {
        Logging::error(MString("no mb steps, something's wrong."));
        return;
    }

    int numMbSteps = (int)getWorldPtr()->worldRenderGlobalsPtr->mbElementList.size();

    for (int mbStepId = 0; mbStepId < numMbSteps; mbStepId++)
    {
        getWorldPtr()->worldRenderGlobalsPtr->currentMbStep = mbStepId;
        getWorldPtr()->worldRenderGlobalsPtr->currentMbElement = getWorldPtr()->worldRenderGlobalsPtr->mbElementList[mbStepId];
        getWorldPtr()->worldRenderGlobalsPtr->currentFrameNumber = (float)(currentFrame + getWorldPtr()->worldRenderGlobalsPtr->mbElementList[mbStepId].m_time);
        bool needView = true;

        // we can have some mb time steps at the same time, e.g. for xform and deform, then we do not need to update the view
        if (mbStepId > 0)
        {
            if (getWorldPtr()->worldRenderGlobalsPtr->mbElementList[mbStepId].m_time == getWorldPtr()->worldRenderGlobalsPtr->mbElementList[mbStepId - 1].m_time)
            {
                needView = false;
            }
        }

        if (needView)
        {
            Logging::debug(MString("doFrameJobs() viewFrame: ") + getWorldPtr()->worldRenderGlobalsPtr->currentFrameNumber);
            MGlobal::viewFrame(getWorldPtr()->worldRenderGlobalsPtr->currentFrameNumber);
        }

        if (getWorldPtr()->worldScenePtr)
            mayaScene->updateScene();
        else
            Logging::error(MString("no maya scene ptr."));

        Logging::info(MString("update scene done"));
        getWorldPtr()->worldRenderGlobalsPtr->currentMbStep++;
    }

    if (MGlobal::mayaState() != MGlobal::kBatch)
        MGlobal::viewFrame(currentFrame);
}

void doFrameJobs()
{
    Logging::debug("doFrameJobs()");
}

void render()
{
}
