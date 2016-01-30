
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

static const MString swatchName("AppleseedRenderSwatch");
static const MString swatchFullName("swatch/AppleseedRenderSwatch");

static const MString asDisneyMaterialId("asDisneyMaterialId");
static const MString asDisneyMaterialIdDrawDBClassification("drawdb/shader/surface/asDisneyMaterialId");
static const MString asDisneyMaterialIdFullClassification("shader/surface:Appleseed/material:" + swatchFullName + ":" + asDisneyMaterialIdDrawDBClassification);

static const MString asLayeredId("asLayeredId");
static const MString asLayeredIdFullClassification("shader/surface:Appleseed/material:" + swatchFullName);

MStatus initializePlugin( MObject obj )
{
    const MString   UserClassify( "shader/surface" );

    std::vector<std::string>::iterator it;
    std::vector<std::string> versions = getFullVersionString();
    for (it = versions.begin(); it != versions.end(); it++)
        MGlobal::displayInfo(it->c_str());

    MStatus   status;
    MFnPlugin plugin( obj, VENDOR, getFullVersionString()[0].c_str(), "Any");

    status = plugin.registerCommand(MAYATOCMDNAME, MayaToAppleseed::creator, MayaToAppleseed::newSyntax );
    if (!status) {
        status.perror("cannot register command: mayatoappleseed");
        return status;
    }

    status = plugin.registerNode(MayaToAppleseedGlobalsName, MayaToAppleseedGlobals::id, MayaToAppleseedGlobals::creator, MayaToAppleseedGlobals::initialize );
    if (!status) {
        status.perror("cannot register node: MayaToAppleseedGlobals");
        return status;
    }

    CHECK_MSTATUS(MHWRender::MDrawRegistry::registerSurfaceShadingNodeOverrideCreator(asDisneyMaterialIdDrawDBClassification, asDisneyMaterialId, asDisneyMaterialOverride::creator));
    status = plugin.registerNode("asDisneyMaterial", asDisneyMaterial::id, asDisneyMaterial::creator, asDisneyMaterial::initialize, MPxNode::kDependNode, &asDisneyMaterialIdFullClassification);
    CHECK_MSTATUS(status);
    status = plugin.registerNode("asLayeredShader", asLayeredShader::id, asLayeredShader::creator, asLayeredShader::initialize, MPxNode::kDependNode, &asLayeredIdFullClassification);
    CHECK_MSTATUS(status);

    MString command( "if( `window -exists createRenderNodeWindow` ) {refreshCreateRenderNodeWindow(\"" );
    command += UserClassify;
    command += "\");}\n";
    MGlobal::executeCommand( command );

    setRendererName("Appleseed");
    setRendererShortCutName("mtap");
    setRendererHome(getenv("MTAP_HOME"));

    MString cmd = MString("import appleseed.mtap_initialize as minit; minit.initRenderer()");
    MGlobal::displayInfo("try to register...");
    status = MGlobal::executePythonCommand(cmd, true, false);
    if(!status)
    {
        status.perror("Problem executing cmd: mtap_initialize.initRenderer()");
        return status;
    }

    MayaTo::defineWorld();
    MString loadPath = plugin.loadPath();
    MayaTo::getWorldPtr()->shaderSearchPath.append(loadPath);

    if (MGlobal::mayaState() != MGlobal::kBatch)
    {
        MSwatchRenderRegister::registerSwatchRender(swatchName, NewSwatchRenderer::creator);
    }

#if MAYA_API_VERSION >= 201600
    status = plugin.registerRenderer("Appleseed", mtap_MayaRenderer::creator);
    if (!status) {
        status.perror("cannot register node: Appleseed Maya renderer");
        return status;
    }
#endif
    return status;
}

MStatus uninitializePlugin( MObject obj)
{
    MStatus   status;
    MFnPlugin plugin( obj );

    MayaTo::deleteWorld();

    const MString UserClassify( "shader/surface" );

    std::cout << "deregister mtap cmd\n";
    status = plugin.deregisterCommand( MAYATOCMDNAME );
    if (!status) {
        status.perror("cannot deregister command: MayaToAppleseedCmd");
        return status;
    }

    if (MGlobal::mayaState() != MGlobal::kBatch)
        MSwatchRenderRegister::unregisterSwatchRender(swatchName);


#if MAYA_API_VERSION >= 201600
    status = plugin.deregisterRenderer("Corona");
    if (!status) {
        status.perror("cannot deregister node: Corona Maya renderer");
        return status;
    }
#endif

    //std::cout << "deregister mtap standinLocator\n";
    //status = plugin.deregisterNode( mtap_StandinLocator::id );
    //if (!status) {
    //  status.perror("cannot deregister node: mtap standinLocator");
    //  return status;
    //}

    //std::cout << "deregister mtap standinMeshNode\n";
    //status = plugin.deregisterNode( mtap_standinMeshNode::id );
    //if (!status) {
    //  status.perror("cannot deregister node: mtap_standinMeshNode");
    //  return status;
    //}

    std::cout << "deregister mtap globals\n";
    status = plugin.deregisterNode( MayaToAppleseedGlobals::id );
    if (!status) {
        status.perror("cannot deregister node: MayaToAppleseedGlobals");
        return status;
    }

    CHECK_MSTATUS(MHWRender::MDrawRegistry::deregisterSurfaceShadingNodeOverrideCreator(asDisneyMaterialIdDrawDBClassification, asDisneyMaterialId));
    CHECK_MSTATUS(plugin.deregisterNode(asDisneyMaterial::id));

    std::cout << "update mtap shader ui\n";
    MString command( "if( `window -exists createRenderNodeWindow` ) {refreshCreateRenderNodeWindow(\"" );
    command += UserClassify;
    command += "\");}\n";
    CHECK_MSTATUS( MGlobal::executeCommand( command ) );

    std::cout << "minit.unregister()\n";
    MString cmd = MString("import appleseed.mtap_initialize as minit; minit.unregister()");
    status = MGlobal::executePythonCommand(cmd);
    if(!status)
    {
        status.perror("Problem executing cmd: mtap_initialize.unregister()");
        return status;
    }

    return status;
}
