
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
#include "renderqueue.h"

// appleseed-maya headers.
#include "utilities/concurrentqueue.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "event.h"
#include "mayascene.h"
#include "nodecallbacks.h"
#include "renderglobals.h"
#include "world.h"

// appleseed.foundation headers.
#include "foundation/platform/thread.h"

// Maya headers.
#include <maya/MDagPath.h>
#include <maya/MDGMessage.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MNodeMessage.h>
#include <maya/MRenderView.h>
#include <maya/MTimerMessage.h>

// Standard headers.
#include <algorithm>
#include <cassert>
#include <ctime>
#include <map>
#include <vector>

namespace
{
    MCallbackId idleCallbackId = 0;
    MCallbackId nodeAddedCallbackId = 0;
    MCallbackId nodeRemovedCallbackId = 0;
    clock_t renderStartTime = 0;
    clock_t renderEndTime = 0;
    size_t numPixelsDone;
    size_t numPixelsTotal;

    boost::thread renderThread;
    concurrent_queue<Event> renderEventQueue;
}

namespace
{
    void updateRenderView(
        const unsigned int  xMin,
        const unsigned int  xMax,
        const unsigned int  yMin,
        const unsigned int  yMax,
        RV_PIXEL*           pixels)
    {
        if (!MRenderView::doesRenderEditorExist())
            return;

        if (getWorldPtr()->mRenderGlobals->getUseRenderRegion())
        {
            unsigned int xmin = xMin, ymin = yMin, xmax = xMax, ymax = yMax;
            unsigned int left, right, bottom, top;
            MRenderView::getRenderRegion(left, right, bottom, top);
            if ((xMax < left) || (xMin > right) || (yMax < bottom) || (yMin > top))
                return;
            if ((xmin < left) || (ymin < bottom) || (xmax > right) || (ymax > top))
            {
                xmin = std::max(xmin, left);
                xmax = std::min(xmax, right);
                ymin = std::max(ymin, bottom);
                ymax = std::min(ymax, top);
                const unsigned int origWidth = xMax - xMin + 1;
                const unsigned int origHeight = yMax - yMin + 1;
                const unsigned int width = xmax - xmin + 1;
                const unsigned int height = ymax - ymin + 1;
                const unsigned int xStart = xmin - xMin;
                const unsigned int yStart = ymin - yMin;
                std::auto_ptr<RV_PIXEL> p = std::auto_ptr<RV_PIXEL>(new RV_PIXEL[width * height]);
                for (unsigned int x = 0; x < width; x++)
                {
                    for (unsigned int y = 0; y < height; y++)
                    {
                        const unsigned int sourceIndex = (yStart + y) * origWidth + xStart + x;
                        const unsigned int destIndex = y * width + x;
                        p.get()[destIndex] = pixels[sourceIndex];
                    }
                }
                MRenderView::updatePixels(xmin, xmax, ymin, ymax, p.get(), true);
                MRenderView::refresh(xmin, xmax, ymin, ymax);
                return;
            }
        }

        MRenderView::updatePixels(xMin, xMax, yMin, yMax, pixels, true);
        MRenderView::refresh(xMin, xMax, yMin, yMax);
    }

    void preTileRenderView(
        const unsigned int  xMin,
        const unsigned int  xMax,
        const unsigned int  yMin,
        const unsigned int  yMax)
    {
        if (!MRenderView::doesRenderEditorExist())
            return;

        unsigned int xmin = xMin, ymin = yMin, xmax = xMax, ymax = yMax;
        if (getWorldPtr()->mRenderGlobals->getUseRenderRegion())
        {
            unsigned int left, right, bottom, top;
            MRenderView::getRenderRegion(left, right, bottom, top);
            if (xMax < left || xMin > right || yMax < bottom || yMin > top)
                return;
            xmin = std::max(xmin, left);
            ymin = std::max(ymin, bottom);
            xmax = std::min(xmax, right);
            ymax = std::min(ymax, top);
        }

        RV_PIXEL line[4];
        for (uint x = 0; x < 4; x++)
            line[x].r = line[x].g = line[x].b = line[x].a = 1.0f;

        MRenderView::updatePixels(xmin, xmin + 3, ymin, ymin, line, true);
        MRenderView::updatePixels(xmin, xmin + 3, ymax, ymax, line, true);
        MRenderView::updatePixels(xmax - 3, xmax, ymin, ymin, line, true);
        MRenderView::updatePixels(xmax - 3, xmax, ymax, ymax, line, true);
        MRenderView::updatePixels(xmin, xmin, ymin, ymin + 3, line, true);
        MRenderView::updatePixels(xmax, xmax, ymin, ymin + 3, line, true);
        MRenderView::updatePixels(xmin, xmin, ymax - 3, ymax, line, true);
        MRenderView::updatePixels(xmax, xmax, ymax - 3, ymax, line, true);
        MRenderView::refresh(xmin, xmax, ymin, ymax);
    }

