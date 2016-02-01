
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

#ifndef RENDER_QUEUE_H
#define RENDER_QUEUE_H

#include <functional>
#include <maya/MRenderView.h>
#include <maya/MNodeMessage.h>
#include <map>
#include "queue.h"

#include "boost/function.hpp"

static concurrent_queue<Event> RenderEventQueue;
concurrent_queue<Event> *theRenderEventQueue();

struct Callback
{
    Callback()
    {
        millsecondInterval = 100;
        callbackId = 0;
        terminate = false;
    }
    unsigned int millsecondInterval;
    boost::function<void()> functionPointer;
    unsigned int callbackId;
    bool terminate;
};

class RenderQueueWorker
{
  public:
    RenderQueueWorker();
    ~RenderQueueWorker();
    static void addIPRCallbacks();
    static void removeCallbacks();
    static void startRenderQueueWorker();
    static void renderQueueWorkerTimerCallback(float time, float lastTime, void *userPtr);
    static void IPRIdleCallback(float time, float lastTime, void *userPtr);
    static void IPRNodeDirtyCallback(void *userPtr);
    static void renderQueueWorkerIdleCallback(float time, float lastTime, void *userPtr);
    static void IPRattributeChangedCallback(MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void*);
    static void addIdleUIComputationCreateCallback(void* data);
    static void addIdleUIComputationCallback();
    static void IPRUpdateCallbacks();
    static void IPRNodeAddedCallback(MObject& node, void *userPtr);
    static void IPRNodeRemovedCallback(MObject& node, void *userPtr);
    static void sceneCallback(void *);
    static void pluginUnloadCallback(void *);
    static void computationEventThread();
    static boost::thread sceneThread;
    static void renderProcessThread();
    static void sendFinalizeIfQueueEmpty(void *);
    static void setStartTime();
    static void setEndTime();
    static MString getElapsedTimeString();
    static MString getCaptionString();
    static void updateRenderView(Event& e);
    static size_t registerCallback(boost::function<void()> function, unsigned int millisecondsUpdateInterval = 100);
    static void unregisterCallback(size_t cbId);
    static void callbackWorker(size_t cbId);
    static bool iprCallbacksDone();
    static void iprFindLeafNodes();
    static void iprWaitForFinish(Event e);

    static void interactiveStartThread();
};

struct RandomPixel
{
    RV_PIXEL pixel;
    int x, y;
};

std::vector<MObject> *getModifiedObjectList();

void setStartComputation();
void setEndComputation();

#endif
