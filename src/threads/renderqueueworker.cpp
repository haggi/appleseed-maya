
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

#include <time.h>
#include <map>

#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>
#include <maya/MTimerMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MEventMessage.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>

#include "renderqueueworker.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "../mayascene.h"
#include "../mayatoworld.h"

#include "foundation/platform/thread.h"

namespace
{
    int numTiles = 0;
    int tilesDone = 0;
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

    bool escPressed;
    bool checkDone;
    boost::thread checkThread;

    void checkInterrupt()
    {
        while (!checkDone)
        {
    #ifdef _WIN32
            if (GetAsyncKeyState(VK_ESCAPE))
            {
                escPressed = true;
                break;
            }
    #endif
            foundation::sleep(100);
        }
    }

    void beginComputation()
    {
        escPressed = false;
        checkDone = false;
        checkThread = boost::thread(checkInterrupt);

        if (MRenderView::doesRenderEditorExist())
            MGlobal::executePythonCommand("import pymel.core as pm;pm.waitCursor(state=True);");
    }

    void endComputation()
    {
        checkDone = true;

        if (checkThread.joinable())
            checkThread.join();

        if (MRenderView::doesRenderEditorExist())
            MGlobal::executePythonCommand("import pymel.core as pm;pm.waitCursor(state=False);pm.refresh()");
    }
}

concurrent_queue<Event> *theRenderEventQueue()
{
    return &RenderEventQueue;
}

bool RenderQueueWorker::iprCallbacksDone()
{
    return IprCallbacksDone;
}

void setStartTime()
{
    renderStartTime = clock();
}

void setEndTime()
{
    renderEndTime = clock();
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
    const MString frameString = MString("Frame ") + getWorldPtr()->worldRenderGlobalsPtr->getFrameNumber();
    const MString timeString = getElapsedTimeString();
    return MString("(appleseed)\\n") + frameString + "  " + timeString;
}

// one problem: In most cases the renderer translates shapes only not complete hierarchies
// To do a correct update of all interesting nodes, I have to find the shapes below a transform node.
// Here we fill the modifiedElementList with shape nodes and non transform nodes

void iprFindLeafNodes()
{
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;
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

    Logging::debug(MString("iprFindLeafNodes ") + leafList.size());

    // the idea is that the renderer waits in IPR mode for an non empty modifiesElementList,
    // it updates the render database with the elements and empties the list which is then free for the next run
    // todo: maybe use wait for variables.
    while (modifiedElementList.size() > 0)
        asf::sleep(100);

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
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;

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

    getWorldPtr()->worldRendererPtr->abortRendering();
    iprFindLeafNodes();
    idInteractiveMap.clear();
}

void IPRNodeAddedCallback(MObject& node, void *userPtr)
{
    Logging::debug(MString("IPRNodeAddedCallback. Node: ") + getObjectName(node));
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;
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

void IPRNodeRemovedCallback(MObject& node, void *userPtr)
{
    Logging::debug(MString("IPRNodeRemovedCallback. Removing node: ") + getObjectName(node));

    //get the callback id and remove the callback for this node and remove the callback from the list
    std::map<MCallbackId, MObject>::iterator idIter;
    MCallbackId nodeCallbackId = 0;
    for (idIter = objIdMap.begin(); idIter != objIdMap.end(); idIter++)
    {
        if (idIter->second == node)
        {
            MNodeMessage::removeCallback(idIter->first);
            nodeCallbackId = idIter->first;
            break;
        }
    }
    if (nodeCallbackId != 0)
        objIdMap.erase(nodeCallbackId);

    // get the MayaObject element and mark it as removed.
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;
    std::map<uint, InteractiveElement>::iterator iter;
    for (iter = mayaScene->interactiveUpdateMap.begin(); iter != mayaScene->interactiveUpdateMap.end(); iter++)
    {
        InteractiveElement ie = iter->second;

        if (ie.node == node)
        {
            if (ie.obj)
            {
                ie.obj->removed = true;
                // trigger a ipr scene update
                idInteractiveMap[nodeCallbackId] = &mayaScene->interactiveUpdateMap[iter->first];
                break;
            }
        }
    }
}

void addIPRCallbacks()
{
    MStatus stat;
    IprCallbacksDone = false;
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;

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

void RenderQueueWorker::IPRUpdateCallbacks()
{
    MStatus stat;
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;

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
            id = MNodeMessage::addNodeDirtyCallback(nodeDirty, IPRNodeDirtyCallback, element, &stat);
            objIdMap[id] = nodeDirty;
            if (stat)
                nodeCallbacks.push_back(id);
        }
    }
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
    idleCallbackId = nodeRemovedCallbackId = nodeAddedCallbackId = 0;
    std::vector<MCallbackId>::iterator iter;
    for (iter = nodeCallbacks.begin(); iter != nodeCallbacks.end(); iter++)
        MMessage::removeCallback(*iter);
    nodeCallbacks.clear();
    objIdMap.clear();
    modifiedElementList.clear(); // make sure that the iprFindLeafNodes exits with an empty list
}