    MString getElapsedTimeString()
    {
        float elapsedTime = static_cast<float>(renderEndTime - renderStartTime) / CLOCKS_PER_SEC;
        const int hours = static_cast<int>(elapsedTime / 3600);
        elapsedTime -= hours * 3600;
        const int minutes = static_cast<int>(elapsedTime / 60);
        elapsedTime -= minutes * 60;
        const float seconds = elapsedTime;

        char hourStr[32], minStr[32], secStr[32];
        sprintf(hourStr, "%02d", hours);
        sprintf(minStr, "%02d", minutes);
        sprintf(secStr, "%02.1f", seconds);

        return format("Render Time: ^1s:^2s:^3s", hourStr, minStr,secStr);
    }

    MString getCaptionString()
    {
        const MString frameString = MString("Frame ") + getWorldPtr()->mRenderGlobals->getFrameNumber();
        const MString timeString = getElapsedTimeString();
        return MString("(appleseed)\\n") + frameString + "  " + timeString;
    }

    void renderThreadMain()
    {
        getWorldPtr()->mRenderer->render();

        Event event;
        event.mType = Event::RENDERDONE;
        renderEventQueue.push(event);
    }

    void markTransformsChildrenAsDirty()
    {
        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

        for (MayaScene::EditableElementContainer::iterator
                i = mayaScene->editableElements.begin(),
                e = mayaScene->editableElements.end(); i != e; ++i)
        {
            if (i->second.isDirty)
            {
                if (i->second.node.hasFn(MFn::kTransform))
                {
                    MItDag childIter;
                    for (childIter.reset(i->second.node); !childIter.isDone(); childIter.next())
                    {
                        if (childIter.currentItem().hasFn(MFn::kShape))
                        {
                            for (MayaScene::EditableElementContainer::iterator
                                    j = mayaScene->editableElements.begin(),
                                    f = mayaScene->editableElements.end(); j != f; ++j)
                            {
                                j->second.isDirty = true;
                                j->second.isTransformed = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    void addNodeCallbacks()
    {
        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

        // Add some global dependency nodes to the update list.
        MObject renderGlobalsNode = getRenderGlobalsNode();
        MCallbackId callbackId = MNodeMessage::addNodeDirtyCallback(renderGlobalsNode, IPRNodeDirtyCallback);
        EditableElement& element = mayaScene->editableElements[callbackId];
        element.mobj = renderGlobalsNode;
        element.name = getObjectName(element.mobj);
        element.node = element.mobj;

        idleCallbackId = MTimerMessage::addTimerCallback(0.2f, IPRIdleCallback);
        nodeAddedCallbackId = MDGMessage::addNodeAddedCallback(IPRNodeAddedCallback);
        nodeRemovedCallbackId = MDGMessage::addNodeRemovedCallback(IPRNodeRemovedCallback);
    }

    void removeNodeCallbacks()
    {
        if (idleCallbackId != 0)
        {
            MMessage::removeCallback(idleCallbackId);
            idleCallbackId = 0;
        }

        if (nodeAddedCallbackId != 0)
        {
            MDGMessage::removeCallback(nodeAddedCallbackId);
            nodeAddedCallbackId = 0;
        }

        if (nodeRemovedCallbackId != 0)
        {
            MDGMessage::removeCallback(nodeRemovedCallbackId);
            nodeRemovedCallbackId = 0;
        }

        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

        for (MayaScene::EditableElementContainer::const_iterator
                i = mayaScene->editableElements.begin(),
                e = mayaScene->editableElements.end(); i != e; ++i)
            MMessage::removeCallback(i->first);
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
            getWorldPtr()->mRenderGlobals->currentFrameNumber = (float)(currentFrame + getWorldPtr()->mRenderGlobals->mbElementList[mbStepId].time);
            bool needView = true;

            // we can have some mb time steps at the same time, e.g. for xform and deform, then we do not need to update the view
            if (mbStepId > 0)
            {
                if (getWorldPtr()->mRenderGlobals->mbElementList[mbStepId].time == getWorldPtr()->mRenderGlobals->mbElementList[mbStepId - 1].time)
                {
                    needView = false;
                }
            }

            if (needView)
            {
                Logging::debug(MString("doFrameJobs() viewFrame: ") + getWorldPtr()->mRenderGlobals->currentFrameNumber);
                MGlobal::viewFrame(getWorldPtr()->mRenderGlobals->currentFrameNumber);
            }

            if (getWorldPtr()->mScene)
                mayaScene->updateScene();
            else
                Logging::error(MString("no maya scene ptr."));

            Logging::info(MString("update scene done"));
            getWorldPtr()->mRenderGlobals->currentMbStep++;
        }

        if (MGlobal::mayaState() != MGlobal::kBatch)
            MGlobal::viewFrame(currentFrame);
    }

    void logRenderingProgress(const size_t pixelsDone, const size_t pixelsTotal)
    {
        const float percents = static_cast<float>(100 * pixelsDone) / pixelsTotal;
        if (static_cast<int>(percents) % 5 == 0)
        {
            char buffer[16];
            sprintf(buffer, "%3.2f", percents);
            MString progressStr;
            progressStr.format("^1s% done.", MString(buffer));
            Logging::info(progressStr);
            MGlobal::executePythonCommand(format("import appleseed_maya.initialize; appleseed_maya.initialize.theRenderer().updateProgressBar(^1s)", buffer));
        }
    }
}

void initRender(const World::RenderType renderType, const int width, const int height, const MDagPath cameraDagPath, const bool doRenderRegion)
{
    renderStartTime = clock();
    getWorldPtr()->setRenderType(renderType);

    // Here we create the overall scene, renderer and renderGlobals objects
    getWorldPtr()->initializeRenderEnvironment();
    getWorldPtr()->mRenderGlobals->setResolution(width, height);
    getWorldPtr()->mRenderGlobals->setUseRenderRegion(doRenderRegion);
    getWorldPtr()->mScene->uiCamera = cameraDagPath;
    getWorldPtr()->mRenderer->initializeRenderer(); // init renderer with all type, image size etc.

    if (MRenderView::doesRenderEditorExist())
    {
        const int width = getWorldPtr()->mRenderGlobals->getWidth();
        const int height = getWorldPtr()->mRenderGlobals->getHeight();

        if (getWorldPtr()->mRenderGlobals->getUseRenderRegion())
        {
            unsigned int left, right, bottom, top;
            MRenderView::getRenderRegion(left, right, bottom, top);
#if MAYA_API_VERSION >= 201600
            MRenderView::startRegionRender(width, height, left, right, bottom, top, true, true);
#else
            MRenderView::startRegionRender(width, height, left, right, bottom, top, false, true);
#endif
        }
        else
        {
#if MAYA_API_VERSION >= 201600
            MRenderView::startRender(width, height, true, true);
#else
            MRenderView::startRender(width, height, false, true);
#endif
        }
#if MAYA_API_VERSION >= 201600
        MRenderView::setDrawTileBoundary(false);
#endif
    }
    getWorldPtr()->setRenderState(World::RSTATETRANSLATING);
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

    if (MGlobal::mayaState() != MGlobal::kBatch)
    {
        if (getWorldPtr()->getRenderType() != World::IPRRENDER)
        {
            if (MRenderView::doesRenderEditorExist())
                MGlobal::executePythonCommand("import pymel.core as pm; pm.waitCursor(state=True);");
        }
    }

    numPixelsDone = 0;
    numPixelsTotal = width * height;

    if (getWorldPtr()->getRenderType() != World::IPRRENDER)
    {
        if (MGlobal::mayaState() != MGlobal::kBatch)
        {
            boost::thread cet = boost::thread(computationEventThread);
            cet.detach();
        }
    }

    if (MGlobal::mayaState() != MGlobal::kBatch)
    {
        getWorldPtr()->mRenderGlobals->updateFrameNumber();
        doPreFrameJobs(); // preRenderScript etc.
        doPrepareFrame(); // parse scene and update objects
        getWorldPtr()->mRenderer->preFrame();
        if (getWorldPtr()->getRenderType() == World::IPRRENDER)
            addNodeCallbacks();
        startRendering();
    }
    else
    {
        while (!getWorldPtr()->mRenderGlobals->frameListDone())
        {
            getWorldPtr()->mRenderGlobals->updateFrameNumber();
            doPreFrameJobs(); // preRenderScript etc.
            doPrepareFrame(); // parse scene and update objects
            getWorldPtr()->mRenderer->preFrame();
            getWorldPtr()->mRenderer->render(); // render blocking
            doPostFrameJobs();
        }

        waitUntilRenderFinishes();
    }
}

void startRendering()
{
    renderThread = boost::thread(renderThreadMain);
}

void stopRendering()
{
    getWorldPtr()->mRenderer->abortRendering();
    renderThread.join();
}

void waitUntilRenderFinishes()
{
    renderThread.join();

    if (MGlobal::mayaState() != MGlobal::kBatch)
    {
        if (MRenderView::doesRenderEditorExist())
        {
            MRenderView::endRender();
            if (getWorldPtr()->getRenderType() != World::IPRRENDER)
                MGlobal::executePythonCommand("import pymel.core as pm; pm.waitCursor(state=False); pm.refresh()");
        }
    }

    if (getWorldPtr()->getRenderType() == World::IPRRENDER)
        removeNodeCallbacks();

    getWorldPtr()->setRenderState(World::RSTATEDONE);
    renderEndTime = clock();

    if (MRenderView::doesRenderEditorExist())
    {
        MGlobal::executePythonCommandOnIdle(
            MString("import pymel.core as pm; pm.renderWindowEditor(\"renderView\", edit=True, pcaption=\"") + getCaptionString() + "\");");

        // Empty the queue.
        Event e;
        while (renderEventQueue.try_pop(e)) {}
    }

    getWorldPtr()->cleanUpAfterRender();
    getWorldPtr()->mRenderer->unInitializeRenderer();
    getWorldPtr()->setRenderState(World::RSTATENONE);
    MGlobal::executePythonCommand("import appleseed_maya.initialize; appleseed_maya.initialize.theRenderer().postRenderProcedure()");
}

void iprUpdateRenderRegion()
{
    stopRendering();

    unsigned int left, right, bottom, top;
    MRenderView::getRenderRegion(left, right, bottom, top);
    const foundation::AABB2u crop(
        foundation::AABB2u::VectorType(left, bottom),
        foundation::AABB2u::VectorType(right, top));
    getWorldPtr()->mRenderer->getProjectPtr()->get_frame()->set_crop_window(crop);

    startRendering();
}

void pushEvent(const Event& e)
{
    renderEventQueue.push(e);
}

void renderQueueWorkerCallback(float time, float lastTime, void* userPtr)
{
    Event e;
    if (!renderEventQueue.try_pop(e))
        return;

    switch (e.mType)
    {
      case Event::RENDERDONE:
        {
            if (getWorldPtr()->getRenderType() != World::IPRRENDER)
                waitUntilRenderFinishes();
        }
        break;

      case Event::UPDATEUI:
        {
            updateRenderView(e.xMin, e.xMax, e.yMin, e.yMax, e.pixels.get());
            numPixelsDone += (e.xMax - e.xMin) * (e.yMax - e.yMin);
            if (getWorldPtr()->getRenderType() != World::IPRRENDER)
                logRenderingProgress(numPixelsDone, numPixelsTotal);
        }
        break;

      case Event::PRETILE:
        {
            preTileRenderView(e.xMin, e.xMax, e.yMin, e.yMax);
        }
        break;
    }
}
