#include "appleseed.h"
#include "mtap_tilecallback.h"

#include "renderer/global/globallogger.h"
#include "renderer/api/environment.h"
#include "renderer/api/environmentedf.h"
#include "renderer/api/texture.h"
#include "renderer/api/environmentshader.h"
#include "renderer/api/edf.h"

#include "renderer/modeling/environmentedf/sphericalcoordinates.h"

#include "maya/mfndependencynode.h"
#include "maya/mfnmesh.h"
#include "maya/mitmeshpolygon.h"
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

#include <stdio.h>
#include <iostream>

#include "utilities/logging.h"
#include "appleseedutils.h"

static int tileCount = 0;
static int tileCountTotal = 0;

using namespace AppleRender;

std::vector<asr::Entity *> definedEntities;

AppleseedRenderer::AppleseedRenderer()
{
	asr::global_logger().set_format(asf::LogMessage::Debug, "");
	log_target = std::auto_ptr<asf::ILogTarget>(asf::create_console_log_target(stdout));
	asr::global_logger().add_target(log_target.get());
}

AppleseedRenderer::~AppleseedRenderer()
{
	if (log_target.get() != nullptr)
	{
		Logging::debug("Unloading logger.");
		asr::global_logger().remove_target(log_target.get());
	}
}

void AppleseedRenderer::initializeRenderer()
{
	//RENDERER_LOG_INFO("%s", asf::Appleseed::get_synthetic_version_string());
	//definedEntities.clear();
	this->project = asr::ProjectFactory::create("mtap_project");

	std::string oslShaderPath = (getRendererHome() + "shaders").asChar();
	Logging::debug(MString("setting osl shader search path to: ") + oslShaderPath.c_str());
	project->search_paths().push_back(oslShaderPath.c_str());
	for (uint i = 0; i < MayaTo::getWorldPtr()->shaderSearchPath.length(); i++)
	{
		Logging::debug(MString("Search path: ") + MayaTo::getWorldPtr()->shaderSearchPath[i]);
		project->search_paths().push_back(MayaTo::getWorldPtr()->shaderSearchPath[i].asChar());
	}
	defineConfig();
	defineScene(this->project.get());
}

void AppleseedRenderer::unInitializeRenderer()
{
	MayaTo::getWorldPtr()->setRenderState(MayaTo::MayaToWorld::RSTATEDONE);
	MayaTo::getWorldPtr()->setRenderType(MayaTo::MayaToWorld::RTYPENONE);

	Logging::debug("Releasing project");
	this->project.release();
	Logging::debug("Releasing done.");
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
	Logging::debug("AppleseedRenderer::preFrame");
	defineProject();
}

void AppleseedRenderer::postFrame()
{
	Logging::debug("AppleseedRenderer::postFrame");
	// Save the frame to disk.
	std::shared_ptr<RenderGlobals> renderGlobals = MayaTo::getWorldPtr()->worldRenderGlobalsPtr;
	renderGlobals->getImageName();
	MString filename = renderGlobals->imageOutputFile.asChar();
	Logging::debug(MString("Saving image as ") + renderGlobals->imageOutputFile);
	project->get_frame()->write_main_image(renderGlobals->imageOutputFile.asChar());	

	// if we render the very last frame or if we are in UI where the last frame == first frame, then delete the master renderer before
	// the deletion of the assembly because otherwise it will be deleted automatically if the renderer instance is deleted what results in a crash
	// because the masterRenderer still have references to the shading groups which are defined in the world assembly. If the masterRenderer is deleted
	// AFTER the assembly it tries to access non existent shadingGroups.
	if (renderGlobals->currentFrameNumber == renderGlobals->frameList.back())
	{
		masterRenderer.release();
	}

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

		RENDERER_LOG_INFO("%s", asf::Appleseed::get_synthetic_version_string());

		this->tileCallbackFac.reset(new mtap_ITileCallbackFactory());

		if (MayaTo::getWorldPtr()->getRenderType() == MayaTo::MayaToWorld::IPRRENDER)
		{
			masterRenderer = std::auto_ptr<asr::MasterRenderer>( new asr::MasterRenderer(
				this->project.ref(),
				this->project->configurations().get_by_name("interactive")->get_inherited_parameters(),
				&mtap_controller,
				this->tileCallbackFac.get()));
		}
		else{
			masterRenderer = std::auto_ptr<asr::MasterRenderer>(new asr::MasterRenderer(
				this->project.ref(),
				this->project->configurations().get_by_name("final")->get_inherited_parameters(),
				&mtap_controller,
				this->tileCallbackFac.get()));
		}
		// Save the project to disk.
		asr::ProjectFileWriter::write(project.ref(), "C:/daten/3dprojects/mayaToAppleseed/renderData/test.appleseed");

		if (MayaTo::getWorldPtr()->getRenderType() == MayaTo::MayaToWorld::WorldRenderType::IPRRENDER)
		{
			EventQueue::Event e;
			e.type = EventQueue::Event::ADDIPRCALLBACKS;
			theRenderEventQueue()->push(e);
			while (!RenderQueueWorker::iprCallbacksDone())
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		sceneBuilt = true;
	}

	MayaTo::getWorldPtr()->setRenderState(MayaTo::MayaToWorld::RSTATERENDERING);
	mtap_controller.status = asr::IRendererController::ContinueRendering;
	masterRenderer->render();
}

void AppleseedRenderer::abortRendering()
{
	Logging::debug(MString("abortRendering"));
	mtap_controller.status = asr::IRendererController::AbortRendering;
}
