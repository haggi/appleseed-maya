
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

#ifndef EVENT_H
#define EVENT_H

// appleseed-maya headers.
#include "world.h"

// Maya headers.
#include <maya/MDagPath.h>
#include <maya/MRenderView.h>

// Boost headers.
#include "boost/shared_ptr.hpp"

// Standard headers.
#include <cstddef>

class Event
{
  public:
    enum Type
    {
        INTERRUPT = 0,
        TILEDONE = 1,
        FRAMEDONE = 2,
        RENDERDONE = 3,
        IPRSTART = 6,
        IPRSTOP = 7,
        IPRPAUSE = 8,
        IPRREMOVE = 9,
        IPRFRAMEDONE = 11,
        UPDATEUI = 17,
        INITRENDER = 19,
        ADDIPRCALLBACKS = 21,
        FRAMERENDER = 22
    };

    Type type;

    int width;
    int height;
    bool useRenderRegion;
    MDagPath cameraDagPath;
    World::RenderType renderType;
    size_t tile_xmin, tile_xmax, tile_ymin, tile_ymax;

    boost::shared_ptr<RV_PIXEL> pixelData;

    Event()
    {
        type = INTERRUPT;
    }
};

#endif  // !EVENT_H
