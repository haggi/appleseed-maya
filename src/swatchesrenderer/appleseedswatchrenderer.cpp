
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
#include "appleseedswatchrenderer.h"

// appleseed.renderer headers.
#include "renderer/api/bsdf.h"
#include "renderer/api/camera.h"
#include "renderer/api/color.h"
#include "renderer/api/edf.h"
#include "renderer/api/environment.h"
#include "renderer/api/environmentedf.h"
#include "renderer/api/environmentshader.h"
#include "renderer/api/frame.h"
#include "renderer/api/light.h"
#include "renderer/api/log.h"
#include "renderer/api/material.h"
#include "renderer/api/object.h"
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"
#include "renderer/api/scene.h"
#include "renderer/api/shadergroup.h"
#include "renderer/api/surfaceshader.h"
#include "renderer/api/texture.h"
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

// appleseed-maya headers.
#include "osl/oslutils.h"
#include "shadingtools/material.h"
#include "shadingtools/shadingutils.h"
#include "swatchesrenderer/newswatchrenderer.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "swatchesevent.h"
#include "world.h"

// Maya headers.
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MPlugArray.h>

namespace asf = foundation;
namespace asr = renderer;

AppleseedSwatchRenderer::AppleseedSwatchRenderer()
  : terminateLoop(false)
  , enableSwatchRenderer(true)
{
    MString swatchRenderFile = getRendererHome() + "resources/swatchRender.xml";
    MString schemaPath = getRendererHome() + "schemas/project.xsd";
    project = asr::ProjectFileReader().read(swatchRenderFile.asChar(), schemaPath.asChar());
    if (project.get() == 0)
    {
        Logging::error(MString("Unable to load swatch render file correctly: ") + swatchRenderFile);
        return;
    }
    else
    {
        Logging::info(MString("Successfully loaded swatch render file."));
    }
    MString cmd = MString("import renderer.osltools as osl; osl.getOSODirs();");
    MStringArray oslDirs;
    MGlobal::executePythonCommand(cmd, oslDirs, false, false);
    for (uint i = 0; i < oslDirs.length(); i++)
    {
        project->search_paths().push_back(oslDirs[i].asChar());
    }

    mrenderer.reset(
        new asr::MasterRenderer(
            project.ref(),
            project->configurations().get_by_name("final")->get_inherited_parameters(),
            &renderer_controller));
}

AppleseedSwatchRenderer::~AppleseedSwatchRenderer()
{
    terminateAppleseedSwatchRender(this);

    // todo: wasn't this supposed to be project.reset()?
    project.release();
}

void AppleseedSwatchRenderer::renderSwatch(NewSwatchRenderer *sr)
{
    int res(sr->resolution());
    setSize(res);
    setShader(sr->dNode);
    mrenderer->render();

    sr->image().create(res, res, 4, MImage::kFloat);
    float *floatPixels = sr->image().floatPixels();
    fillSwatch(floatPixels);
}

void AppleseedSwatchRenderer::fillSwatch(float *pixels)
{
    const asf::Image& image = project->get_frame()->image();
    const size_t res = image.properties().m_canvas_height;

    assert(image.properties().m_channel_count == 4);

    size_t index = 0;

    if (image.properties().m_canvas_height == image.properties().m_tile_height)
    {
        asf::Tile& tile = project->get_frame()->image().tile(0, 0);
        for (size_t y = 0; y < res; y++)
        {
            for (size_t x = 0; x < res; x++)
            {
                for (size_t c = 0; c < 4; c++)
                    pixels[index++] = tile.get_component<float>(x, y, c);
            }
        }
    }
    else
    {
        for (size_t y = 0; y < res; y++)
        {
            for (size_t x = 0; x < res; x++)
            {
                asf::Color4f p;
                image.get_pixel(x, y, p);
                pixels[index++] = p.r;
                pixels[index++] = p.g;
                pixels[index++] = p.g;
                pixels[index++] = 1.0f;
            }
        }
    }
}

void AppleseedSwatchRenderer::setSize(int size)
{
    MString res = MString("") + size + " " + size;
    asr::ParamArray frameParams = project->get_frame()->get_parameters();
    frameParams.insert("resolution", res.asChar());
    frameParams.insert("tile_size", res);
    project->set_frame(
        asr::FrameFactory::create(
        "beauty",
        frameParams));
}

void AppleseedSwatchRenderer::setShader(MObject shader)
{
    defineMaterial(shader);
}

void AppleseedSwatchRenderer::mainLoop()
{
#ifdef _DEBUG
    Logging::setLogLevel(Logging::LevelDebug);
#endif

    Logging::debug("Starting AppleseedSwatchRenderer main loop.");

    SwatchesEvent swatchEvent;
    while (!terminateLoop)
    {
        SwatchesQueue.wait_and_pop(swatchEvent);

        if (swatchEvent.renderDone == 0)
        {
            Logging::debug(MString("AppleseedSwatchRenderer main Loop: received a null ptr. Terminating loop"));
            terminateLoop = true;
        }
        else
        {
            *swatchEvent.renderDone = true;
        }
    }
}

