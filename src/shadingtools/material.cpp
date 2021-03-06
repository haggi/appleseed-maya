
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
#include "material.h"

// appleseed-maya headers.
#include "shadingtools/shaderdefinitions.h"
#include "utilities/attrtools.h"
#include "utilities/tools.h"
#include "utilities/logging.h"
#include "utilities/pystring.h"

// Maya headers.
#include <maya/MFnDependencyNode.h>
#include <maya/MPlugArray.h>
#include <maya/MFnParticleSystem.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>

ShadingNetwork::ShadingNetwork()
{
}

ShadingNetwork::ShadingNetwork(const MObject& node) : rootNode(node)
{
    rootNodeName = getObjectName(node);
    parseNetwork(rootNode);
}

bool ShadingNetwork::hasValidShadingNodeConnections(const ShadingNode& source, const ShadingNode& dest) const
{
    MFnDependencyNode destFn(dest.mobject);
    MPlugArray plugArray;
    MPlugArray validPlugs;
    destFn.getConnections(plugArray);
    for (uint pId = 0; pId < plugArray.length(); pId++)
    {
        if (dest.isInPlugValid(plugArray[pId]))
            validPlugs.append(plugArray[pId]);
    }

    for (uint pId = 0; pId < validPlugs.length(); pId++)
    {
        MPlugArray sourcePlugs;
        validPlugs[pId].connectedTo(sourcePlugs, true, false);
        for (uint spId = 0; spId < sourcePlugs.length(); spId++)
        {
            if (source.isOutPlugValid(sourcePlugs[spId]))
                return true;
        }
    }

    return false;
}

void ShadingNetwork::parseNetwork(const MObject& shaderNode)
{
    ShadingNode sn;
    if (!ShaderDefinitions::findShadingNode(shaderNode, sn))
    {
        Logging::debug(MString("Node is not supported (INVALID): ") + getObjectName(shaderNode));
        return;
    }

    // to avoid any dg node cycles
    if (alreadyDefined(sn))
    {
        Logging::debug("Node is already defined, skipping.");
        return;
    }

    MObjectArray connectedNodeList;
    sn.getConnectedInputObjects(connectedNodeList);
    checkNodeList(connectedNodeList);

    Logging::debug(MString("Node ") + sn.fullName + " has " + connectedNodeList.length() + " input connections.");

    for (uint i = 0; i < connectedNodeList.length(); i++)
    {
        ShadingNode source;
        if (ShaderDefinitions::findShadingNode(connectedNodeList[i], source))
        {
            if (hasValidShadingNodeConnections(source, sn))
                parseNetwork(source.mobject);
        }
    }

    shaderList.push_back(sn);
}

bool ShadingNetwork::alreadyDefined(const ShadingNode& sn) const
{
    for (int i = 0; i < (int)shaderList.size(); i++)
    {
        if (shaderList[i].mobject == sn.mobject)
            return true;
    }

    return false;
}

void ShadingNetwork::checkNodeList(MObjectArray& mobjectArray)
{
    MObjectArray cleanArray;
    for (uint oId = 0; oId < mobjectArray.length(); oId++)
    {
        ShadingNode sn;
        if (ShaderDefinitions::findShadingNode(mobjectArray[oId], sn))
            cleanArray.append(mobjectArray[oId]);
    }

    mobjectArray = cleanArray;
}

void Material::checkNodeList(MObjectArray& nodeList)
{
    MObjectArray cleanArray;
    for (uint oId = 0; oId < nodeList.length(); oId++)
    {
        ShadingNode sn;
        if (ShaderDefinitions::findShadingNode(nodeList[oId], sn))
            cleanArray.append(nodeList[oId]);
    }
    nodeList = cleanArray;
}

// already defined means it appears more than 100 times. See material.h for a detailed explanation.
bool Material::alreadyDefined(const ShadingNode& sn, const ShadingNetwork& network) const
{
    int occurences = 0;
    std::vector<ShadingNode>::iterator it;
    std::vector<ShadingNode> shaders = network.shaderList;
    for (it = shaders.begin(); it != shaders.end(); it++)
    {
        if (it->mobject == sn.mobject)
            occurences++;
    }

    return occurences < 100;
}

// "source" is the node which is connected to the inputs of "dest"
// the dest connections are already checked previously so we only have to check the
// outgoing connections from "source" for validity
bool Material::hasValidShadingNodeConnections(const ShadingNode& source, const ShadingNode& dest) const
{
    MFnDependencyNode destFn(dest.mobject);
    MPlugArray plugArray;
    MPlugArray validPlugs;
    destFn.getConnections(plugArray);
    for (uint pId = 0; pId < plugArray.length(); pId++)
    {
        if (dest.isInPlugValid(plugArray[pId]))
            validPlugs.append(plugArray[pId]);
    }

    for (uint pId = 0; pId < validPlugs.length(); pId++)
    {
        MPlugArray sourcePlugs;
        validPlugs[pId].connectedTo(sourcePlugs, true, false);
        for (uint spId = 0; spId < sourcePlugs.length(); spId++)
        {
            if (source.isOutPlugValid(sourcePlugs[spId]))
                return true;
        }
    }

    return false;
}

