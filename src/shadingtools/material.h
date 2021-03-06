
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

#ifndef SHADINGTOOLS_MATERIAL_H
#define SHADINGTOOLS_MATERIAL_H

// appleseed-maya headers.
#include "shadingnode.h"

// Maya headers.
#include <maya/MDagPath.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>

// Standard headers.
#include <vector>

/*
    A material collects all necessary object attributes for rendering a surface or volume.
    The material node in maya is called shadingGroup (shadingEngine).
    Here all shader types are connected. At the moment there are:
        - surface shader
        - displacement shader
        - volume shader
        - shadow shader
        - ...

    The main purpose of a material is the organisation and collection of shader nodes and
    the possiblity to translate them in another form, may it be an text shader source file for
    Renderman or for mantra or into a shading node network for an node based renderer.

    In maya behind every shader type stands a whole shading network. There can be a lot of nodes
    or simply none or e.g. only one node like a lambert shader. These nodes are collected in a list
    of shadings nodes.

    As soon as a Material node is defined, it parses the connected shaders based on the maya shading group connections.
    During the parsing of the network care is taken that already existing nodes are reused. This makes sure that we will have no
    infinite loops. Sometimes we have such loops in shading networks and these would cause an endless loop.

    Nodes can have a lot of connections, some of them can be useful others not. e.g. a color can be animated and has a connection to a anim curve.
    Not all nodes and connections are valid for all renderers. e.g. some renderers can map a color, but not the single components. This means a
    connection in maya to a single component will not be valid and instead of converting the whole graph, the current value of the color is used.
    This connection can be time dependent and because we can have a lot of these connections, it is recommended to translate the whole shading network
    for every frame, or at least update the elements and translated nodes.

    In general we have two possibilities of the shading workflow: A monolithic shader or a node based shading network.

    Monolithic shaders are shaders generated of a maya material and are compiled into one final shading node.

    Shading networks consist of multiple nodes which will be connected via API (or exported scene file) during translation.

    Both methods can have pros and cons. I prefer the node based approach because it is easier to implement and no external compiler has to be used.
    Either workflow will work with the parsed shader list. The result of the shader list is a MObject or a ShaderNode array. This array is exported,
    the very last nodes first.

    Let have a look at the following simple shading network:

               place2dF \
                         \
                         fileNode --> lambert.color --> surfaceShader
                         /
    place2dR -->ramp    /

    It will be translated into this list:

    place2dF
    place3dR
    ramp
    fileNode
    lambert

    If a system does not support a ramp the result will look like this because the list creation is cancelled as soon as the ramp is reached:

    place2dF
    fileNode
    lambert

    In a node based system the single nodes will be translated to their equivalent nodes in the renderer API. This is very renderer specific and should
    take place in a derived class.

    In a monolithic approach, every nodes will be translated in its source code and will be included into the final shader source code. It is a bit complicated,
    but much more flexible.

    To be able to know which connections and nodes are supported by the renderers, every renderer needs its own list of shaders and connections. This list will be
    parsed by the validConnection() and validNode() methods.

    Of course the amount of connections is high and we have a lot of nodes. Not all nodes are supported by a 3rd party renderer and not all connections on supported
    nodes are valid. To check if a node or a connection is supported, I read an external shader definition file. These are defined in the shaderDefs.h header file.
    One reason why we use an external file instead of e.g. a builtin header file, is that in case of a monolitic approach, where all shaders are defined via code snippets
    and will be compiled by a shader compiler, it can be done completly without recompiling the whole pluign. You only have to extend the shader definitions file, the
    corresponding shading code and it will work automatically.

    An additional problem occured with osl shading networks. In OSL we are forced to create the nodes in a way that connections are always done from a previous defined node
    to the later defined node. That means:
        - create node A
        - create node B
        - connect A->B is okay
        - connect B->A is invalid
    With the current approach this can lead to problems if we have something like this:

        place2dF \
            |     \
            |       fileNode --> lambert.color --> surfaceShader
            |            /
            |-- ramp----/

    The same placement node is connected to several other nodes. This can lead to the following list if we try to avoid duplicates:

    surfaceShader
    lambert
    fileNode
    place2d
    ramp

    But now the ramp needs an input from the placement node what would result in an invalid order. To avoid these problem I simply add all nodes and filter later for
    duplicates from the back. To avoid cycles, I check for a high amount of the same nodes (>100), simply because i dont know another useful solution.
*/

class ShadingNetwork
{
  public:
    ShadingNetwork();
    explicit ShadingNetwork(const MObject& node);

    void parseNetwork(const MObject& node);
    bool alreadyDefined(const ShadingNode& sn) const;
    void checkNodeList(MObjectArray& mobjectArray);
    bool hasValidShadingNodeConnections(const ShadingNode& source, const ShadingNode& dest) const;

    std::vector<ShadingNode> shaderList;
    MObject rootNode;
    MString rootNodeName;
};

class Material
{
  public:
    explicit Material(const MObject& shadingEngine);

    void printNodes(const ShadingNetwork& network) const;
    void parseNetworks();

    // here we save all nodes for a certain shader type connection
    ShadingNetwork surfaceShaderNet;
    ShadingNetwork volumeShaderNet;
    ShadingNetwork displacementShaderNet;
    ShadingNetwork shadowShaderNet;
    ShadingNetwork lightShaderNet;
    MObject shadingEngineNode;
    MString materialName;

  private:
    void parseNetwork(const MObject& shaderNode, ShadingNetwork& network);
    bool alreadyDefined(const ShadingNode& sn, const ShadingNetwork& network) const;
    void checkNodeList(MObjectArray& nodeList);
    bool hasValidShadingNodeConnections(const ShadingNode& source, const ShadingNode& dest) const;
    void cleanNetwork(ShadingNetwork& network); // remove any duplicates
};

#endif  // !SHADINGTOOLS_MATERIAL_H