namespace
{
    void updateMaterial(MObject materialNode, const asr::Assembly *assembly)
    {
        OSLUtilClass OSLShaderClass;
        MObject surfaceShaderNode = getConnectedInNode(materialNode, "surfaceShader");
        MString surfaceShaderName = getObjectName(surfaceShaderNode);
        MString shadingGroupName = getObjectName(materialNode);
        ShadingNetwork network(surfaceShaderNode);
        size_t numNodes = network.shaderList.size();

        MString assName = "swatchRenderer_world";
        if (assName == assembly->get_name())
        {
            shadingGroupName = "previewSG";
        }

        MString shaderGroupName = shadingGroupName + "_OSLShadingGroup";

        asr::ShaderGroup *shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());

        if (shaderGroup != 0)
        {
            shaderGroup->clear();
        }
        else
        {
            asf::auto_release_ptr<asr::ShaderGroup> oslShadingGroup = asr::ShaderGroupFactory().create(shaderGroupName.asChar());
            assembly->shader_groups().insert(oslShadingGroup);
            shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());
        }

        OSLShaderClass.group = (OSL::ShaderGroup *)shaderGroup;

        MFnDependencyNode shadingGroupNode(materialNode);
        MPlug shaderPlug = shadingGroupNode.findPlug("surfaceShader");
        OSLShaderClass.createOSLProjectionNodes(shaderPlug);

        for (int shadingNodeId = 0; shadingNodeId < numNodes; shadingNodeId++)
        {
            ShadingNode snode = network.shaderList[shadingNodeId];
            Logging::debug(MString("ShadingNode Id: ") + shadingNodeId + " ShadingNode name: " + snode.fullName);
            if (shadingNodeId == (numNodes - 1))
                Logging::debug(MString("LastNode Surface Shader: ") + snode.fullName);
            OSLShaderClass.createOSLShadingNode(network.shaderList[shadingNodeId]);
        }

        OSLShaderClass.cleanupShadingNodeList();
        OSLShaderClass.createAndConnectShaderNodes();

        if (numNodes > 0)
        {
            ShadingNode snode = network.shaderList[numNodes - 1];
            MString layer = (snode.fullName + "_interface");
            Logging::debug(MString("Adding interface shader: ") + layer);
            asr::ShaderGroup *sg = (asr::ShaderGroup *)OSLShaderClass.group;
            sg->add_shader("surface", "surfaceShaderInterface", layer.asChar(), asr::ParamArray());
            const char *srcLayer = snode.fullName.asChar();
            const char *srcAttr = "outColor";
            const char *dstLayer = layer.asChar();
            const char *dstAttr = "inColor";
            Logging::debug(MString("Connecting interface shader: ") + srcLayer + "." + srcAttr + " -> " + dstLayer + "." + dstAttr);
            sg->add_connection(srcLayer, srcAttr, dstLayer, dstAttr);
        }

        MString physicalSurfaceName = shadingGroupName + "_physical_surface_shader";

        // add shaders only if they do not yet exist
        if (assembly->surface_shaders().get_by_name(physicalSurfaceName.asChar()) == 0)
        {
            assembly->surface_shaders().insert(
                asr::PhysicalSurfaceShaderFactory().create(
                physicalSurfaceName.asChar(),
                asr::ParamArray()));
        }
        if (assembly->materials().get_by_name(shadingGroupName.asChar()) == 0)
        {
            assembly->materials().insert(
                asr::OSLMaterialFactory().create(
                shadingGroupName.asChar(),
                asr::ParamArray()
                .insert("surface_shader", physicalSurfaceName.asChar())
                .insert("osl_surface", shaderGroupName.asChar())));
        }
    }
}

void AppleseedSwatchRenderer::defineMaterial(MObject shadingNode)
{
    MStatus status;
    // to use the unified material function we need the shading group
    // this works only for surface shaders, textures can be handled differently later
    MPlugArray pa, paOut;
    MFnDependencyNode depFn(shadingNode);
    depFn.getConnections(pa);
    asr::Assembly *assembly = project->get_scene()->assemblies().get_by_name("swatchRenderer_world");
    for (uint i = 0; i < pa.length(); i++)
    {
        if (pa[i].isDestination())
            continue;
        pa[i].connectedTo(paOut, false, true);
        if (paOut.length() > 0)
        {
            for (uint k = 0; k < paOut.length(); k++)
            {
                if (paOut[k].node().hasFn(MFn::kShadingEngine))
                {
                    MObject outNode = paOut[k].node();
                    updateMaterial(outNode, assembly);
                    break;
                }
            }
        }
    }
}

void AppleseedSwatchRenderer::startAppleseedSwatchRender(AppleseedSwatchRenderer *swRend)
{
    if (swRend != 0)
    {
        Logging::debug(MString("startAppleseedSwatchRender"));
        swRend->mainLoop();
        delete swRend;
        Logging::debug(MString("appleseedSwatchRender done and deleted."));
    }
}

void AppleseedSwatchRenderer::terminateAppleseedSwatchRender(AppleseedSwatchRenderer *swRend)
{
    Logging::debug(MString("terminateAppleseedSwatchRender"));
    SwatchesEvent swatchEvent;
    SwatchesQueue.push(swatchEvent);
    swRend->terminateLoop = true;
}
