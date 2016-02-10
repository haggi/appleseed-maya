
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

// appleseed-maya headers.
#include "shadingtools/material.h"
#include "shadingtools/shadingutils.h"
#include "utilities/logging.h"
#include "utilities/oslutils.h"
#include "utilities/tools.h"
#include "newswatchrenderer.h"
#include "world.h"

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

// Maya headers.
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MPlugArray.h>

AppleseedSwatchRenderer::AppleseedSwatchRenderer()
  : mTerminateLoop(false)
{
    const MString swatchRenderFile = getRendererHome() + "resources/swatchRender.xml";
    const MString schemaPath = getRendererHome() + "schemas/project.xsd";
    mProject = renderer::ProjectFileReader().read(swatchRenderFile.asChar(), schemaPath.asChar());

    if (mProject.get() == 0)
    {
        Logging::error(MString("Unable to load swatch render file correctly: ") + swatchRenderFile);
        return;
    }

    MStringArray oslDirs;
    MGlobal::executePythonCommand("import appleseed.osltools as osl; osl.getOSODirs();", oslDirs, false, false);

    for (unsigned int i = 0; i < oslDirs.length(); i++)
        mProject->search_paths().push_back(oslDirs[i].asChar());

    mRenderer.reset(
        new renderer::MasterRenderer(
            mProject.ref(),
            mProject->configurations().get_by_name("final")->get_inherited_parameters(),
            &mRendererController));
}

void AppleseedSwatchRenderer::renderSwatch(NewSwatchRenderer *sr)
{
    const int res = sr->resolution();

    const MString resString = MString("") + res + " " + res;
    renderer::ParamArray frameParams = mProject->get_frame()->get_parameters();
    frameParams.insert("resolution", resString);
    frameParams.insert("tile_size", resString);
    mProject->set_frame(renderer::FrameFactory::create("beauty", frameParams));

    defineMaterial(sr->dNode);

    mRenderer->render();

    sr->image().create(res, res, 4, MImage::kFloat);

    float* pixels = sr->image().floatPixels();
    size_t index = 0;

    const foundation::Image& image = mProject->get_frame()->image();
    const foundation::CanvasProperties& props = image.properties();
    assert(props.m_channel_count == 4);

    const foundation::Tile& tile = mProject->get_frame()->image().tile(0, 0);

    for (size_t y = 0; y < props.m_canvas_height; y++)
    {
        for (size_t x = 0; x < props.m_canvas_width; x++)
        {
            for (size_t c = 0; c < 4; c++)
                pixels[index++] = tile.get_component<float>(x, y, c);
        }
    }
}

namespace
{
    void updateMaterial(MObject materialNode, const renderer::Assembly *assembly)
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

        renderer::ShaderGroup *shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());

        if (shaderGroup != 0)
        {
            shaderGroup->clear();
        }
        else
        {
            foundation::auto_release_ptr<renderer::ShaderGroup> oslShadingGroup = renderer::ShaderGroupFactory().create(shaderGroupName.asChar());
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
            renderer::ShaderGroup *sg = (renderer::ShaderGroup *)OSLShaderClass.group;
            sg->add_shader("surface", "surfaceShaderInterface", layer.asChar(), renderer::ParamArray());
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
                renderer::PhysicalSurfaceShaderFactory().create(
                    physicalSurfaceName.asChar(),
                    renderer::ParamArray()));
        }
        if (assembly->materials().get_by_name(shadingGroupName.asChar()) == 0)
        {
            assembly->materials().insert(
                renderer::OSLMaterialFactory().create(
                    shadingGroupName.asChar(),
                    renderer::ParamArray()
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
    renderer::Assembly *assembly = mProject->get_scene()->assemblies().get_by_name("swatchRenderer_world");
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
