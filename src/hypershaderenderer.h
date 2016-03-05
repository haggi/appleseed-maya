
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

#ifndef HYPERSHADERENDERER_H
#define HYPERSHADERENDERER_H

// Include first as this defines MAYA_API_VERSION.
#include <maya/MTypes.h>

#if MAYA_API_VERSION >= 201600

// appleseed-maya headers.
#include "renderercontroller.h"

// appleseed.renderer headers.
#include "renderer/api/object.h"
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"

// appleseed.foundation headers.
#include "foundation/platform/compiler.h"
#include "foundation/utility/autoreleaseptr.h"

// Maya headers.
#include <maya/MPxRenderer.h>
#include <maya/MObject.h>
#include <maya/MString.h>

// Standard headers.
#include <memory>
#include <vector>

// Forward declarations.
namespace foundation    { class Tile; }
namespace renderer      { class Frame; }
class HypershadeRenderer;

class HypershadeTileCallback
  : public renderer::TileCallbackBase
{
  public:
    explicit HypershadeTileCallback(HypershadeRenderer* renderer);
    virtual void release() APPLESEED_OVERRIDE;

    virtual void pre_render(const size_t x, const size_t y, const size_t width, const size_t height) APPLESEED_OVERRIDE;
    virtual void post_render_tile(const renderer::Frame* frame, const size_t tile_x, const size_t tile_y) APPLESEED_OVERRIDE;
    virtual void post_render(const renderer::Frame* frame) APPLESEED_OVERRIDE;

  private:
    HypershadeRenderer* mRenderer;
};

class HypershadeTileCallbackFactory
  : public renderer::ITileCallbackFactory
{
  public:
    explicit HypershadeTileCallbackFactory(HypershadeRenderer* renderer);
    virtual void release() APPLESEED_OVERRIDE;

    virtual renderer::ITileCallback* create() APPLESEED_OVERRIDE;

  private:
    HypershadeRenderer* mRenderer;
    std::auto_ptr<HypershadeTileCallback> mTileCallback;
};

struct IdNameStruct
{
    MUuid id;
    MString name; // in appleseed objects must have unique names
    MObject mobject; // I need to know if I have a light or a mesh or a camera
};

class HypershadeRenderer 
  : public MPxRenderer
{
  public:
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

    void copyTileToBuffer(foundation::Tile& tile, int tile_x, int tile_y);
    void copyFrameToBuffer(float *frame, int w, int h);
    void render();
    void updateMaterial(const MObject materialNode, const renderer::Assembly* assembly);

  private:
    float* rb;
    int tileSize;
    int initialSize;
    int width, height;
    RefreshParams refreshParams;
    boost::thread renderThread;
    foundation::auto_release_ptr<renderer::Project> project;
    std::auto_ptr<renderer::MasterRenderer> mrenderer;
    foundation::auto_release_ptr<HypershadeTileCallbackFactory> tileCallbackFac;
    RendererController controller;
    MUuid lastShapeId; // save the last shape id, needed by translateTransform
    MString lastMaterialName;
    std::vector<IdNameStruct> objectArray;
    bool asyncStarted;
};

#endif

#endif  // !HYPERSHADERENDERER_H
