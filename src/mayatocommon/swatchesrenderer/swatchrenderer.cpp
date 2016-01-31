
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

#include "swatchrenderer.h"
#include <maya/MImage.h>
#include <maya/MGlobal.h>
#include <assert.h>
#include <string.h>
#include <string>
#include <algorithm>

#include "utilities/tools.h"
#include "utilities/logging.h"
#include "swatchrendererinterfacefactory.h"
#include "../world.h"

MSwatchRenderBase* SwatchRenderer::creator(MObject dependNode, MObject renderNode, int imageResolution)
{
    return new SwatchRenderer(dependNode, renderNode, imageResolution);
}

SwatchRenderer::SwatchRenderer(MObject dependNode, MObject renderNode, int imageResolution) : MSwatchRenderBase(dependNode, renderNode, imageResolution)
{
    Logging::debug("Create Swatch Renderer.");
    this->renderInterface = SwatchRendererInterfaceFactory().createSwatchRendererInterface(dependNode, renderNode, imageResolution);
}

SwatchRenderer::~SwatchRenderer()
{
    if (this->renderInterface != 0)
        SwatchRendererInterfaceFactory().deleteSwatchRendererInterface(this->renderInterface);
    this->renderInterface = 0;
}

bool SwatchRenderer::doIteration()
{
    MStatus status;
    image().create(resolution(), resolution(), 4, MImage::kFloat);

    // if another render process is rendering then...
    MayaTo::MayaToWorld::WorldRenderState rState = MayaTo::getWorldPtr()->getRenderState();
    MayaTo::MayaToWorld::WorldRenderType rType = MayaTo::getWorldPtr()->getRenderType();
    if ((rState == MayaTo::MayaToWorld::RSTATERENDERING) || (rType == MayaTo::MayaToWorld::IPRRENDER))
    {
        this->renderInterface->getImageData(this->image()); // copy empty image
        image().convertPixelFormat(MImage::kByte);
        return false;
    }
    MayaTo::getWorldPtr()->setRenderType(MayaTo::MayaToWorld::SWATCHRENDER);
    MayaTo::getWorldPtr()->setRenderState(MayaTo::MayaToWorld::RSTATESWATCHRENDERING);
    this->renderInterface->renderSwatch();
    this->renderInterface->getImageData(this->image());
    image().convertPixelFormat(MImage::kByte);
    MayaTo::getWorldPtr()->setRenderState(MayaTo::MayaToWorld::RSTATENONE);
    MayaTo::getWorldPtr()->setRenderType(MayaTo::MayaToWorld::RTYPENONE);
    return true;
}
