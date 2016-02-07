
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
#include "tilecallback.h"

// appleseed-maya headers.
#include "renderqueueworker.h"

// appleseed.renderer headers.
#include "renderer/api/frame.h"

// appleseed.foundation headers.
#include "foundation/image/canvasproperties.h"
#include "foundation/image/image.h"
#include "foundation/image/pixel.h"
#include "foundation/image/tile.h"
#include "foundation/math/scalar.h"

// Maya headers.
#include <maya/MRenderView.h>

// Standard headers.
#include <cassert>

void TileCallback::release()
{
    delete this;
}

void TileCallback::pre_render(
    const size_t            x,
    const size_t            y,
    const size_t            width,
    const size_t            height)
{
}

void TileCallback::post_render(const renderer::Frame* frame)
{
    const foundation::CanvasProperties& frameProps = frame->image().properties();

    boost::shared_ptr<RV_PIXEL> pixels(new RV_PIXEL[frameProps.m_pixel_count]);
    RV_PIXEL* pixelsPtr = pixels.get();

    for (size_t i = 0; i < frameProps.m_pixel_count; i++)
    {
        pixelsPtr[i].r = 255.0f;
        pixelsPtr[i].g = 0.0f;
        pixelsPtr[i].b = 0.0f;
        pixelsPtr[i].a = 0.0f;
    }

    foundation::Tile float_tile_storage(
        frameProps.m_tile_width,
        frameProps.m_tile_height,
        frameProps.m_channel_count,
        foundation::PixelFormatFloat);

    for (size_t tile_y = 0; tile_y < frameProps.m_tile_count_y; tile_y++)
    {
        for (size_t tile_x = 0; tile_x < frameProps.m_tile_count_x; tile_x++)
        {
            const foundation::Tile& tile = frame->image().tile(tile_x, tile_y);
            const size_t tileWidth = tile.get_width();
            const size_t tileHeight = tile.get_height();

            // Tile with the right pixel format in the right color space.
            foundation::Tile finalTile(
                tile,
                foundation::PixelFormatFloat,
                float_tile_storage.get_storage());
            frame->transform_to_output_color_space(finalTile);

            for (size_t y = 0; y < tileHeight; y++)
            {
                for (size_t x = 0; x < tileWidth; x++)
                {
                    const size_t pixelIndex =
                        frameProps.m_canvas_width * (frameProps.m_canvas_height - tile_y * frameProps.m_tile_height - y - 1) +
                        tile_x * frameProps.m_tile_width +
                        x;
                    assert(pixelIndex < frameProps.m_pixel_count);

                    const float* source = reinterpret_cast<const float*>(finalTile.pixel(x, y));
                    pixelsPtr[pixelIndex].r = foundation::saturate(source[0]) * 255.0f;
                    pixelsPtr[pixelIndex].g = foundation::saturate(source[1]) * 255.0f;
                    pixelsPtr[pixelIndex].b = foundation::saturate(source[2]) * 255.0f;
                    pixelsPtr[pixelIndex].a = foundation::saturate(source[3]) * 255.0f;
                }
            }
        }
    }

    Event e;
    e.mType = Event::TILEDONE;
    e.pixelData = pixels;
    e.tile_xmin = 0;
    e.tile_ymin = 0;
    e.tile_xmax = frameProps.m_canvas_width - 1;
    e.tile_ymax = frameProps.m_canvas_height - 1;
    gEventQueue()->push(e);
}

void TileCallback::post_render_tile(
    const renderer::Frame*  frame,
    const size_t            tile_x,
    const size_t            tile_y)
{
    const foundation::Image& image = frame->image();
    const foundation::CanvasProperties& frameProps = image.properties();
    const foundation::Tile& tile = image.tile(tile_x, tile_y);
    const size_t tileWidth = tile.get_width();
    const size_t tileHeight = tile.get_height();

    // Tile with the right pixel format in the right color space.
    foundation::Tile finalTile(tile, foundation::PixelFormatFloat);
    frame->transform_to_output_color_space(finalTile);

    boost::shared_ptr<RV_PIXEL> pixels(new RV_PIXEL[tileWidth * tileHeight]);
    RV_PIXEL* pixelsPtr = pixels.get();

    for (size_t y = 0; y < tileHeight; y++)
    {
        for (size_t x = 0; x < tileWidth; x++)
        {
            const float* source = reinterpret_cast<const float*>(finalTile.pixel(x, tileHeight - y - 1));
            const size_t pixelIndex = y * tileWidth + x;
            pixelsPtr[pixelIndex].r = foundation::saturate(source[0]) * 255.0f;
            pixelsPtr[pixelIndex].g = foundation::saturate(source[1]) * 255.0f;
            pixelsPtr[pixelIndex].b = foundation::saturate(source[2]) * 255.0f;
            pixelsPtr[pixelIndex].a = foundation::saturate(source[3]) * 255.0f;
        }
    }

    const size_t x = tile_x * frameProps.m_tile_width;
    const size_t y = tile_y * frameProps.m_tile_height;

    Event e;
    e.mType = Event::TILEDONE;
    e.pixelData = pixels;
    e.tile_xmin = x;
    e.tile_xmax = x + tileWidth - 1;
    e.tile_ymin = frameProps.m_canvas_height - y - tileHeight;
    e.tile_ymax = frameProps.m_canvas_height - y - 1;
    gEventQueue()->push(e);
}

void TileCallbackFactory::release()
{
    delete this;
}

renderer::ITileCallback* TileCallbackFactory::create()
{
    return new TileCallback();
}
