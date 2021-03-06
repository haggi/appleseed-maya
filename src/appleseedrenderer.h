
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

#ifndef APPLESEEDRENDERER_H
#define APPLESEEDRENDERER_H

// appleseed-maya headers.
#include "mayascene.h"
#include "renderercontroller.h"
#include "tilecallback.h"

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

// Forward declarations.
class MayaObject;
class MObject;
class mtap_MayaScene;
class mtap_RenderGlobals;
class ShadingNode;

#define APPLESEED_GLOBALS_ID 0x0011CF40

class AppleseedRenderer
{
  public:
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

    // Apply updates to the scene.
    void applyInteractiveUpdates(const MayaScene::EditableElementContainer& editableElements);

    void defineProject();
    void addRenderParams(renderer::ParamArray& paramArray);//add current render settings to all render configurations
    void defineConfig();
    void defineOutput();
    void createMesh(boost::shared_ptr<MayaObject> obj, renderer::MeshObjectArray& meshArray, bool& isProxyArray);
    void createMesh(boost::shared_ptr<MayaObject> obj);
    renderer::Project *getProjectPtr(){ return this->project.get(); }
    foundation::StringArray defineMaterial(boost::shared_ptr<MayaObject> obj);
    void updateMaterial(MObject sufaceShader);
    void preFrame();
    void postFrame();

  private:
    foundation::auto_release_ptr<renderer::Project> project;
    std::auto_ptr<renderer::MasterRenderer> masterRenderer;
    std::auto_ptr<foundation::ILogTarget> log_target;
    foundation::auto_release_ptr<TileCallbackFactory> tileCallbackFac;
    RendererController mRendererController;
    bool sceneBuilt;
};

#endif  // !APPLESEEDRENDERER_H
