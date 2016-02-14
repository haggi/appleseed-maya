
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
#include "renderqueueworker.h"

// appleseed-maya headers.
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "appleseedrenderer.h"
#include "mayascene.h"
#include "world.h"

// appleseed.foundation headers.
#include "foundation/platform/thread.h"

// Maya headers.
#include <maya/MDGMessage.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MNodeMessage.h>
#include <maya/MRenderView.h>
#include <maya/MTimerMessage.h>

// Standard headers.
#include <vector>

namespace
{
    boost::thread renderThread;
    concurrent_queue<Event> RenderEventQueue;
}

concurrent_queue<Event>* gEventQueue()
{
    return &RenderEventQueue;
}

namespace
{
    void removeCallbacks()
    {
        if (idleCallbackId != 0)
            MMessage::removeCallback(idleCallbackId);

        if (nodeAddedCallbackId != 0)
            MDGMessage::removeCallback(nodeAddedCallbackId);

        if (nodeRemovedCallbackId != 0)
            MDGMessage::removeCallback(nodeRemovedCallbackId);

        idleCallbackId = 0;
        nodeRemovedCallbackId = 0;
        nodeAddedCallbackId = 0;

        for (std::vector<MCallbackId>::iterator i = nodeCallbacks.begin(); i != nodeCallbacks.end(); i++)
            MMessage::removeCallback(*i);

        nodeCallbacks.clear();
        objIdMap.clear();
        modifiedElementList.clear(); // make sure that the iprFindLeafNodes exits with an empty list
    }

    void computationEventThread()
    {
        bool renderingStarted = false;

        while (true)
        {
            if (getWorldPtr()->getRenderState() == World::RSTATERENDERING)
                renderingStarted = true;

            if (renderingStarted && getWorldPtr()->getRenderState() != World::RSTATERENDERING)
                break;

#ifdef _WIN32
            if (GetAsyncKeyState(VK_ESCAPE) && getWorldPtr()->getRenderState() == World::RSTATERENDERING)
            {
                getWorldPtr()->mRenderer->abortRendering();
                break;
            }
#endif

            foundation::sleep(100);
        }
    }

    void renderThreadProc()
    {
        if (getWorldPtr()->getRenderType() == World::IPRRENDER)
        {
            // the idea is that the renderer waits in IPR mode for an non empty modifiesElementList,
            // it updates the render database with the elements and empties the list which is then free for the next run
            while (getWorldPtr()->getRenderType() == World::IPRRENDER)
            {
                getWorldPtr()->mRenderer->render();

                while (modifiedElementList.empty() &&
                       getWorldPtr()->getRenderType() == World::IPRRENDER &&
                       getWorldPtr()->getRenderState() != World::RSTATESTOPPED)
                    foundation::sleep(100);

                if (getWorldPtr()->getRenderType() != World::IPRRENDER ||
                    getWorldPtr()->getRenderState() == World::RSTATESTOPPED)
                    break;

                getWorldPtr()->mRenderer->interactiveUpdateList = modifiedElementList;
                getWorldPtr()->mRenderer->doInteractiveUpdate();
                modifiedElementList.clear();
            }
        }
        else
        {
            getWorldPtr()->mRenderer->render();
        }

        Event event;
        event.mType = Event::FRAMEDONE;
        RenderEventQueue.push(event);
    }

    void iprWaitForFinish(Event e)
    {
        while (getWorldPtr()->getRenderState() != World::RSTATENONE)
            foundation::sleep(100);

        RenderEventQueue.push(e);
    }

    void doPreFrameJobs()
    {
        MString result;
        MGlobal::executeCommand(getWorldPtr()->mRenderGlobals->preFrameScript, result, true);
    }

    void doPostFrameJobs()
    {
        MString result;
        MGlobal::executeCommand(getWorldPtr()->mRenderGlobals->postFrameScript, result, true);
        getWorldPtr()->mRenderer->postFrame();
    }

    void doPrepareFrame()
    {
        float currentFrame = getWorldPtr()->mRenderGlobals->getFrameNumber();
        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;
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

        getWorldPtr()->mRenderGlobals->getMbSteps();

        if (getWorldPtr()->mRenderGlobals->mbElementList.size() == 0)
        {
            Logging::error(MString("no mb steps, something's wrong."));
            return;
        }

        int numMbSteps = (int)getWorldPtr()->mRenderGlobals->mbElementList.size();

        for (int mbStepId = 0; mbStepId < numMbSteps; mbStepId++)
        {
            getWorldPtr()->mRenderGlobals->currentMbStep = mbStepId;
            getWorldPtr()->mRenderGlobals->currentMbElement = getWorldPtr()->mRenderGlobals->mbElementList[mbStepId];
            getWorldPtr()->mRenderGlobals->currentFrameNumber = (float)(currentFrame + getWorldPtr()->mRenderGlobals->mbElementList[mbStepId].m_time);
            bool needView = true;

            // we can have some mb time steps at the same time, e.g. for xform and deform, then we do not need to update the view
            if (mbStepId > 0)
            {
                if (getWorldPtr()->mRenderGlobals->mbElementList[mbStepId].m_time == getWorldPtr()->mRenderGlobals->mbElementList[mbStepId - 1].m_time)
                {
                    needView = false;
                }
            }

            if (needView)
                MGlobal::viewFrame(getWorldPtr()->mRenderGlobals->currentFrameNumber);

            mayaScene->updateScene();

            getWorldPtr()->mRenderGlobals->currentMbStep++;
        }

        if (MGlobal::mayaState() != MGlobal::kBatch)
            MGlobal::viewFrame(currentFrame);
    }
}

