
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

#ifndef MTAP_APPLESEED_H
#define MTAP_APPLESEED_H

// appleseed-maya headers.
#include "mtap_renderercontroller.h"
#include "mtap_tilecallback.h"

// appleseed.renderer headers.
#include "renderer/api/bsdf.h"
#include "renderer/api/camera.h"
#include "renderer/api/color.h"
#include "renderer/api/environment.h"
#include "renderer/api/frame.h"
#include "renderer/api/light.h"
#include "renderer/api/log.h"
#include "renderer/api/material.h"
#include "renderer/api/object.h"
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"
#include "renderer/api/scene.h"
#include "renderer/api/surfaceshader.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/core/appleseed.h"
#include "foundation/image/image.h"
#include "foundation/image/tile.h"
#include "foundation/math/matrix.h"
#include "foundation/math/scalar.h"
#include "foundation/math/transform.h"
#include "foundation/utility/containers/specializedarrays.h"
#include "foundation/utility/log/consolelogtarget.h"
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/searchpaths.h"

// Maya headers.
#include <maya/MString.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MFnMeshData.h>

// Standard headers.
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#define RENDERGLOBALS_NODE      0x0011CF40
#define PHYSICAL_SURFACE_SHADER 0x0011CF46
#define AO_SHADER               0x0011CF41
#define WIREFRAME_SHADER        0x00106EF6
#define AOVOXEL_SHADER          0x0011CF42
#define FASTSSS_SHADER          0x0011CF45
#define CONST_SHADER            0x0011CF43
#define DIAGNOSTIC_SHADER       0x0011CF44
#define SMOKE_SHADER            0x0011CF47

// Forward declarations.
class InteractiveElement;
class MayaObject;
class MObject;
class mtap_MayaScene;
class mtap_RenderGlobals;
class ShadingNode;

namespace asf = foundation;
namespace asr = renderer;

class AppleseedRenderer
{
  public:
    std::vector<InteractiveElement *> interactiveUpdateList;

    AppleseedRenderer();
    ~AppleseedRenderer();

    void defineCamera();
    void defineCamera(boost::shared_ptr<MayaObject> obj);
    void defineEnvironment();
    void defineGeometry();
    void updateGeometry(boost::shared_ptr<MayaObject> obj);
    void updateInstance(boost::shared_ptr<MayaObject> obj);
    void defineLights();
    void defineLight(boost::shared_ptr<MayaObject> obj);
    void render();

    // This method is called before rendering starts.
    // It should prepare all data which can/should be reused during
    // IPR/frame/sequence rendering.
    void initializeRenderer();

    // This method is called at the end of rendering.
    void unInitializeRenderer();

    // This method is called during scene updating. If a renderer can update motionblur steps on the fly,
    // the geometry is defined at the very first step and later this definition will be updated for every motion step.
    // Other renderers will need a complete definition of all motionblur steps at once, so the motion steps will be
    // in the geometry e.g. with obj->addMeshData(); and at the very last step, everything is defined.
    void updateShape(boost::shared_ptr<MayaObject> obj);

    // This method is necessary only if the renderer is able to update the transform definition interactively.
    // In other cases, the world space transform will be defined directly during the creation of the geometry.
    void updateTransform(boost::shared_ptr<MayaObject> obj);

    void abortRendering();

    // This method can be called after a rendering is finished and
    // the user wants to modify the color mapping of the final rendering.
    // All interactive callbacks should be pushed into the worker queue with INTERACTIVEFBCALLBACK so that we don't get
    // any race conditions if the callback takes longer than the next call.
    void interactiveFbCallback() {}

    // Make an interactive update of the scene. Before this call the interactiveUpdateList should be filled appropriately.
    void doInteractiveUpdate();

    void handleUserEvent(int event, MString strData, float floatData, int intData){}

    asf::auto_release_ptr<asr::MeshObject> defineStandardPlane(bool area = false);
    void defineProject();
    void addRenderParams(asr::ParamArray& paramArray);//add current render settings to all render configurations
    void defineConfig();
    void defineOutput();
    void createMesh(boost::shared_ptr<MayaObject> obj, asr::MeshObjectArray& meshArray, bool& isProxyArray);
    void createMesh(boost::shared_ptr<MayaObject> obj);
    asr::Project *getProjectPtr(){ return this->project.get(); }
    asf::StringArray defineMaterial(boost::shared_ptr<MayaObject> obj);
    void updateMaterial(MObject sufaceShader);
    void preFrame();
    void postFrame();

  private:
    asf::auto_release_ptr<asr::Project> project;
    std::auto_ptr<asr::MasterRenderer> masterRenderer;
    std::auto_ptr<asf::ILogTarget> log_target;
    asf::auto_release_ptr<mtap_ITileCallbackFactory> tileCallbackFac;
    mtap_IRendererController mtap_controller;
    bool sceneBuilt;
};

#endif