void Material::parseNetwork(const MObject& shaderNode, ShadingNetwork& network)
{
    ShadingNode sn;
    if (!ShaderDefinitions::findShadingNode(shaderNode, sn))
    {
        Logging::debug(MString("Node is not supported (INVALID): ") + getObjectName(shaderNode));
        return;
    }


    // to avoid any dg node cycles
    if (alreadyDefined(sn, network))
    {
        Logging::debug("Node is already defined, skipping.");
        return;
    }

    MObjectArray connectedNodeList;
    sn.getConnectedInputObjects(connectedNodeList);
    checkNodeList(connectedNodeList);

    Logging::debug(MString("Node ") + sn.fullName + " has " + connectedNodeList.length() + " input connections.");

    for (uint i = 0; i < connectedNodeList.length(); i++)
    {
        ShadingNode source;
        if (ShaderDefinitions::findShadingNode(connectedNodeList[i], source))
        {
            if (hasValidShadingNodeConnections(source, sn))
                parseNetwork(source.mobject, network);
        }
    }

    network.shaderList.push_back(sn);
}

void Material::printNodes(const ShadingNetwork& network) const
{
    int numNodes = (int)network.shaderList.size();
    for (int i = numNodes - 1; i >= 0 ; i--)
    {
        ShadingNode sn = network.shaderList[i];
        Logging::info(MString("Material::Node id ") + (double)i + " typename " + sn.typeName);
        Logging::info(MString("Material::Node id ") + (double)i + " mayaname " + sn.fullName);
    }
}

void Material::cleanNetwork(ShadingNetwork& network)
{
    std::vector<ShadingNode> cleanList;
    std::vector<ShadingNode>::iterator it;
    std::vector<ShadingNode> shaders = network.shaderList;
    for (it = shaders.begin(); it != shaders.end(); it++)
    {
        bool found = false;
        std::vector<ShadingNode>::iterator cit;
        std::vector<ShadingNode> cshaders = cleanList;
        for (cit = cshaders.begin(); cit != cshaders.end(); cit++)
        {
            if (*cit == *it)
                found = true;
        }
        if (!found)
            cleanList.push_back(*it);
    }

    network.shaderList = cleanList;
}

void Material::parseNetworks()
{
    MObject surfaceShaderNode = getOtherSideNode(MString("surfaceShader"), shadingEngineNode);
    MObject volumeShaderNode = getOtherSideNode(MString("volumeShader"), shadingEngineNode);
    MObject displacementShaderNode = getOtherSideNode(MString("displacementShader"), shadingEngineNode);

    // if a mr material is connected, check it and use it instead of the normal shader connection
    // mr can be supported because it is the default maya renderer
    MObject miMaterialShaderNode = getOtherSideNode(MString("miMaterialShader"), shadingEngineNode);
    MObject miVolumeShaderNode = getOtherSideNode(MString("miVolumeShader"), shadingEngineNode);
    MObject miDisplacementShaderNode = getOtherSideNode(MString("miDisplacementShader"), shadingEngineNode);

    // read surface shader hierarchy
    // if mr node is defined, use it, if not use the normal one
    if ((miMaterialShaderNode != MObject::kNullObj) || (surfaceShaderNode != MObject::kNullObj))
    {
        if (miMaterialShaderNode != MObject::kNullObj)
            surfaceShaderNode = miMaterialShaderNode;

        parseNetwork(surfaceShaderNode, surfaceShaderNet);
    }

    // read volume shader hierarchy
    // if mr node is defined, use it, if not use the normal one
    if ((miVolumeShaderNode != MObject::kNullObj) || (volumeShaderNode != MObject::kNullObj))
    {
        if (miVolumeShaderNode != MObject::kNullObj)
            volumeShaderNode = miVolumeShaderNode;
        parseNetwork(miVolumeShaderNode, volumeShaderNet);
    }

    // read displacement shader hierarchy
    // if mr node is defined, use it, if not use the normal one
    if ((miDisplacementShaderNode != MObject::kNullObj) || (displacementShaderNode != MObject::kNullObj))
    {
        if (miDisplacementShaderNode != MObject::kNullObj)
            displacementShaderNode = miDisplacementShaderNode;

        parseNetwork(displacementShaderNode, displacementShaderNet);
    }

    // read light shader hierarchy
    if ((shadingEngineNode != MObject::kNullObj) && (shadingEngineNode.hasFn(MFn::kLight)))
        parseNetwork(shadingEngineNode, lightShaderNet);

    cleanNetwork(surfaceShaderNet);
    cleanNetwork(volumeShaderNet);
    cleanNetwork(displacementShaderNet);
    cleanNetwork(lightShaderNet);
}

Material::Material(const MObject &shadingEngine)
{
    shadingEngineNode = shadingEngine;
    materialName = getObjectName(shadingEngineNode);
    parseNetworks();
    printNodes(surfaceShaderNet);
}
