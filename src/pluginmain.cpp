
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

// appleseed-maya headers.
#include "mayatoappleseed.h"
#include "mtap_renderglobalsnode.h"
#include "swatchesrenderer/newswatchrenderer.h"
#if MAYA_API_VERSION >= 201600
#include "mtap_mayarenderer.h"
#endif
#include "utilities/tools.h"
#include "threads/renderqueueworker.h"
#include "world.h"
#include "version.h"
#include "binmeshtranslator.h"
#include "binmeshwritercmd.h"
#include "binmeshreadercmd.h"
#include "shaders/asdisneymaterial.h"
#include "shaders/asdisneymaterialoverride.h"
#include "shaders/aslayeredshader.h"

// Maya headers.
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>
#include <maya/MDrawRegistry.h>
#include <maya/MSwatchRenderRegister.h>

#ifdef _WIN32
#define APPLESEEDMAYA_DLL_EXPORT __declspec(dllexport)
#else
#define APPLESEEDMAYA_DLL_EXPORT
#endif

static const MString swatchName("AppleseedRenderSwatch");
static const MString swatchFullName("swatch/AppleseedRenderSwatch");

static const MString asDisneyMaterialIdDrawDBClassification("drawdb/shader/surface/asDisneyMaterialId");
static const MString asDisneyMaterialIdFullClassification("shader/surface:Appleseed/material:" + swatchFullName + ":" + asDisneyMaterialIdDrawDBClassification);

static const MString asLayeredId("asLayeredId");
static const MString asLayeredIdFullClassification("shader/surface:Appleseed/material:" + swatchFullName);

APPLESEEDMAYA_DLL_EXPORT MStatus initializePlugin(MObject obj)
{
    MGlobal::displayInfo("Initializing appleseed-maya plugin...");

    const std::vector<std::string> versions = getFullVersionString();
    for (size_t i = 0; i < versions.size(); ++i)
        MGlobal::displayInfo(versions[i].c_str());

    MFnPlugin plugin(obj, VENDOR, versions[0].c_str(), "Any");
    MStatus status;

    status =
        plugin.registerFileTranslator(
            "appleseedBinaryMesh",
            "",                                 // pixmap name
            BinMeshTranslator::creator,
            "binMeshTranslatorOpts",            // options display script name
            "",                                 // default options which are passed to the display script
            true);                              // can use MGlobal::executeCommand
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        MGlobal::executeCommand(
            "global proc binMeshTranslatorOpts(string $a, string $b, string $c, string $d)\n"
            "{\n"
            "    python(\"import binMeshTranslator; binMeshTranslator.binMeshTranslatorOpts('\" + $a + \"', '\" + $b + \"', '\" + $c + \"', '\" + $d + \"')\");\n"
            "}\n");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        plugin.registerCommand(
            "binMeshWriterCmd",
            BinMeshWriterCmd::creator,
            BinMeshWriterCmd::newSyntax);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        plugin.registerCommand(
            "binMeshReaderCmd",
            BinMeshReaderCmd::creator,
            BinMeshReaderCmd::newSyntax);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        plugin.registerCommand(
            "appleseedMaya",
            MayaToAppleseed::creator,
            MayaToAppleseed::syntaxCreator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        plugin.registerNode(
            "appleseedGlobals",
            MayaToAppleseedGlobals::id,
            MayaToAppleseedGlobals::creator,
            MayaToAppleseedGlobals::initialize);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        MHWRender::MDrawRegistry::registerSurfaceShadingNodeOverrideCreator(
            "drawdb/shader/surface/asDisneyMaterialId",
            "asDisneyMaterialId",
            asDisneyMaterialOverride::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        plugin.registerNode(
            "asDisneyMaterial",
            asDisneyMaterial::id,
            asDisneyMaterial::creator,
            asDisneyMaterial::initialize,
            MPxNode::kDependNode,
            &asDisneyMaterialIdFullClassification);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        plugin.registerNode(
            "asLayeredShader",
            asLayeredShader::id,
            asLayeredShader::creator,
            asLayeredShader::initialize,
            MPxNode::kDependNode, &asLayeredIdFullClassification);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        MGlobal::executeCommand(
            "if (`window -exists createRenderNodeWindow`)\n"
            "{\n"
            "    refreshCreateRenderNodeWindow(\"shader/surface\");\n"
            "}\n");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    setRendererHome(getenv("MTAP_HOME"));

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

APPLESEEDMAYA_DLL_EXPORT MStatus uninitializePlugin(MObject obj)
{
    MGlobal::displayInfo("Shutting down appleseed-maya plugin...");

    MFnPlugin plugin(obj);
    MStatus status;

#if MAYA_API_VERSION >= 201600
    status = plugin.deregisterRenderer("appleseed");
    CHECK_MSTATUS_AND_RETURN_IT(status);
#endif

    if (MGlobal::mayaState() != MGlobal::kBatch)
        MSwatchRenderRegister::unregisterSwatchRender(swatchName);

    deleteWorld();

    status = MGlobal::executePythonCommand("import mtap_initialize as minit; minit.unregister()");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        MGlobal::executeCommand(
            "if (`window -exists createRenderNodeWindow`)\n"
            "{\n"
            "    refreshCreateRenderNodeWindow(\"shader/surface\");\n"
            "}\n");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.deregisterNode(asLayeredShader::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.deregisterNode(asDisneyMaterial::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        MHWRender::MDrawRegistry::deregisterSurfaceShadingNodeOverrideCreator(
            "drawdb/shader/surface/asDisneyMaterialId",
            "asDisneyMaterialId");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.deregisterNode(MayaToAppleseedGlobals::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.deregisterCommand("appleseedMaya");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.deregisterCommand("binMeshReaderCmd");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.deregisterCommand("binMeshWriterCmd");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.deregisterFileTranslator("appleseedBinaryMesh");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}
