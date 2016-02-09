
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

#ifndef WORLD_H
#define WORLD_H

// appleseed-maya headers.
#include "utilities/tools.h"
#include "appleseedrenderer.h"

// Maya headers.
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MImage.h>
#include <maya/MTimerMessage.h>
#include <maya/MDistance.h>

// Standard headers.
#include <memory>

// Forward declarations.
class AppleseedSwatchRenderer;
class MayaScene;
class RenderGlobals;

class World
{
  public:
    World();
    ~World();

    boost::shared_ptr<MayaScene> mScene;
    boost::shared_ptr<AppleseedRenderer> mRenderer;
    boost::shared_ptr<RenderGlobals> mRenderGlobals;

    MStringArray shaderSearchPath;

    void initializeRenderEnvironment();
    void destroyMayaScene();

    enum RenderType
    {
        RTYPENONE = 0,
        UIRENDER,
        IPRRENDER
    };

    void setRenderType(RenderType type);
    RenderType getRenderType();

    enum RenderState
    {
        RSTATENONE = 0,
        RSTATERENDERING,
        RSTATESTOPPED,
        RSTATEDONE,
        RSTATETRANSLATING
    };

    void setRenderState(RenderState state);
    RenderState getRenderState();

    AppleseedSwatchRenderer* getSwatchRenderer();

  private:
    RenderType                              mRenderType;
    RenderState                             mRenderState;
    std::auto_ptr<AppleseedSwatchRenderer>  mSwatchRenderer;
};

void deleteWorld();
void defineWorld();
World* getWorldPtr();

#endif  // !WORLD_H