void RenderQueueWorker::renderQueueWorkerTimerCallback(float time, float lastTime, void *userPtr)
{
    RenderQueueWorker::startRenderQueueWorker();
}

void computationEventThread()
{
    bool done = false;
    bool renderingStarted = false;
    while (!done)
    {
        if (getWorldPtr()->getRenderState() == MayaToWorld::RSTATERENDERING)
            renderingStarted = true;
        if (renderingStarted && (getWorldPtr()->getRenderState() != MayaToWorld::RSTATERENDERING))
            done = true;
        if (escPressed && (getWorldPtr()->getRenderState() == MayaToWorld::RSTATERENDERING))
        {
            Logging::debug("computationEventThread::InterruptRequested.");
            done = true;
            Event e;
            e.type = Event::INTERRUPT;
            theRenderEventQueue()->push(e);
        }
        if (!done)
            asf::sleep(100);
        else
            Logging::debug("computationEventThread done");
    }
}

void renderProcessThread()
{
    if (getWorldPtr()->renderType == MayaToWorld::IPRRENDER)
    {
        // the idea is that the renderer waits in IPR mode for an non empty modifiesElementList,
        // it updates the render database with the elements and empties the list which is then free for the next run
        while ((getWorldPtr()->renderType == MayaToWorld::IPRRENDER))
        {
            getWorldPtr()->worldRendererPtr->render();
            while ((modifiedElementList.size() == 0) && (getWorldPtr()->renderType == MayaToWorld::IPRRENDER) && (getWorldPtr()->renderState != MayaToWorld::RSTATESTOPPED))
                asf::sleep(100);
            if ((getWorldPtr()->renderType != MayaToWorld::IPRRENDER) || (getWorldPtr()->renderState == MayaToWorld::RSTATESTOPPED))
                break;
            getWorldPtr()->worldRendererPtr->interactiveUpdateList = modifiedElementList;
            getWorldPtr()->worldRendererPtr->doInteractiveUpdate();
            modifiedElementList.clear();
        }
    }
    else
    {
        Logging::debug("RenderQueueWorker::renderProcessThread()");
        getWorldPtr()->worldRendererPtr->render();
        Logging::debug("RenderQueueWorker::renderProcessThread() - DONE.");
    }
    Event event;
    event.type = Event::FRAMEDONE;
    theRenderEventQueue()->push(event);
}

void updateRenderView(Event& e)
{
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;
    int width, height;
    renderGlobals->getWidthHeight(width, height);

    if (e.pixelData)
    {
        if (MRenderView::doesRenderEditorExist())
        {
            // we have cases where the the the render mrender view has changed but the framebuffer callback may have still the old settings.
            // here we make sure we do not exceed the renderView area.
            if (renderGlobals->getUseRenderRegion())
            {
                if ((e.tile_xmin != 0) || (e.tile_xmax != width - 1) || (e.tile_ymin != 0) || (e.tile_ymax != height - 1))
                {
                    uint left, right, bottom, top;
                    MRenderView::getRenderRegion(left, right, bottom, top);
                    if ((left != e.tile_xmin) || (right != e.tile_xmax) || (bottom != e.tile_ymin) || (top != e.tile_ymax))
                        return;
                }
            }
            MRenderView::updatePixels(e.tile_xmin, e.tile_xmax, e.tile_ymin, e.tile_ymax, e.pixelData.get());
            MRenderView::refresh(e.tile_xmin, e.tile_xmax, e.tile_ymin, e.tile_ymax);
        }
    }
}

