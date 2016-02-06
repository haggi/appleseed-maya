
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

#ifndef TILECALLBACK_H
#define TILECALLBACK_H

// appleseed.renderer headers.
#include "renderer/api/rendering.h"

// appleseed.foundation headers.
#include "foundation/platform/compiler.h"

// Standard headers.
#include <cstddef>

// Forward declarations.
namespace foundation    { class Tile; }
namespace renderer      { class Frame; }

class TileCallback
  : public renderer::ITileCallback
{
  public:
    // Delete this instance.
    virtual void release() APPLESEED_OVERRIDE;

    // This method is called before a region is rendered.
    virtual void pre_render(
        const size_t            x,
        const size_t            y,
        const size_t            width,
        const size_t            height) APPLESEED_OVERRIDE;

    // This method is called after a whole frame is rendered (at once).
    virtual void post_render(
        const renderer::Frame*  frame) APPLESEED_OVERRIDE;

    virtual void post_render_tile(
        const renderer::Frame*  frame,
        const size_t            tile_x,
        const size_t            tile_y) APPLESEED_OVERRIDE;
};

class TileCallbackFactory
  : public renderer::ITileCallbackFactory
{
  public:
    // Delete this instance.
    virtual void release() APPLESEED_OVERRIDE;

    virtual renderer::ITileCallback* create() APPLESEED_OVERRIDE;
};

#endif  // !TILECALLBACK_H
