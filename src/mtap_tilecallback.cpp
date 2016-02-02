
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
#include "mtap_tilecallback.h"

#include "threads/queue.h"
#include "utilities/logging.h"
#include "threads/renderqueueworker.h"

#include "renderer/api/frame.h"
#include "foundation/image/color.h"
#include "foundation/image/image.h"
#include "foundation/image/tile.h"

void mtap_TileCallback::release()
{
    delete this;
}

void mtap_TileCallback::pre_render(
    const size_t x,
    const size_t y,
    const size_t width,
    const size_t height)
{
}

void mtap_TileCallback::copyTileToImage(RV_PIXEL* pixels, asf::Tile& tile, int tile_x, int tile_y, const asr::Frame* frame)
{
    const asf::CanvasProperties& frame_props = frame->image().properties();
    size_t tw =  tile.get_width();
    size_t th =  tile.get_height();
    size_t ypos = frame_props.m_canvas_height - tile_y * frame_props.m_tile_height - 1;
    size_t pixelIdYStart = frame_props.m_canvas_width * ypos;
    size_t max = frame_props.m_canvas_width * frame_props.m_canvas_height;

    for (size_t ty = 0; ty < th; ty++)
    {
        for (size_t tx = 0; tx < tw; tx++)
        {
            size_t yPixelPos = pixelIdYStart - ty * frame_props.m_canvas_width;
            size_t pixelId = yPixelPos + tile_x * frame_props.m_tile_width + tx;
            asf::uint8 *source = tile.pixel(tx, ty);
            if (pixelId < max)
            {
                pixels[pixelId].r = (float)source[0];
                pixels[pixelId].g = (float)source[1];
                pixels[pixelId].b = (float)source[2];
                pixels[pixelId].a = (float)source[3];
            }
        }
    }
}

void mtap_TileCallback::post_render(const asr::Frame* frame)
{
    asf::Image img = frame->image();
    const asf::CanvasProperties& frame_props = img.properties();
    size_t numPixels = frame_props.m_canvas_width * frame_props.m_canvas_height;

    boost::shared_ptr<RV_PIXEL> pixelsPtr(new RV_PIXEL[numPixels]);
    RV_PIXEL *pixels = pixelsPtr.get();

    for (int x = 0; x < numPixels; x++)
    {
        pixels[x].r = 255.0f;
        pixels[x].g = .0f;
        pixels[x].b = .0f;
        pixels[x].a = .0f;
    }

    for (int tile_x = 0; tile_x < frame_props.m_tile_count_x; tile_x++)
    {
        for (int tile_y = 0; tile_y < frame_props.m_tile_count_y; tile_y++)
        {
            const asf::Tile& tile = frame->image().tile(tile_x, tile_y);

            asf::Tile float_tile_storage(
                tile.get_width(),
                tile.get_height(),
                frame_props.m_channel_count,
                asf::PixelFormatFloat);

            asf::Tile uint8_tile_storage(
                tile.get_width(),
                tile.get_height(),
                frame_props.m_channel_count,
                asf::PixelFormatUInt8);

            asf::Tile fp_rgb_tile(
                tile,
                asf::PixelFormatFloat,
                float_tile_storage.get_storage());

            frame->transform_to_output_color_space(fp_rgb_tile);

            static const size_t ShuffleTable[] = { 0, 1, 2, 3 };
            asf::Tile uint8_rgb_tile(
                fp_rgb_tile,
                asf::PixelFormatUInt8,
                uint8_tile_storage.get_storage());

            copyTileToImage(pixels, uint8_rgb_tile, tile_x, tile_y, frame);
        }
    }

    Event e;
    e.pixelData = pixelsPtr;
    e.type = Event::TILEDONE;
    e.tile_xmin = 0;
    e.tile_xmax = frame_props.m_canvas_width - 1;
    e.tile_ymin = 0;
    e.tile_ymax = frame_props.m_canvas_height - 1;
    theRenderEventQueue()->push(e);
}

void mtap_TileCallback::post_render_tile(
    const asr::Frame* frame,
    const size_t tile_x,
    const size_t tile_y)
{
    asf::Image img = frame->image();
    const asf::CanvasProperties& frame_props = img.properties();
    const asf::Tile& tile = frame->image().tile(tile_x, tile_y);

    asf::Tile float_tile_storage(
        frame_props.m_tile_width,
        frame_props.m_tile_height,
        frame_props.m_channel_count,
        asf::PixelFormatFloat);

    asf::Tile uint8_tile_storage(
        frame_props.m_tile_width,
        frame_props.m_tile_height,
        frame_props.m_channel_count,
        asf::PixelFormatUInt8);

    asf::Tile fp_rgb_tile(
        tile,
        asf::PixelFormatFloat,
        float_tile_storage.get_storage());

    frame->transform_to_output_color_space(fp_rgb_tile);

    static const size_t ShuffleTable[] = { 0, 1, 2, 3 };
    const asf::Tile uint8_rgb_tile(
        fp_rgb_tile,
        asf::PixelFormatUInt8,
        uint8_tile_storage.get_storage());

    size_t tw =  tile.get_width();
    size_t th =  tile.get_height();
    size_t numPixels = tw * th;
    boost::shared_ptr<RV_PIXEL> pixelsPtr(new RV_PIXEL[numPixels]);
    RV_PIXEL *pixels = pixelsPtr.get();
    for (size_t yy = 0; yy < th; yy++)
    {
        for (size_t xx = 0; xx < tw; xx++)
        {
            size_t pixelId = yy * tw + xx;
            size_t yy1 = th - yy - 1;
            asf::uint8 *source = uint8_rgb_tile.pixel(xx, yy1);
            pixels[pixelId].r = (float)source[0];
            pixels[pixelId].g = (float)source[1];
            pixels[pixelId].b = (float)source[2];
            pixels[pixelId].a = (float)source[3];
        }
    }

    size_t x = tile_x * frame_props.m_tile_width;
    size_t y = tile_y * frame_props.m_tile_height;
    size_t x1 = x + tw - 1;
    size_t y1 = y + th;
    size_t miny = img.properties().m_canvas_height - y1;
    size_t maxy = img.properties().m_canvas_height - y - 1;

    Event e;
    e.pixelData = pixelsPtr;
    e.type = Event::TILEDONE;
    e.tile_xmin = x;
    e.tile_xmax = x1;
    e.tile_ymin = miny;
    e.tile_ymax = maxy;
    theRenderEventQueue()->push(e);
}

asr::ITileCallback* mtap_TileCallbackFactory::create()
{
    return new mtap_TileCallback();
}

void mtap_TileCallbackFactory::release()
{
    delete this;
}
