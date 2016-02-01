
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

#include "mtap_swatchrenderer.h"

#include <string.h>

#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"

mtap_SwatchRendererInterface::mtap_SwatchRendererInterface(MObject dependNode, MObject renderNode, int imageResolution)
{
    this->imgDone = false;
    this->inProgress = false;
    this->dependNode = dependNode;
    this->renderNode = renderNode;
    this->swatchWidth = imageResolution;
    this->imageData = new float[this->swatchWidth * this->swatchWidth];
    memset(this->imageData, 0, sizeof(float)* this->swatchWidth * this->swatchWidth * 4);
}

mtap_SwatchRendererInterface::~mtap_SwatchRendererInterface()
{
    delete [] this->imageData;
}

void mtap_SwatchRendererInterface::init()
{
}

void mtap_SwatchRendererInterface::loadGeometry()
{
}

void mtap_SwatchRendererInterface::fillDummySwatch(MImage& image)
{
    const int res(swatchWidth);
    const float rndR = rnd();
    const float rndG = rnd();
    const float rndB = rnd();

    float* pixels = image.floatPixels();
    size_t index = 0;

    for (int y = 0; y < res; y++)
    {
        for (int x = 0; x < res; x++)
        {
            const float fac = static_cast<float>(y) / res;
            pixels[index++] = fac * rndR;
            pixels[index++] = fac * rndG;
            pixels[index++] = fac * rndB;
            pixels[index++] = 1.0f;
        }
    }
}

void mtap_SwatchRendererInterface::getImageData(MImage& imageRef)
{
    fillDummySwatch(imageRef);
}

void mtap_SwatchRendererInterface::renderSwatch()
{
}

void mtap_SwatchRendererInterface::getImage(MImage& imageRef)
{
}
