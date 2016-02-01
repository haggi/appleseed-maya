
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

#include "appleseed.h"
#include "mtap_tilecallback.h"

#include "renderer/global/globallogger.h"
#include "renderer/api/environment.h"
#include "renderer/api/environmentedf.h"
#include "renderer/api/texture.h"
#include "renderer/api/environmentshader.h"
#include "renderer/api/edf.h"

#include "renderer/modeling/environmentedf/sphericalcoordinates.h"

#include "foundation/platform/thread.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPointArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnCamera.h>
#include <maya/MGlobal.h>
#include <maya/MRenderView.h>
#include <maya/MFileIO.h>
#include <maya/MItDag.h>
#include <maya/MFnTransform.h>

#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "threads/threads.h"
#include "threads/queue.h"
#include "utilities/pystring.h"
#include "mayascene.h"
#include "renderglobals.h"
#include "threads/renderqueueworker.h"
#include "world.h"

#include <iostream>

#include "utilities/logging.h"
#include "appleseedutils.h"

AppleseedRenderer::AppleseedRenderer()
  : sceneBuilt(false)
{
    asr::global_logger().set_format(asf::LogMessage::Debug, "");
    log_target.reset(asf::create_console_log_target(stdout));
    asr::global_logger().add_target(log_target.get());
}

AppleseedRenderer::~AppleseedRenderer()
{
    if (log_target.get() != 0)
        asr::global_logger().remove_target(log_target.get());
}

void AppleseedRenderer::initializeRenderer()
{
    this->project = asr::ProjectFactory::create("mtap_project");

    std::string oslShaderPath = (getRendererHome() + "shaders").asChar();
    Logging::debug(MString("setting osl shader search path to: ") + oslShaderPath.c_str());
    project->search_paths().push_back(oslShaderPath.c_str());
    for (uint i = 0; i < getWorldPtr()->shaderSearchPath.length(); i++)
    {
        Logging::debug(MString("Search path: ") + getWorldPtr()->shaderSearchPath[i]);
        project->search_paths().push_back(getWorldPtr()->shaderSearchPath[i].asChar());
    }
    defineConfig();
    defineScene(this->project.get());
}

void AppleseedRenderer::unInitializeRenderer()
{
    getWorldPtr()->setRenderState(MayaToWorld::RSTATEDONE);
    getWorldPtr()->setRenderType(MayaToWorld::RTYPENONE);

    // todo: isn't this supposed to be reset()?
    this->project.release();
}

void AppleseedRenderer::defineProject()
{
    defineCamera(); // first camera
    defineOutput(); // output accesses camera so define it after camera
    defineMasterAssembly(this->project.get());
    defineDefaultMaterial(this->project.get());
    defineEnvironment();
    defineGeometry();
    defineLights();
}

void AppleseedRenderer::preFrame()
{
    defineProject();
}

void AppleseedRenderer::postFrame()
{
    // Save the frame to disk.
    boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;
    renderGlobals->getImageName();
    MString filename = renderGlobals->imageOutputFile.asChar();
    Logging::debug(MString("Saving image as ") + renderGlobals->imageOutputFile);
    project->get_frame()->write_main_image(renderGlobals->imageOutputFile.asChar());

    // if we render the very last frame or if we are in UI where the last frame == first frame, then delete the master renderer before
    // the deletion of the assembly because otherwise it will be deleted automatically if the renderer instance is deleted what results in a crash
    // because the masterRenderer still have references to the shading groups which are defined in the world assembly. If the masterRenderer is deleted
    // AFTER the assembly it tries to access non existent shadingGroups.
    if (renderGlobals->currentFrameNumber == renderGlobals->frameList.back())
        masterRenderer.reset();

    asf::UniqueID aiuid = project->get_scene()->assembly_instances().get_by_name("world_Inst")->get_uid();
    asf::UniqueID auid = project->get_scene()->assemblies().get_by_name("world")->get_uid();
    project->get_scene()->assembly_instances().remove(aiuid);
    project->get_scene()->assemblies().remove(auid);
    asr::Assembly *worldass = project->get_scene()->assemblies().get_by_name("world");
}

void AppleseedRenderer::render()
{
    Logging::debug("AppleseedRenderer::render");

    if (!sceneBuilt)
    {
        this->tileCallbackFac.reset(new mtap_ITileCallbackFactory());

        if (getWorldPtr()->getRenderType() == MayaToWorld::IPRRENDER)
        {
            masterRenderer.reset(
                new asr::MasterRenderer(
                    this->project.ref(),
                    this->project->configurations().get_by_name("interactive")->get_inherited_parameters(),
                    &mtap_controller,
                    this->tileCallbackFac.get()));
        }
        else
        {
            masterRenderer.reset(
                new asr::MasterRenderer(
                    this->project.ref(),
                    this->project->configurations().get_by_name("final")->get_inherited_parameters(),
                    &mtap_controller,
                    this->tileCallbackFac.get()));
        }

        if (getWorldPtr()->getRenderType() == MayaToWorld::IPRRENDER)
        {
            Event e;
            e.type = Event::ADDIPRCALLBACKS;
            theRenderEventQueue()->push(e);

            while (!RenderQueueWorker::iprCallbacksDone())
                asf::sleep(10);
        }

        sceneBuilt = true;
    }

    getWorldPtr()->setRenderState(MayaToWorld::RSTATERENDERING);
    mtap_controller.status = asr::IRendererController::ContinueRendering;

    masterRenderer->render();
}

void AppleseedRenderer::abortRendering()
{
    mtap_controller.status = asr::IRendererController::AbortRendering;
}
