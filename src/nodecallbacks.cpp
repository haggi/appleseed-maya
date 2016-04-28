
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
#include "nodecallbacks.h"

// appleseed-maya headers.
#include "utilities/logging.h"
#include "appleseedutils.h"
#include "mayascene.h"
#include "renderqueue.h"
#include "world.h"

// Maya headers.
#include <maya/MItDag.h>
#include <maya/MPlug.h>

// Boost headers.
#include <boost/shared_ptr.hpp>

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

void IPRAttributeChangedCallback(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* userPtr)
{
    Logging::debug(MString("IPRAttributeChangedCallback. attribA: ") + plug.name() + " attribB: " + otherPlug.name());
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

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
                EditableElement& element = mayaScene->editableElements[MMessage::currentCallbackId()];
                element.isDirty = true;
                mayaScene->isAnyDirty = true;
                element.isDeformed = true; // trigger a complete mesh update
            }
        }
    }
    else if (msg & MNodeMessage::kConnectionBroken)
    {
        Logging::debug(MString("IPRAttributeChangedCallback. connection broken."));
    }
}

void IPRNodeDirtyCallback(void* userPtr)
{
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

    MCallbackId callbackId = MMessage::currentCallbackId();
    EditableElement& element = mayaScene->editableElements[callbackId];
    element.isDirty = true;

    mayaScene->isAnyDirty = true;
}

namespace
{
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
                                if (childIter.currentItem() == j->second.node)
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
    }
}

void IPRIdleCallback(float time, float lastTime, void* userPtr)
{
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;

    if (!mayaScene->isAnyDirty)
        return;

    stopRendering();

    markTransformsChildrenAsDirty();
    getWorldPtr()->mRenderer->applyInteractiveUpdates(mayaScene->editableElements);

    for (MayaScene::EditableElementContainer::iterator
            i = mayaScene->editableElements.begin(),
            e = mayaScene->editableElements.end(); i != e; ++i)
    {
        i->second.isDirty = false;
        i->second.isTransformed = false;
    }

    mayaScene->isAnyDirty = false;

    startRendering();
}

// Register new created nodes. We need the transform and the shape node to correctly use it in IPR.
// So we simply use the shape node, get it's parent - a shape node and let the scene parser do the rest.
// Then add a node dirty callback for the new elements. By adding the callback ids to the idInteractiveMap, the
// IPR should detect a modification during the netxt update cycle.
// Interestingly, if an object is duplicated, it first receives a prefix called "__PrenotatoPerDuplicare_", then later
// the geometry will be renamed to the correct name.

// Handling of surface shaders is a bit different. A shader is not assigned directly to a surface but it is connected to a shading group
// which is nothing else but a objectSet. If a new surface shader is created, it is not in use until it is assigned to an object what means it is connected
// to a shading group. So I simply add a shadingGroup callback for new shading groups.

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
        if (!dagPath.node().hasFn(MFn::kMesh))
            return;
    }
    else
    {
        if (!node.hasFn(MFn::kShape))
            return;
    }
    MFnDagNode dagNode(node);
    MDagPath transformPath;
    dagNode.getPath(transformPath);
    MDagPath dagPath = transformPath;
    transformPath.pop();

    // Here the new object and its children are added to the object list and to the interactive object list.
    mayaScene->parseSceneHierarchy(transformPath, 0, boost::shared_ptr<ObjectAttributes>(), boost::shared_ptr<MayaObject>());

    for (MayaScene::EditableElementContainer::iterator
        i = mayaScene->editableElements.begin(),
        e = mayaScene->editableElements.end(); i != e; ++i)
    {
        if (i->second.mayaObject == 0)
            continue;
        if (i->second.mayaObject->dagPath == dagPath)
        {
            i->second.isDirty = true;
            i->second.isDeformed = true;
            mayaScene->isAnyDirty = true;
            break;
        }
    }

}

void IPRNodeRenamedCallback(MObject& node, const MString& oldName, void* userPtr)
{
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;
    for (MayaScene::EditableElementContainer::iterator
        i = mayaScene->editableElements.begin(),
        e = mayaScene->editableElements.end(); i != e; ++i)
    {
        if (i->second.mayaObject != 0)
        {
            if (i->second.node == node)
            {
                break;
            }
        }
    }

}

void IPRNodeRemovedCallback(MObject& node, void* userPtr)
{
    // Find the MayaObject and mark it as removed.
    boost::shared_ptr<MayaScene> mayaScene = getWorldPtr()->mScene;
    std::vector<MCallbackId> removeIds;
    for (MayaScene::EditableElementContainer::iterator
            i = mayaScene->editableElements.begin(),
            e = mayaScene->editableElements.end(); i != e; ++i)
    {
        EditableElement& element = i->second;
        if (element.node == node)
        {
            MNodeMessage::removeCallback(i->first);
            removeIds.push_back(i->first);
            if (element.mayaObject)
            {
                element.mayaObject->removed = true;
                element.isDirty = true;
                element.isDeformed = true; // trigger mesh update
                mayaScene->isAnyDirty = true;
            }
            break;
        }
    }

}