void initRender(Event e)
{
    getWorldPtr()->setRenderType(e.renderType);

    // It is possible that a new IPR rendering is started before another one is completely done, so wait for it.
    if (getWorldPtr()->getRenderType() == World::IPRRENDER &&
        getWorldPtr()->getRenderState() != World::RSTATENONE)
    {
        boost::thread waitThread = boost::thread(iprWaitForFinish, e);
        waitThread.detach();
        return;
    }

    getWorldPtr()->initializeRenderEnvironment();
    getWorldPtr()->mRenderGlobals->setResolution(e.width, e.height);
    getWorldPtr()->mRenderGlobals->setUseRenderRegion(e.useRenderRegion);
    getWorldPtr()->mScene->uiCamera = e.cameraDagPath;
    getWorldPtr()->mRenderer->createProject();

    if (MRenderView::doesRenderEditorExist())
    {
        MRenderView::endRender();
        MRenderView::startRender(
            getWorldPtr()->mRenderGlobals->getWidth(),
            getWorldPtr()->mRenderGlobals->getHeight(),
            false,      // do clear background
            true);      // immediate feedback
    }

    getWorldPtr()->setRenderState(World::RSTATETRANSLATING);

    if (MGlobal::mayaState() != MGlobal::kBatch)
    {
        // we only need renderComputation (means esc-able rendering) if we render in UI (==NORMAL)
        if (getWorldPtr()->getRenderType() == World::UIRENDER)
        {
            if (MRenderView::doesRenderEditorExist())
                MGlobal::executePythonCommand("import pymel.core as pm; pm.waitCursor(state=True);");
        }
    }

    e.mType = Event::FRAMERENDER;
    RenderEventQueue.push(e);

    if (MGlobal::mayaState() != MGlobal::kBatch)
    {
        boost::thread cet = boost::thread(computationEventThread);
        cet.detach();
    }
}

void RenderQueueWorker::startRenderQueueWorker()
{
    while (true)
    {
        Event e;
        if (MGlobal::mayaState() == MGlobal::kBatch)
            RenderEventQueue.wait_and_pop(e);
        else
        {
            if (!RenderEventQueue.try_pop(e))
                break;
        }

        switch (e.mType)
        {
          case Event::FRAMERENDER:
            {
                if (renderThread.joinable())
                    renderThread.join();

                getWorldPtr()->mRenderGlobals->updateFrameNumber();
                doPreFrameJobs();
                doPrepareFrame();
                getWorldPtr()->mRenderer->defineProject();

                renderThread = boost::thread(renderThreadProc);
            }
            break;

          case Event::FRAMEDONE:
            {
                doPostFrameJobs();

                if (getWorldPtr()->mRenderGlobals->frameListDone())
                {
                    if (MGlobal::mayaState() != MGlobal::kBatch)
                    {
                        if (MRenderView::doesRenderEditorExist())
                            MGlobal::executePythonCommand("import pymel.core as pm; pm.waitCursor(state=False); pm.refresh()");
                    }

                    if (getWorldPtr()->getRenderType() == World::IPRRENDER)
                        removeCallbacks();

                    getWorldPtr()->setRenderState(World::RSTATEDONE);

                    if (MRenderView::doesRenderEditorExist())
                    {
                        MGlobal::executePythonCommandOnIdle(
                            MString("import pymel.core as pm; pm.renderWindowEditor(\"renderView\", edit=True);"));

                        // Empty the queue.
                        while (RenderEventQueue.try_pop(e)) {}
                    }

                    getWorldPtr()->destroyMayaScene();
                    getWorldPtr()->mRenderer->destroyProject();
                    getWorldPtr()->setRenderType(World::RTYPENONE);
                    getWorldPtr()->setRenderState(World::RSTATENONE);

                    return;
                }
                else
                {
                    e.mType = Event::FRAMERENDER;
                    RenderEventQueue.push(e);
                }
            }
            break;

          case Event::IPRSTOP:
            getWorldPtr()->setRenderState(World::RSTATESTOPPED);
            getWorldPtr()->mRenderer->abortRendering();
            break;
        }

        if (MGlobal::mayaState() != MGlobal::kBatch)
            break;
    }
}

void RenderQueueWorker::renderQueueWorkerTimerCallback(float time, float lastTime, void* userPtr)
{
    RenderQueueWorker::startRenderQueueWorker();
}
