
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

#ifndef RENDERER_H
#define RENDERER_H

#include <memory>
#include <vector>
#include <maya/MString.h>
#include "definitions.h"

class MayaObject;
class MObject;
struct InteractiveElement;

class Renderer
{
  public:
    std::vector<InteractiveElement *>  interactiveUpdateList;
    virtual ~Renderer() {};
    virtual void defineCamera() = 0;
    virtual void defineEnvironment() = 0;
    virtual void defineGeometry() = 0;
    virtual void defineLights() = 0;
    // interactiveFbCallback can be called after a rendering is finished and
    // the user wants to modify the color mapping of the final rendering.
    // all interactive callbacks should be pushed into the worker queue with INTERACTIVEFBCALLBACK so that we don't get
    // any race conditions if the callback takes longer than the next call.
    virtual void interactiveFbCallback() = 0;

    virtual void render() = 0;
    // initializeRenderer is called before rendering starts
    // it should prepare all data which can/should be reused during
    // ipr/frame/sequence rendering
    virtual void initializeRenderer() = 0;
    // unInitializeRenderer is called at the end of rendering.
    virtual void unInitializeRenderer() = 0;
    // updateShape is called during scene updating. If a renderer can update motionblur steps on the fly,
    // the geometry is defined at the very first step and later this definition will be updated for every motion step.
    // Other renderers will need a complete definition of all motionblur steps at once, so the motion steps will be
    // in the geometry e.g. with obj->addMeshData(); and at the very last step, everything is defined.
    virtual void updateShape(sharedPtr<MayaObject> obj) = 0;
    // This method is necessary only if the renderer is able to update the transform definition interactively.
    // In other cases, the world space transform will be defined directly during the creation of the geometry.
    virtual void updateTransform(sharedPtr<MayaObject> obj) = 0;
    virtual void abortRendering() = 0;
    // make an interactive update of the scene. Before this call the interactiveUpdateList should be filled appropriatly
    virtual void doInteractiveUpdate() = 0;
    virtual void handleUserEvent(int event, MString strData, float floatData, int intData) = 0;
    virtual void preFrame(){};
    virtual void postFrame(){};
};

#endif