void iprWaitForFinish(Event e)
{
    Logging::debug("iprWaitForFinish.");
    while (getWorldPtr()->getRenderState() != MayaToWorld::RSTATENONE)
        asf::sleep(100);
    Logging::debug("iprWaitForFinish - Renderstate is RSTATENONE, sending event.");
    theRenderEventQueue()->push(e);
}

namespace
{
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
}

void RenderQueueWorker::startRenderQueueWorker()
{
    Event e;
    bool terminateLoop = false;
    int pixelsChanged = 0;
    int minPixelsChanged = 500;

    MStatus status;
    while (!terminateLoop)
    {
        if (MGlobal::mayaState() != MGlobal::kBatch)
        {
            if (!theRenderEventQueue()->try_pop(e))
                break;
        }
        else
        {
            theRenderEventQueue()->wait_and_pop(e);
        }

        switch (e.type)
        {
        case Event::FINISH:
            break;

        case Event::INITRENDER:
            {
                Logging::debug("Event::InitRendering");
                if (!e.cmdArgsData)
                {
                    Logging::error("Event::InitRendering:: cmdArgsData - not defined.");
                    break;
                }
                setStartTime();
                getWorldPtr()->setRenderType(e.cmdArgsData->renderType);

                // it is possible that a new ipr rendering is started before another one is completly done, so wait for it
                if (getWorldPtr()->renderType == MayaToWorld::IPRRENDER)
                {
                    MayaToWorld::WorldRenderState rs = getWorldPtr()->getRenderState();
                    if (getWorldPtr()->getRenderState() != MayaToWorld::RSTATENONE)
                    {
                        boost::thread waitThread = boost::thread(iprWaitForFinish, e);
                        waitThread.detach();
                        break;
                    }
                }
                // Here we create the overall scene, renderer and renderGlobals objects
                getWorldPtr()->initializeRenderEnvironment();

                getWorldPtr()->worldRenderGlobalsPtr->setWidthHeight(e.cmdArgsData->width, e.cmdArgsData->height);
                getWorldPtr()->worldRenderGlobalsPtr->setUseRenderRegion(e.cmdArgsData->useRenderRegion);
                getWorldPtr()->worldScenePtr->uiCamera = e.cmdArgsData->cameraDagPath;
                getWorldPtr()->worldRendererPtr->initializeRenderer(); // init renderer with all type, image size etc.
                doPreRenderJobs();

                int width, height;
                getWorldPtr()->worldRenderGlobalsPtr->getWidthHeight(width, height);
                if (MRenderView::doesRenderEditorExist())
                {
                    MRenderView::endRender();
                    status = MRenderView::startRender(width, height, true, true);
                }
                getWorldPtr()->setRenderState(MayaToWorld::RSTATETRANSLATING);
                boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;

                if (MGlobal::mayaState() != MGlobal::kBatch)
                {
                    // we only need renderComputation (means esc-able rendering) if we render in UI (==NORMAL)
                    if (getWorldPtr()->getRenderType() == MayaToWorld::UIRENDER)
                        beginComputation();
                }
                e.type = Event::FRAMERENDER;
                theRenderEventQueue()->push(e);

                if (MGlobal::mayaState() != MGlobal::kBatch)
                {
                    boost::thread cet = boost::thread(computationEventThread);
                    cet.detach();
                }

                // calculate numtiles
                int numTX = (int)ceil((float)width/(float)getWorldPtr()->worldRenderGlobalsPtr->tilesize);
                int numTY = (int)ceil((float)height/(float)getWorldPtr()->worldRenderGlobalsPtr->tilesize);
                numTiles = numTX * numTY;
                tilesDone = 0;

                break;
            }

        case Event::FRAMERENDER:
            {
                Logging::debug("Event::Framerender");

                if (sceneThread.joinable())
                    sceneThread.join();

                if (!getWorldPtr()->worldRenderGlobalsPtr->frameListDone())
                {
                    getWorldPtr()->worldRenderGlobalsPtr->updateFrameNumber();
                    doPreFrameJobs(); // preRenderScript etc.
                    doPrepareFrame(); // parse scene and update objects
                    doRenderPreFrameJobs(); // call renderers pre frame jobs
                    sceneThread = boost::thread(renderProcessThread);
                }
                else
                {
                    e.type = Event::RENDERDONE;
                    theRenderEventQueue()->push(e);
                }
            }
            break;

        case Event::STARTRENDER:
            {
                Logging::debug("Event::Startrender");
                break;
            }
        case Event::RENDERERROR:
            {
                e.type = Event::RENDERDONE;
                theRenderEventQueue()->push(e);
                getWorldPtr()->setRenderState(MayaToWorld::RSTATEERROR);
            }
            break;

        case Event::FRAMEDONE:
            Logging::debug("Event::FRAMEDONE");
            doPostFrameJobs();
            updateRenderView(e);
            e.type = Event::FRAMERENDER;
            theRenderEventQueue()->push(e);
            break;

        case Event::RENDERDONE:
            {
                Logging::debug("Event::RENDERDONE");

                if (MGlobal::mayaState() != MGlobal::kBatch)
                    endComputation();

                if (getWorldPtr()->renderType == MayaToWorld::IPRRENDER)
                {
                    removeCallbacks();
                }
                getWorldPtr()->setRenderState(MayaToWorld::RSTATEDONE);
                terminateLoop = true;

                setEndTime();
                if (MRenderView::doesRenderEditorExist())
                {
                    MString captionString = getCaptionString();
                    MString captionCmd = MString("import pymel.core as pm;pm.renderWindowEditor(\"renderView\", edit=True, pcaption=\"") + captionString + "\");";
                    Logging::debug(captionCmd);
                    MGlobal::executePythonCommandOnIdle(captionCmd);

                    // clean the queue
                    while (RenderEventQueue.try_pop(e))
                    {
                    }
                }

                doPostRenderJobs();

                getWorldPtr()->cleanUpAfterRender();
                getWorldPtr()->worldRendererPtr->unInitializeRenderer();
                getWorldPtr()->setRenderState(MayaToWorld::RSTATENONE);
            }
            break;

        case Event::FRAMEUPDATE:
            updateRenderView(e);
            break;

        case Event::IPRUPDATE:
            Logging::debug("Event::IPRUPDATE - whole frame");
            updateRenderView(e);
            break;

        case Event::INTERRUPT:
            Logging::debug("Event::INTERRUPT");
            getWorldPtr()->worldRendererPtr->abortRendering();
            break;

        case Event::IPRSTOP:
            Logging::debug("Event::IPRSTOP");
            getWorldPtr()->setRenderState(MayaToWorld::RSTATESTOPPED);
            getWorldPtr()->worldRendererPtr->abortRendering();
            break;

        case Event::TILEDONE:
            {
                Logging::debug(MString("Event::TILEDONE - queueSize: ") + theRenderEventQueue()->size());
                updateRenderView(e);
                if (getWorldPtr()->renderType != MayaToWorld::IPRRENDER)
                {
                    tilesDone++;
                    float percentDone = ((float)tilesDone / (float)numTiles) * 100.0;
                    Logging::progress(MString("") + (int)percentDone + "% done");
                }
            }
            break;

        case Event::PIXELSDONE:
            break;

        case Event::PRETILE:
            updateRenderView(e);
            break;

        case Event::IPRUPDATESCENE:
            {
                Logging::debug("Event::IPRUPDATESCENE");
            }
            break;

        case Event::INTERACTIVEFBCALLBACK:
            Logging::debug("Event::INTERACTIVEFBCALLBACK");
            getWorldPtr()->worldRendererPtr->interactiveFbCallback();
            break;

        case Event::ADDIPRCALLBACKS:
            Logging::debug("Event::ADDIPRCALLBACKS");
            addIPRCallbacks();
            break;

        case Event::USER:
            {
                Logging::debug("Event::USER");
                if (!e.cmdArgsData)
                {
                    Logging::error("Event::USER:: cmdArgsData - not defined.");
                    break;
                }
                if (getWorldPtr()->worldRendererPtr == 0)
                {
                    Logging::error("Event::USER:: no renderer defined. Please render an image.");
                }
                else
                {
                    getWorldPtr()->worldRendererPtr->handleUserEvent(e.cmdArgsData->userEvent, e.cmdArgsData->userDataString, e.cmdArgsData->userDataFloat, e.cmdArgsData->userDataInt);
                }
            }
            break;
        }

        if (MGlobal::mayaState() != MGlobal::kBatch)
            break;
    }
}

RenderQueueWorker::~RenderQueueWorker()
{
    // Empty the queue.
    Event e;
    while (RenderEventQueue.try_pop(e)) {}
}
