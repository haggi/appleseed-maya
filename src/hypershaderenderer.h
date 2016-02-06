
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

#ifndef MAYA_RENDERER_H
#define MAYA_RENDERER_H

#if MAYA_API_VERSION >= 201600

#include "renderer/api/scene.h"
#include "renderer/api/project.h"
#include "renderer/global/globallogger.h"
#include "renderer/api/rendering.h"
#include "foundation/image/tile.h"

#include <maya/MTypes.h>
#include <maya/MPxRenderer.h>

// Standard headers.
#include <map>
#include <memory>

namespace asf = foundation;
namespace asr = renderer;

class HypershadeRenderer;

class HypershadeTileCallback
  : public asr::TileCallbackBase
{
  public:
    HypershadeRenderer *renderer;
    explicit HypershadeTileCallback(HypershadeRenderer *mrenderer) : renderer(mrenderer)
    {}
    virtual ~HypershadeTileCallback()
    {}
    virtual void release(){}
    void pre_render(const size_t x, const size_t y, const size_t width, const size_t height){}
    void post_render(const asr::Frame* frame);
    virtual void post_render_tile(const asr::Frame* frame, const size_t tile_x, const size_t tile_y);
};

class HypershadeTileCallbackFactory
  : public asr::ITileCallbackFactory
{
  public:
    HypershadeTileCallback *tileCallback;
    explicit HypershadeTileCallbackFactory(HypershadeRenderer *renderer)
    {
        tileCallback = new HypershadeTileCallback(renderer);
    }
    virtual ~HypershadeTileCallbackFactory()
    {
        delete tileCallback;
    }
    virtual asr::ITileCallback* create()
    {
        return tileCallback;
    };
    virtual void release(){ delete this; };
};

struct IdNameStruct
{
    MUuid id;
    MString name; // in appleseed objects must have unique names
    MObject mobject; // I need to know if I have a light or a mesh or a camera
};

class HypershadeRenderer : public MPxRenderer
{
  public:
    RefreshParams refreshParams;
    float* rb = 0;
    int tileSize = 32;
    int initialSize = 256;
    HypershadeRenderer();
    virtual ~HypershadeRenderer();
    static void* creator();
    virtual MStatus startAsync(const JobParams& params);
    virtual MStatus stopAsync();
    virtual bool isRunningAsync();
    void initProject();
    void initEnv();
    virtual MStatus beginSceneUpdate();

    virtual MStatus translateMesh(const MUuid& id, const MObject& node);
    virtual MStatus translateLightSource(const MUuid& id, const MObject& node);
    virtual MStatus translateCamera(const MUuid& id, const MObject& node);
    virtual MStatus translateEnvironment(const MUuid& id, EnvironmentType type);
    virtual MStatus translateTransform(const MUuid& id, const MUuid& childId, const MMatrix& matrix);
    virtual MStatus translateShader(const MUuid& id, const MObject& node);

    virtual MStatus setProperty(const MUuid& id, const MString& name, bool value);
    virtual MStatus setProperty(const MUuid& id, const MString& name, int value);
    virtual MStatus setProperty(const MUuid& id, const MString& name, float value);
    virtual MStatus setProperty(const MUuid& id, const MString& name, const MString& value);

    virtual MStatus setShader(const MUuid& id, const MUuid& shaderId);
    virtual MStatus setResolution(unsigned int width, unsigned int height);

    virtual MStatus endSceneUpdate();

    virtual MStatus destroyScene();

    virtual bool isSafeToUnload();

    void copyTileToBuffer(asf::Tile& tile, int tile_x, int tile_y);
    void copyFrameToBuffer(float *frame, int w, int h);
    void render();

  private:
    int width, height;
    //Render output buffer, it is R32G32B32A32_FLOAT format.
    boost::thread renderThread;
    asf::auto_release_ptr<asr::Project> project;
    std::auto_ptr<asr::MasterRenderer> mrenderer;
    asf::auto_release_ptr<HypershadeTileCallbackFactory> tileCallbackFac;
    RenderController controller;
    MUuid lastShapeId; // save the last shape id, needed by translateTransform
    MString lastMaterialName = "default";
    std::vector<IdNameStruct> objectArray;
    bool asyncStarted = false;
};

#endif

#endif
