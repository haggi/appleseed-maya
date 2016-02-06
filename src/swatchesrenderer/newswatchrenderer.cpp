
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

#include "swatchesrenderer/newswatchrenderer.h"
#include "appleseedswatchrenderer.h"
#include "swatchesevent.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "mayatoworld.h"

NewSwatchRenderer::NewSwatchRenderer(MObject dependNode, MObject renderNode, int imageResolution)
  : MSwatchRenderBase(dependNode, renderNode, imageResolution)
  , renderInProgress(true)
  , swatchRenderingDone(false)
  , rNode(renderNode)
  , dNode(dependNode)
{
}

bool NewSwatchRenderer::renderParallel()
{
    return false;
}

void NewSwatchRenderer::finishParallelRender()
{
    Logging::debug(MString("finishParallelRender called."));
}

void NewSwatchRenderer::cancelParallelRendering()
{
    Logging::debug(MString("cancelParallelRendering called."));
}

MSwatchRenderBase* NewSwatchRenderer::creator(MObject dependNode, MObject renderNode, int imageResolution)
{
    return new NewSwatchRenderer(dependNode, renderNode, imageResolution);
}

bool NewSwatchRenderer::doIteration()
{
    if (getWorldPtr()->getRenderType() == MayaToWorld::IPRRENDER)
    {
        image().create(resolution(), resolution(), 4, MImage::kFloat);
        image().convertPixelFormat(MImage::kByte);
        return true;
    }

    AppleseedSwatchRenderer* swatchRenderer =
        static_cast<AppleseedSwatchRenderer*>(getObjPtr("appleseedSwatchesRenderer"));

    if (swatchRenderer)
    {
        if (swatchRenderer->mrenderer.get() != 0)
        {
            swatchRenderer->renderSwatch(this);
            image().convertPixelFormat(MImage::kByte);
        }
        else
        {
            image().create(resolution(), resolution(), 4, MImage::kFloat);
            image().convertPixelFormat(MImage::kByte);
        }
        return true;
    }

    return false;
}
