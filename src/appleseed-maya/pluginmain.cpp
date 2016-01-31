
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

#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>
#include <maya/MDrawRegistry.h>
#include <maya/MSwatchRenderRegister.h>

#include "mayatoappleseed.h"
#include "mtap_renderglobalsnode.h"
#include "swatchesrenderer/swatchrenderer.h"
#include "swatchesrenderer/newswatchrenderer.h"

#include "utilities/tools.h"
#include "threads/renderqueueworker.h"
#include "world.h"
#include "version.h"

#include "shaders/asdisneymaterial.h"
#include "shaders/asdisneymaterialoverride.h"
#include "shaders/aslayeredshader.h"

#if MAYA_API_VERSION >= 201600
#include "mtap_mayarenderer.h"
#endif

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

static const MString commandName("appleseedMaya");

static const MString swatchName("AppleseedRenderSwatch");
static const MString swatchFullName("swatch/AppleseedRenderSwatch");

static const MString asDisneyMaterialId("asDisneyMaterialId");
static const MString asDisneyMaterialIdDrawDBClassification("drawdb/shader/surface/asDisneyMaterialId");
static const MString asDisneyMaterialIdFullClassification("shader/surface:Appleseed/material:" + swatchFullName + ":" + asDisneyMaterialIdDrawDBClassification);

static const MString asLayeredId("asLayeredId");
static const MString asLayeredIdFullClassification("shader/surface:Appleseed/material:" + swatchFullName);

static const MString UserClassify("shader/surface");

DLLEXPORT MStatus initializePlugin(MObject obj)
{
    const std::vector<std::string> versions = getFullVersionString();
    for (size_t i = 0; i < versions.size(); ++i)
        MGlobal::displayInfo(versions[i].c_str());

    MStatus status;
    MFnPlugin plugin(obj, VENDOR, versions[0].c_str(), "Any");

    status = plugin.registerCommand(commandName, MayaToAppleseed::creator, MayaToAppleseed::syntaxCreator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.registerNode(MayaToAppleseedGlobalsName, MayaToAppleseedGlobals::id, MayaToAppleseedGlobals::creator, MayaToAppleseedGlobals::initialize);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MHWRender::MDrawRegistry::registerSurfaceShadingNodeOverrideCreator(asDisneyMaterialIdDrawDBClassification, asDisneyMaterialId, asDisneyMaterialOverride::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.registerNode("asDisneyMaterial", asDisneyMaterial::id, asDisneyMaterial::creator, asDisneyMaterial::initialize, MPxNode::kDependNode, &asDisneyMaterialIdFullClassification);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.registerNode("asLayeredShader", asLayeredShader::id, asLayeredShader::creator, asLayeredShader::initialize, MPxNode::kDependNode, &asLayeredIdFullClassification);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString command("if (`window -exists createRenderNodeWindow`) {refreshCreateRenderNodeWindow(\"");
    command += UserClassify;
    command += "\");}\n";
    status = MGlobal::executeCommand(command);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    setRendererHome(getenv("MTAP_HOME"));

    MGlobal::displayInfo("try to register...");
    status = MGlobal::executePythonCommand("import mtap_initialize as minit; minit.initRenderer()", true, false);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    defineWorld();

    getWorldPtr()->shaderSearchPath.append(plugin.loadPath());

    if (MGlobal::mayaState() != MGlobal::kBatch)
        MSwatchRenderRegister::registerSwatchRender(swatchName, NewSwatchRenderer::creator);

#if MAYA_API_VERSION >= 201600
    status = plugin.registerRenderer("appleseed", mtap_MayaRenderer::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);
#endif

    return status;
}

DLLEXPORT MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj);

    deleteWorld();

    status = plugin.deregisterCommand(commandName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (MGlobal::mayaState() != MGlobal::kBatch)
        MSwatchRenderRegister::unregisterSwatchRender(swatchName);

#if MAYA_API_VERSION >= 201600
    status = plugin.deregisterRenderer("appleseed");
    CHECK_MSTATUS_AND_RETURN_IT(status);
#endif

    status = plugin.deregisterNode(MayaToAppleseedGlobals::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MHWRender::MDrawRegistry::deregisterSurfaceShadingNodeOverrideCreator(asDisneyMaterialIdDrawDBClassification, asDisneyMaterialId);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.deregisterNode(asDisneyMaterial::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString command("if (`window -exists createRenderNodeWindow`) {refreshCreateRenderNodeWindow(\"");
    command += UserClassify;
    command += "\");}\n";
    status = MGlobal::executeCommand(command);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MGlobal::executePythonCommand("import mtap_initialize as minit; minit.unregister()");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}
