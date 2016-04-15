
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
#include "renderglobals.h"
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
#include <algorithm>
#include <ctime>
#include <map>
#include <vector>

namespace
{
    MCallbackId idleCallbackId = 0;
    MCallbackId nodeAddedCallbackId = 0;
    MCallbackId nodeRemovedCallbackId = 0;
    std::vector<MCallbackId> callbacksToDelete;
    std::vector<InteractiveElement*> modifiedElementList;   // todo: not thread-safe!
    clock_t renderStartTime = 0;
    clock_t renderEndTime = 0;
    bool IprCallbacksDone = false;
    std::map<MCallbackId, MObject> objIdMap;
    std::map<MCallbackId, InteractiveElement*> idInteractiveMap;
    size_t numPixelsDone;
    size_t numPixelsTotal;

    boost::thread renderThread;
    concurrent_queue<Event> renderEventQueue;
}

concurrent_queue<Event>* gEventQueue()
{
    return &renderEventQueue;
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
            if ((xMax < left) || (xMin > right) || (yMax < bottom) || (yMin > top))
                return;
            xmin = std::max(xmin, left);
            xmax = std::min(xmax, right);
            ymin = std::max(ymin, bottom);
            ymax = std::min(ymax, top);
        }
        RV_PIXEL line[4];
        for (uint x = 0; x < 4; x++)
        {
            line[x].r = line[x].g = line[x].b = line[x].a = 1.0f;
        }
        MRenderView::updatePixels(xmin, xmin + 3, ymin, ymin, line, true);
        MRenderView::updatePixels(xmax - 3, xmax, ymin, ymin, line, true);
        MRenderView::updatePixels(xmin, xmin + 3, ymax, ymax, line, true);
        MRenderView::updatePixels(xmax - 3, xmax, ymax, ymax, line, true);
        MRenderView::updatePixels(xmin, xmin, ymin, ymin + 3, line, true);
        MRenderView::updatePixels(xmin, xmin, ymax - 3, ymax, line, true);
        MRenderView::updatePixels(xmax, xmax, ymin, ymin + 3, line, true);
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

    void renderProcessThread()
    {
        getWorldPtr()->mRenderer->render();
        Event event;
        event.mType = Event::RENDERDONE;
        gEventQueue()->push(event);
    }

    // one problem: In most cases the renderer translates shapes only not complete hierarchies
    // To do a correct update of all interesting nodes, I have to find the shapes below a transform node.
    // Here we fill the modifiedElementList with shape nodes and non transform nodes

    void iprFindLeafNodes()
    {
        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;
        std::map<MCallbackId, InteractiveElement*>::iterator it;
        std::vector<InteractiveElement*> leafList;

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
                                    InteractiveElement* ie = &mayaScene->interactiveUpdateMap[seIt->first];
                                    ie->triggeredFromTransform = true;
                                    modifiedElementList.push_back(&mayaScene->interactiveUpdateMap[seIt->first]);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                modifiedElementList.push_back(it->second);
            }
        }
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
    //      a scene message is created - we have to stop everything as soon as the plugin will be removed, otherwise Maya will crash.
    //      IMPORTANT:  We do add dirty callbacks for translated nodes only which are saved in the mayaScene::interactiveUpdateMap.
    //                  This map is filled by sceneParsing and shader translation process which are called before rendering and during geometry translation.
    //                  So the addIPRCallbacks() has to be called after everything is translated.

    void IPRAttributeChangedCallback(MNodeMessage::AttributeMessage msg, MPlug & plug,  MPlug & otherPlug, void *element)
    {
        Logging::debug(MString("IPRAttributeChangedCallback. attribA: ") + plug.name() + " attribB: " + otherPlug.name());
        InteractiveElement *userData = (InteractiveElement *)element;
        boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

        if (!userData->obj)
            return;

        if (msg & MNodeMessage::kConnectionMade)
        {
            Logging::debug(MString("IPRAttributeChangedCallback. connection created."));
            MString plugName = plug.name();
            std::string pn = plugName.asChar();
            if (pn.find("instObjGroups[") != std::string::npos)
            {
                Logging::debug(MString("IPRAttributeChangedCallback. InstObjGroups affected, checking other side."));
                if (otherPlug.node().hasFn(MFn::kShadingEngine))
                {
                    Logging::debug(MString("IPRAttributeChangedCallback. Found shading group on the other side: ") + getObjectName(otherPlug.node()));
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
            Logging::debug(MString("IPRAttributeChangedCallback. connection broken."));
        }
    }

    void IPRNodeDirtyCallback(void* interactiveElement)
    {
        MCallbackId thisId = MMessage::currentCallbackId();
        idInteractiveMap[thisId] = static_cast<InteractiveElement*>(interactiveElement);
    }

    void IPRIdleCallback(float time, float lastTime, void* userPtr)
    {
        if (idInteractiveMap.empty())
            return;

        getWorldPtr()->mRenderer->abortRendering();
        renderThread.join();
        iprFindLeafNodes();
        getWorldPtr()->mRenderer->applyInteractiveUpdates(modifiedElementList);
        modifiedElementList.clear();
        idInteractiveMap.clear();
        renderThread = boost::thread(renderProcessThread);
    }

    void IPRNodeAddedCallback(MObject& node, void* userPtr)
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
                    id = MNodeMessage::addAttributeChangedCallback(ie.node, IPRAttributeChangedCallback, userData, &stat);
                    objIdMap[id] = ie.node;
                    if (stat)
                        callbacksToDelete.push_back(id);
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

        // Add some global dependency nodes to the update list.
        InteractiveElement iel;
        iel.mobj = getRenderGlobalsNode();
        iel.name = getObjectName(iel.mobj);
        iel.node = iel.mobj;
        mayaScene->interactiveUpdateMap[mayaScene->interactiveUpdateMap.size()] = iel;

        for (std::map<uint, InteractiveElement>::iterator
                 i = mayaScene->interactiveUpdateMap.begin(),
                 e = mayaScene->interactiveUpdateMap.end();
             i != e; ++i)
        {
            uint elementId = i->first;
            InteractiveElement iae = i->second;
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
            InteractiveElement* userData = &mayaScene->interactiveUpdateMap[elementId];
            MCallbackId id = MNodeMessage::addNodeDirtyCallback(nodeDirty, IPRNodeDirtyCallback, userData, &stat);
            objIdMap[id] = nodeDirty;
            if (stat)
                callbacksToDelete.push_back(id);

            if (nodeDirty.hasFn(MFn::kMesh))
            {
                MString nd = getObjectName(nodeDirty);
                id = MNodeMessage::addAttributeChangedCallback(nodeDirty, IPRAttributeChangedCallback, userData, &stat);
                objIdMap[id] = nodeDirty;
                if (stat)
                    callbacksToDelete.push_back(id);
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

        for (std::vector<MCallbackId>::iterator i = callbacksToDelete.begin(); i != callbacksToDelete.end(); i++)
            MMessage::removeCallback(*i);

        callbacksToDelete.clear();
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
        const float percent = (float)pixelsDone / pixelsTotal;
        if (((int)(percent * 100) % 5) == 0)
        {
            char perc[16];
            sprintf(perc, "%3.2f", percent * 100.0f);
            MString progressStr;
            progressStr.format("^1s% done.", MString(perc));
            Logging::info(progressStr);
            MString cmd = MString("import appleseed_maya.initialize; appleseed_maya.initialize.theRenderer().updateProgressBar(") + perc + ")";
            MGlobal::executePythonCommand(cmd);
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
            addIPRCallbacks();
        renderThread = boost::thread(renderProcessThread);
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
        finishRender();
    }
}

void finishRender()
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
        removeCallbacks();

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
    getWorldPtr()->mRenderer->abortRendering();
    renderThread.join();
    unsigned int left, right, bottom, top;
    MRenderView::getRenderRegion(left, right, bottom, top);
    foundation::AABB2u crop(foundation::AABB2u::VectorType(left, bottom), foundation::AABB2u::VectorType(right, top));
    getWorldPtr()->mRenderer->getProjectPtr()->get_frame()->set_crop_window(crop);
    renderThread = boost::thread(renderProcessThread);
}


void RenderQueueWorker::startRenderQueueWorker()
{
    Event e;
    if (!gEventQueue()->try_pop(e))
        return;

    switch (e.mType)
    {
        case Event::RENDERDONE:
        {
            if (getWorldPtr()->getRenderType() != World::IPRRENDER)
                finishRender();
        }
        break;

        case Event::UPDATEUI:
        {
            updateRenderView(e.xMin, e.xMax, e.yMin, e.yMax, e.pixels.get());
            numPixelsDone += (e.xMax - e.xMin) * (e.yMax - e.yMin);
            if (getWorldPtr()->getRenderType() != World::IPRRENDER)
                logOutput(numPixelsDone, numPixelsTotal);
        }
        break;

        case Event::PRETILE:
        {
            preTileRenderView(e.xMin, e.xMax, e.yMin, e.yMax);
        }
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
        InteractiveElement* element = &mayaScene->interactiveUpdateMap[elementId];
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
                callbacksToDelete.push_back(id);
        }
    }
}

bool RenderQueueWorker::IPRCallbacksDone()
{
    return IprCallbacksDone;
}
