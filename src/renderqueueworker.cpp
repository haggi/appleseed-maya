
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
#include <ctime>
#include <map>
#include <vector>

namespace
{
    MCallbackId idleCallbackId = 0;
    MCallbackId nodeAddedCallbackId = 0;
    MCallbackId nodeRemovedCallbackId = 0;
    std::vector<MCallbackId> nodeCallbacks;
    std::vector<InteractiveElement *> modifiedElementList;
    clock_t renderStartTime = 0;
    clock_t renderEndTime = 0;
    bool IprCallbacksDone = false;
    std::map<MCallbackId, MObject> objIdMap;
    std::map<MCallbackId, InteractiveElement *> idInteractiveMap;

    boost::thread sceneThread;
    concurrent_queue<Event> RenderEventQueue;
}

concurrent_queue<Event>* gEventQueue()
{
    return &RenderEventQueue;
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

        // We have cases where the render view has changed but the framebuffer callback may have still the old settings.
        // Here we make sure we do not exceed the render view area.
        // todo: is this still relevant?
        if (getWorldPtr()->mRenderGlobals->getUseRenderRegion())
        {
            const int width = getWorldPtr()->mRenderGlobals->getWidth();
            const int height = getWorldPtr()->mRenderGlobals->getHeight();

            if (xMin != 0 ||
                yMin != 0 ||
                xMax != width - 1 ||
                yMax != height - 1)
            {
                unsigned int left, right, bottom, top;
                MRenderView::getRenderRegion(left, right, bottom, top);

                if (left != xMin ||
                    right != xMax ||
                    bottom != yMin ||
                    top != yMax)
                    return;
            }
        }

        MRenderView::updatePixels(xMin, xMax, yMin, yMax, pixels);
        MRenderView::refresh(xMin, xMax, yMin, yMax);
    }

    MString getElapsedTimeString()
    {
        int hours;
        int minutes;
        float sec;
        float elapsedTime = (float)(renderEndTime - renderStartTime)/(float)CLOCKS_PER_SEC;
        hours = elapsedTime/3600;
        elapsedTime -= hours * 3600;
        minutes = elapsedTime/60;
        elapsedTime -= minutes * 60;
        sec = elapsedTime;
        char hourStr[1024], minStr[1024], secStr[1024];
        memset(hourStr, '\0', 1024);
        memset(minStr, '\0', 1024);
        memset(secStr, '\0', 1024);
        sprintf(hourStr, "%02d", hours);
        sprintf(minStr, "%02d", minutes);
        sprintf(secStr, "%02.1f", sec);

        MString timeString = MString("") + hourStr + ":" + minStr + ":" + secStr;
        return (MString("Render Time: ") + timeString);
    }

    MString getCaptionString()
    {
        const MString frameString = MString("Frame ") + getWorldPtr()->mRenderGlobals->getFrameNumber();
        const MString timeString = getElapsedTimeString();
        return MString("(appleseed)\\n") + frameString + "  " + timeString;
    }

    // one problem: In most cases the renderer translates shapes only not complete hierarchies
    // To do a correct update of all interesting nodes, I have to find the shapes below a transform node.
    // Here we fill the modifiedElementList with shape nodes and non transform nodes

    void iprFindLeafNodes()
    {
        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;
        std::map<MCallbackId, InteractiveElement *>::iterator it;
        std::vector<InteractiveElement *> leafList;

        for (it = idInteractiveMap.begin(); it != idInteractiveMap.end(); it++)
        {
            MObject node = it->second->node;
            if (node.hasFn(MFn::kTransform))
            {
                MItDag dagIter;
                for (dagIter.reset(node); !dagIter.isDone(); dagIter.next())
                {
                    if (dagIter.currentItem().hasFn(MFn::kShape))
                    {
                        std::map<uint, InteractiveElement>::iterator seIt;
                        for (seIt = mayaScene->interactiveUpdateMap.begin(); seIt != mayaScene->interactiveUpdateMap.end(); seIt++)
                        {
                            if (seIt->second.node.hasFn(MFn::kShape))
                            {
                                InteractiveElement iel = seIt->second;
                                if (seIt->second.node == dagIter.currentItem())
                                {
                                    InteractiveElement *ie = &mayaScene->interactiveUpdateMap[seIt->first];
                                    ie->triggeredFromTransform = true;
                                    leafList.push_back(&mayaScene->interactiveUpdateMap[seIt->first]);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                leafList.push_back(it->second);
            }
        }

        // the idea is that the renderer waits in IPR mode for an non empty modifiesElementList,
        // it updates the render database with the elements and empties the list which is then free for the next run
        // todo: maybe use wait for variables.
        while (modifiedElementList.size() > 0)
            foundation::sleep(100);

        std::vector<InteractiveElement *>::iterator llIt;
        for (llIt = leafList.begin(); llIt != leafList.end(); llIt++)
            modifiedElementList.push_back(*llIt);
    }

    //
    // basic idea:
    //      all important messages like update framebuffer, stop rendering etc. are sent via events to the message queue
    //      a timer callback reads the next event and execute it.
    // IPR:
    //      before IPR rendering starts, callbacks are created.
    //      a node dirty callback for every node in the scene will put the the node into a list of modified objects.
    //          - because we do need the elements only once but the callbacks are called very often, we throw away the callback
    //            from a modified node after putting the node into a list and we later remove duplicate entries
    //      a idle callback is created. It will go through all elements from the modified list and update it in the renderer if necessary.
    //            then the renderer will be called to update its database or restart render, however a renderer handles interactive rendering.
    //          - then the modified list is emptied
    //          - because we want to be able to modify the same object again after it is updated, the node dirty callbacks are recreated for
    //            all the objects in the list.
    //      a scene message is created - we will stop the ipr as soon as a new scene is created or another scene is opened
    //      a scene message is created - we have to stop everything as soon as the plugin will be removed, otherwise maya will crash.
    //      IMPORTANT:  We do add dirty callbacks for translated nodes only which are saved in the mayaScene::interactiveUpdateMap.
    //                  This map is filled by sceneParsing and shader translation process which are called before rendering and during geometry translation.
    //                  So the addIPRCallbacks() has to be called after everything is translated.

    void IPRattributeChangedCallback(MNodeMessage::AttributeMessage msg, MPlug & plug,  MPlug & otherPlug, void *element)
    {
        Logging::debug(MString("IPRattributeChangedCallback. attribA: ") + plug.name() + " attribB: " + otherPlug.name());
        InteractiveElement *userData = (InteractiveElement *)element;
        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

        if (!userData->obj)
            return;

        if (msg & MNodeMessage::kConnectionMade)
        {
            Logging::debug(MString("IPRattributeChangedCallback. connection created."));
            MString plugName = plug.name();
            std::string pn = plugName.asChar();
            if (pn.find("instObjGroups[") != std::string::npos)
            {
                Logging::debug(MString("IPRattributeChangedCallback. InstObjGroups affected, checking other side."));
                if (otherPlug.node().hasFn(MFn::kShadingEngine))
                {
                    Logging::debug(MString("IPRattributeChangedCallback. Found shading group on the other side: ") + getObjectName(otherPlug.node()));
                    MCallbackId thisId = MMessage::currentCallbackId();
                    MObject sgNode = otherPlug.node();
                    InteractiveElement iel;
                    iel.mobj = sgNode;
                    iel.name = getObjectName(sgNode);
                    iel.node = sgNode;
                    iel.obj = userData->obj;
                    mayaScene->interactiveUpdateMap[mayaScene->interactiveUpdateMap.size()] = iel;
                    idInteractiveMap[thisId] = &mayaScene->interactiveUpdateMap[mayaScene->interactiveUpdateMap.size() -1];
                }
            }
        }
        else if (msg & MNodeMessage::kConnectionBroken)
        {
            Logging::debug(MString("IPRattributeChangedCallback. connection broken."));
        }
    }

    void IPRNodeDirtyCallback(void *interactiveElement)
    {
        MStatus stat;
        InteractiveElement *userData = (InteractiveElement *)interactiveElement;

        MCallbackId thisId = MMessage::currentCallbackId();
        idInteractiveMap[thisId] = userData;
    }

    void IPRIdleCallback(float time, float lastTime, void *userPtr)
    {
        if (idInteractiveMap.empty())
            return;

        getWorldPtr()->mRenderer->abortRendering();
        iprFindLeafNodes();
        idInteractiveMap.clear();
    }

    void IPRNodeAddedCallback(MObject& node, void *userPtr)
    {
        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;
        MStatus stat;

        if (node.hasFn(MFn::kTransform))
        {
            MFnDagNode dagNode(node);

            MDagPath dagPath;
            stat = dagNode.getPath(dagPath);
            MString why = stat.errorString();
            stat = dagPath.extendToShape();
            MObject nn = dagPath.node();
            MString na = dagPath.fullPathName();
            if (!dagPath.node().hasFn(MFn::kMesh))
                return;
        }
        else
        {
            if (!node.hasFn(MFn::kShape))
                return;
        }
        MFnDagNode dagNode(node);
        MString p = dagNode.fullPathName();
        MDagPath dagPath;
        dagNode.getPath(dagPath);
        dagPath.pop();
        MObject transform = dagPath.node();
        MString tname = getObjectName(transform);

        // here the new object is added to the object list and is added to the interactive object list
        mayaScene->parseSceneHierarchy(dagPath, 0, boost::shared_ptr<ObjectAttributes>(), boost::shared_ptr<MayaObject>());

        // now we readd all interactive objects to the map
        idInteractiveMap.clear();
        MCallbackId transformId = 0;
        InteractiveElement *userData = 0;
        std::map<uint, InteractiveElement>::reverse_iterator riter;
        for (riter = mayaScene->interactiveUpdateMap.rbegin(); riter != mayaScene->interactiveUpdateMap.rend(); riter++)
        {
            InteractiveElement ie = riter->second;
            if ((ie.node == node) || (ie.node == transform))
            {
                // problem: a newly created procedural mesh does not yet have a shape because it not yet connected to its creator node
                // we have to find a reliable solution for this. Maybe we can add a attribute callback and check for inMesh.
                userData = &mayaScene->interactiveUpdateMap[riter->first];
                MCallbackId id = MNodeMessage::addNodeDirtyCallback(ie.node, IPRNodeDirtyCallback, userData);
                objIdMap[id] = ie.node;

                if (ie.node == node) // we only add the shape node to the update map because do not want a transform update
                    idInteractiveMap[id] = userData;

                if (ie.node.hasFn(MFn::kMesh))
                {
                    MString nd = getObjectName(ie.node);
                    id = MNodeMessage::addAttributeChangedCallback(ie.node, IPRattributeChangedCallback, userData, &stat);
                    objIdMap[id] = ie.node;
                    if (stat)
                        nodeCallbacks.push_back(id);
                }
            }
        }
    }

    void IPRNodeRemovedCallback(MObject& node, void* userPtr)
    {
        // Get the callback id, remove the callback for this node and remove the callback from the list.
        MCallbackId nodeCallbackId = 0;
        for (std::map<MCallbackId, MObject>::iterator i = objIdMap.begin(); i != objIdMap.end(); i++)
        {
            if (i->second == node)
            {
                MNodeMessage::removeCallback(i->first);
                nodeCallbackId = i->first;
                break;
            }
        }
        if (nodeCallbackId != 0)
            objIdMap.erase(nodeCallbackId);

        // Get the MayaObject element and mark it as removed.
        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;
        for (std::map<uint, InteractiveElement>::iterator i = mayaScene->interactiveUpdateMap.begin();
             i != mayaScene->interactiveUpdateMap.end();
             ++i)
        {
            const InteractiveElement& ie = i->second;
            if (ie.node == node)
            {
                if (ie.obj)
                {
                    ie.obj->removed = true;
                    // Trigger an IPR scene update.
                    idInteractiveMap[nodeCallbackId] = &mayaScene->interactiveUpdateMap[i->first];
                    break;
                }
            }
        }
    }

    void addIPRCallbacks()
    {
        MStatus stat;
        IprCallbacksDone = false;
        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

        std::map<uint, InteractiveElement>::iterator ite;
        std::map<uint, InteractiveElement> ielements = mayaScene->interactiveUpdateMap;
        for (ite = ielements.begin(); ite != ielements.end(); ite++)
        {
            uint elementId = ite->first;
            InteractiveElement iae = ite->second;
            MObject nodeDirty;

            if (iae.obj)
                nodeDirty = iae.obj->mobject;
            else
                nodeDirty = iae.mobj;

            if (iae.mobj.hasFn(MFn::kPluginDependNode))
            {
                MFnDependencyNode depFn(iae.mobj);
                nodeDirty = iae.mobj;
            }
            Logging::debug(MString("Adding dirty callback node ") + getObjectName(nodeDirty));
            InteractiveElement *userData = &mayaScene->interactiveUpdateMap[elementId];
            MCallbackId id = MNodeMessage::addNodeDirtyCallback(nodeDirty, IPRNodeDirtyCallback, userData, &stat);
            objIdMap[id] = nodeDirty;
            if (stat)
                nodeCallbacks.push_back(id);

            if (nodeDirty.hasFn(MFn::kMesh))
            {
                MString nd = getObjectName(nodeDirty);
                id = MNodeMessage::addAttributeChangedCallback(nodeDirty, IPRattributeChangedCallback, userData, &stat);
                objIdMap[id] = nodeDirty;
                if (stat)
                    nodeCallbacks.push_back(id);
            }
        }

        idleCallbackId = MTimerMessage::addTimerCallback(0.2, IPRIdleCallback, 0, &stat);
        nodeAddedCallbackId = MDGMessage::addNodeAddedCallback(IPRNodeAddedCallback);
        nodeRemovedCallbackId = MDGMessage::addNodeRemovedCallback(IPRNodeRemovedCallback);

        IprCallbacksDone = true;
    }

    // register new created nodes. We need the transform and the shape node to correctly use it in IPR.
    // So we simply use the shape node, get it's parent - a shape node and let the scene parser do the rest.
    // Then add a node dirty callback for the new elements. By adding the callback ids to the idInteractiveMap, the
    // IPR should detect a modification during the netxt update cycle.

    // Handling of surface shaders is a bit different. A shader is not assigned directly to a surface but it is connected to a shading group
    // which is nothing else but a objectSet. If a new surface shader is created, it is not in use until it is assigned to an object what means it is connected
    // to a shading group. So I simply add a shadingGroup callback for new shading groups.

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
                Event e;
                e.mType = Event::INTERRUPT;
                gEventQueue()->push(e);
                break;
            }
#endif

            foundation::sleep(100);
        }
    }

    void renderProcessThread()
    {
        if (getWorldPtr()->getRenderType() == World::IPRRENDER)
        {
            // the idea is that the renderer waits in IPR mode for an non empty modifiesElementList,
            // it updates the render database with the elements and empties the list which is then free for the next run
            while ((getWorldPtr()->getRenderType() == World::IPRRENDER))
            {
                getWorldPtr()->mRenderer->render();
                while ((modifiedElementList.size() == 0) && (getWorldPtr()->getRenderType() == World::IPRRENDER) && (getWorldPtr()->getRenderState() != World::RSTATESTOPPED))
                    foundation::sleep(100);
                if ((getWorldPtr()->getRenderType() != World::IPRRENDER) || (getWorldPtr()->getRenderState() == World::RSTATESTOPPED))
                    break;
                getWorldPtr()->mRenderer->interactiveUpdateList = modifiedElementList;
                getWorldPtr()->mRenderer->doInteractiveUpdate();
                modifiedElementList.clear();
            }
        }
        else
        {
            Logging::debug("RenderQueueWorker::renderProcessThread()");
            getWorldPtr()->mRenderer->render();
            Logging::debug("RenderQueueWorker::renderProcessThread() - DONE.");
        }

        Event event;
        event.mType = Event::FRAMEDONE;
        gEventQueue()->push(event);
    }

    void iprWaitForFinish(Event e)
    {
        while (getWorldPtr()->getRenderState() != World::RSTATENONE)
            foundation::sleep(100);

        gEventQueue()->push(e);
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

    void logOutput(const int pixelsDone, const int pixelsTotal)
    {
        Logging::info(MString("") + (float)pixelsDone / pixelsTotal + "% done.");

    }
}

void RenderQueueWorker::startRenderQueueWorker()
{
    while (true)
    {
        Event e;
        if (MGlobal::mayaState() == MGlobal::kBatch)
            gEventQueue()->wait_and_pop(e);
        else
        {
            if (!gEventQueue()->try_pop(e))
                break;
        }

        switch (e.mType)
        {
          case Event::INITRENDER:
            {
                renderStartTime = clock();
                getWorldPtr()->setRenderType(e.renderType);

                // It is possible that a new ipr rendering is started before another one is completely done, so wait for it.
                if (getWorldPtr()->getRenderType() == World::IPRRENDER)
                {
                    if (getWorldPtr()->getRenderState() != World::RSTATENONE)
                    {
                        boost::thread waitThread = boost::thread(iprWaitForFinish, e);
                        waitThread.detach();
                        break;
                    }
                }

                // Here we create the overall scene, renderer and renderGlobals objects
                getWorldPtr()->initializeRenderEnvironment();

                getWorldPtr()->mRenderGlobals->setResolution(e.width, e.height);
                getWorldPtr()->mRenderGlobals->setUseRenderRegion(e.useRenderRegion);
                getWorldPtr()->mScene->uiCamera = e.cameraDagPath;
                getWorldPtr()->mRenderer->initializeRenderer(); // init renderer with all type, image size etc.

                if (MRenderView::doesRenderEditorExist())
                {
                    MRenderView::endRender();

                    const int width = getWorldPtr()->mRenderGlobals->getWidth();
                    const int height = getWorldPtr()->mRenderGlobals->getHeight();
                    MRenderView::startRender(width, height, false, true);
                }

                getWorldPtr()->setRenderState(World::RSTATETRANSLATING);
                boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

                if (MGlobal::mayaState() != MGlobal::kBatch)
                {
                    // we only need renderComputation (means esc-able rendering) if we render in UI (==NORMAL)
                    if (getWorldPtr()->getRenderType() == World::UIRENDER)
                    {
                        if (MRenderView::doesRenderEditorExist())
                            MGlobal::executePythonCommand("import pymel.core as pm; pm.waitCursor(state=True);");
                    }
                }

                RenderQueueWorker::numPixelsDone = 0;
                RenderQueueWorker::numPixelsTotal = e.width * e.height;
                e.mType = Event::FRAMERENDER;
                gEventQueue()->push(e);

                if (MGlobal::mayaState() != MGlobal::kBatch)
                {
                    boost::thread cet = boost::thread(computationEventThread);
                    cet.detach();
                }
            }
            break;

          case Event::FRAMERENDER:
            {
                if (sceneThread.joinable())
                    sceneThread.join();

                if (!getWorldPtr()->mRenderGlobals->frameListDone())
                {
                    getWorldPtr()->mRenderGlobals->updateFrameNumber();
                    doPreFrameJobs(); // preRenderScript etc.
                    doPrepareFrame(); // parse scene and update objects
                    getWorldPtr()->mRenderer->preFrame();
                    sceneThread = boost::thread(renderProcessThread);
                }
                else
                {
                    e.mType = Event::RENDERDONE;
                    gEventQueue()->push(e);
                }
            }
            break;

          case Event::FRAMEDONE:
            {
                doPostFrameJobs();

                e.mType = Event::FRAMERENDER;
                gEventQueue()->push(e);
            }
            break;

          case Event::RENDERDONE:
            {
                if (MGlobal::mayaState() != MGlobal::kBatch)
                {
                    if (MRenderView::doesRenderEditorExist())
                        MGlobal::executePythonCommand("import pymel.core as pm; pm.waitCursor(state=False); pm.refresh()");
                }

                if (getWorldPtr()->getRenderType() == World::IPRRENDER)
                    removeCallbacks();

                getWorldPtr()->setRenderState(World::RSTATEDONE);
                renderEndTime = clock();

                if (MRenderView::doesRenderEditorExist())
                {
                    MGlobal::executePythonCommandOnIdle(
                        MString("import pymel.core as pm; pm.renderWindowEditor(\"renderView\", edit=True, pcaption=\"") + getCaptionString() + "\");");

                    // Empty the queue.
                    while (RenderEventQueue.try_pop(e)) {}
                }

                getWorldPtr()->cleanUpAfterRender();
                getWorldPtr()->mRenderer->unInitializeRenderer();
                getWorldPtr()->setRenderState(World::RSTATENONE);
            }
            return;     // note: terminate the loop

          case Event::INTERRUPT:
            getWorldPtr()->mRenderer->abortRendering();
            break;

          case Event::IPRSTOP:
            getWorldPtr()->setRenderState(World::RSTATESTOPPED);
            getWorldPtr()->mRenderer->abortRendering();
            break;

          case Event::ADDIPRCALLBACKS:
            addIPRCallbacks();
            break;

          case Event::UPDATEUI:
            updateRenderView(e.xMin, e.xMax, e.yMin, e.yMax, e.pixels.get());
            RenderQueueWorker::numPixelsDone += (e.xMax - e.xMin) * (e.yMax - e.yMin);
            logOutput(RenderQueueWorker::numPixelsDone, RenderQueueWorker::numPixelsTotal);
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

void RenderQueueWorker::IPRUpdateCallbacks()
{
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

    for (size_t elementId = 0; elementId < mayaScene->interactiveUpdateMap.size(); elementId++)
    {
        InteractiveElement *element = &mayaScene->interactiveUpdateMap[elementId];
        MCallbackId id = 0;

        std::map<MCallbackId, MObject>::iterator mit;
        std::map<MCallbackId, MObject> oimap = objIdMap;
        for (mit = oimap.begin(); mit != oimap.end(); mit++)
        {
            if (element->node == mit->second)
            {
                id = mit->first;
                break;
            }
        }

        if (id == 0)
        {
            MObject nodeDirty = element->node;
            Logging::debug(MString("IPRUpdateCallbacks. Found element without callback: ") + getObjectName(nodeDirty));
            MStatus stat;
            id = MNodeMessage::addNodeDirtyCallback(nodeDirty, IPRNodeDirtyCallback, element, &stat);
            objIdMap[id] = nodeDirty;
            if (stat)
                nodeCallbacks.push_back(id);
        }
    }
}

bool RenderQueueWorker::iprCallbacksDone()
{
    return IprCallbacksDone;
}
