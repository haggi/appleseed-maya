
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

#include "oslutils.h"
#include <maya/MPlugArray.h>
#include <maya/MFnDependencyNode.h>
#include "renderglobals.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "utilities/pystring.h"
#include "shadingtools/shaderdefs.h"
#include "world.h"

#include "boost/filesystem.hpp"

static std::vector<MObject> projectionNodes;
static std::vector<MObject> projectionConnectNodes;

namespace
{
    MString getParamName(MPlug plug)
    {
        MStringArray sa;
        MStatus stat;
        if (plug.isElement())
            plug = plug.array();
        plug.name().split('.', sa);
        return sa[sa.length() - 1];
    }
}

OSLUtilClass::OSLUtilClass()
{
    group = 0;
}

OSLUtilClass::~OSLUtilClass()
{
}

void OSLUtilClass::saveOSLNodeNameInArray(MString& oslNodeName)
{
    if (getWorldPtr()->renderType == MayaToWorld::SWATCHRENDER)
        this->definedOSLSWNodes.push_back(oslNodeName);
    else
        this->definedOSLNodes.push_back(oslNodeName);
}

bool OSLUtilClass::doesOSLNodeAlreadyExist(MString& oslNode)
{
    std::vector<MString> nodes;
    if (getWorldPtr()->renderType == MayaToWorld::SWATCHRENDER)
        nodes = definedOSLSWNodes;
    else
        nodes = definedOSLNodes;

    std::vector<MString>::iterator it;
    std::vector<MString> nodesList = nodes;
    for (it = nodesList.begin(); it != nodesList.end(); it++)
    {
        if (*it == oslNode)
            return true;
    }
    return false;
}

bool OSLUtilClass::doesOSLNodeAlreadyExist(MObject& oslNode)
{
    MString objName = getObjectName(oslNode);
    return doesOSLNodeAlreadyExist(objName);
}

void OSLUtilClass::listProjectionHistory(const MObject& mobject)
{
    MFnDependencyNode depFn(mobject);
    MStatus stat;
    MPlugArray pa;
    MObjectArray inputNodes;
    MString dn = depFn.name();
    depFn.getConnections(pa);
    for (uint i = 0; i < pa.length(); i++)
    {
        if (pa[i].isSource())
            continue;
        getConnectedInNodes(pa[i], inputNodes);
    }

    if (mobject.hasFn(MFn::kProjection))
    {
        ProjectionUtil util;
        util.projectionNode = mobject;
        projectionNodeArray.push_back(util);

        // found a projection node, clear all input plugs
        // and follow only the plugs connected to the "image" plug
        MPlug imagePlug = depFn.findPlug("image");
        MPlugArray imgPlugs;
        imagePlug.connectedTo(imgPlugs, true, false);
        if (imgPlugs.length() == 0)
            return;
        inputNodes.clear();
        for (uint i = 0; i < imgPlugs.length(); i++)
            inputNodes.append(imgPlugs[i].node());
    }

    // no more nodes, this is my leaf node
    if (inputNodes.length() == 0)
    {
        if (projectionNodeArray.size() > 0)
        {
            ProjectionUtil& util = projectionNodeArray.back();
            Logging::debug(MString("node ") + depFn.name() + " has no incoming connections this is my leaf node.");
            util.leafNodes.append(depFn.object());
        }
        return;
    }

    uniqueMObjectArray(inputNodes);

    for (uint i = 0; i < inputNodes.length(); i++)
    {
        MString nodeName = getObjectName(inputNodes[i]);
        Logging::debug(MString("Checking node ") + nodeName);
        listProjectionHistory(inputNodes[i]);
    }
}

void OSLUtilClass::defineOSLParameter(ShaderAttribute& sa, MFnDependencyNode& depFn, OSLParamArray& paramArray)
{
    MStatus stat;
    // if we have an array plug we have do deal with several plugs.
    // OSL is not able to connect nodes to array indices so we always have a list of parameters, e.g.
    // for a color array we have in a osl shader color0 color1 color2 ... at the moment we are limited to
    // several entries this way. I set it to ARRAY_MAX_ENTRIES.
    MPlug plug = depFn.findPlug(sa.name.c_str());
    if (plug.isArray())
    {
        int numEntries = plug.numElements();
        if (numEntries > ARRAY_MAX_ENTRIES)
        {
            Logging::warning(MString("Plug ") + plug.name() + " has more than 10 entries: " + numEntries + ", limiting to " + ARRAY_MAX_ENTRIES);
            numEntries = ARRAY_MAX_ENTRIES;
        }
        for (int i = 0; i < numEntries; i++)
        {
            MPlug p = plug[i];
            MString attrName = getCorrectOSLParameterName(p);

            if (p.isCompound() && (getAttributeNameFromPlug(plug) == "colorEntryList"))
            {
                // yes I know, hardcoded names are bad, bad. But if it works, I can
                MVector vec;
                // colorEntryList has child0 == position, child1 = color
                float position = p.child(0).asFloat();
                vec.x = p.child(1).child(0).asFloat(); // colorR
                vec.y = p.child(1).child(1).asFloat(); // colorG
                vec.z = p.child(1).child(2).asFloat(); // colorB

                MString posName = MString("position") + i;
                MString colName = MString("color") + i;
                paramArray.push_back(OSLParameter(colName, vec));
                paramArray.push_back(OSLParameter(posName, position));
            }
            else
            {
                // array index becomes nr, e.g. layeredShader.materialEntry[3] becomes materialEntry3
                // color closures cannot be set with color value
                int found = pystring::find(sa.type, "Closure");
                if (found == -1)
                {
                    if (sa.type == "color")
                    {
                        paramArray.push_back(OSLParameter(attrName.asChar(), getColorAttr(p)));
                    }
                    if (sa.type == "vector")
                    {
                        paramArray.push_back(OSLParameter(attrName.asChar(), getVectorAttr(p)));
                    }
                    if (sa.type == "float")
                    {
                        paramArray.push_back(OSLParameter(attrName.asChar(), p.asFloat()));
                    }
                    if (sa.type == "enumint")
                    {
                        paramArray.push_back(OSLParameter(attrName.asChar(), getEnumInt(p)));
                    }
                    if (sa.type == "string")
                    {
                        paramArray.push_back(OSLParameter(attrName.asChar(), p.asString()));
                    }
                }
            }
        }
        // all array using shader nodes will have an element with name numEntries
        paramArray.push_back(OSLParameter("numEntries", numEntries));
        return;
    }

    if (sa.type == "string")
    {
        // in OSL we don't have option menus, they are defined in our definition by string metadatas
        if (sa.optionMenu)
        {
            MString v = getEnumString(sa.name.c_str(), depFn);
            paramArray.push_back(OSLParameter(sa.name.c_str(), v));
        }
        else
        {
            MString stringParameter = getString(sa.name.c_str(), depFn);
            if (sa.name == "fileTextureName")
            {
                // to support udim textures we check if we have a file texture node here.
                // if so, we take the fileTextureName and seperate base, ext.
                if (depFn.object().hasFn(MFn::kFileTexture))
                {
                    std::string fileName(getString("fileTextureName", depFn).asChar());
                    int uvTilingMode = getIntAttr("uvTilingMode", depFn, 0);
                    if (uvTilingMode > 3)
                    {
                        Logging::error(MString("Uv Mode is not supported. Only ZBrush(1), Mudbox(2) and Mari(3) are supported."));
                        uvTilingMode = 0;
                    }

                    // we search for the base name. All patterns start with a '<' character, everything before is our base.
                    std::string fileNameWithTokens(getString("computedFileTextureNamePattern", depFn).asChar());
                    std::string baseFileName = fileName;
                    size_t pos = fileNameWithTokens.find("<");
                    if (pos == std::string::npos)
                        uvTilingMode = 0;
                    else
                    {
                        baseFileName = fileNameWithTokens.substr(0, pos - 1);
                        if (uvTilingMode == 3)
                            baseFileName = fileNameWithTokens.substr(0, pos);
                        paramArray.push_back(OSLParameter("baseName", baseFileName));
                    }
                    pos = fileName.rfind(".");
                    std::string ext = "";
                    if (pos == std::string::npos)
                    {
                        Logging::error(MString("Could not find a extension in file texture: ") + fileName.c_str());
                    }
                    else
                    {
                        ext = fileName.substr(pos + 1);
                        Logging::debug(MString("Extension for file texture: ") + fileName.c_str() + " is " + ext.c_str());
                        //if (ext == "exr")
                        //{
                        std::string txFileName = fileName + ".exr.tx";
                        boost::filesystem::path p(txFileName);
                        if (boost::filesystem::exists(p))
                        {
                            Logging::debug(MString("texture file has a .exr.tx extension, using ") + txFileName.c_str() + " instead of original one");
                            ext = ext + ".exr.tx";
                            if (uvTilingMode == 0)
                                stringParameter = txFileName.c_str();
                        }
                        //}
                        paramArray.push_back(OSLParameter("ext", ext));
                    }

                    paramArray.push_back(OSLParameter("uvTilingMode", uvTilingMode));
                }
            }
            paramArray.push_back(OSLParameter(sa.name.c_str(), stringParameter));
        }
    }
    if (sa.type == "float")
    {
        paramArray.push_back(OSLParameter(sa.name.c_str(), getFloatAttr(sa.name.c_str(), depFn, 0.0f)));
    }
    if (sa.type == "color")
    {
        // next to color attributes we can have color multipliers which are really useful sometimes
        MString multiplierName = MString("") + sa.name.c_str() + "Multiplier";
        MPlug multiplierPlug = depFn.findPlug(multiplierName, &stat);
        float multiplier = 1.0f;
        if (stat)
            multiplier = multiplierPlug.asFloat();
        paramArray.push_back(OSLParameter(sa.name.c_str(), getColorAttr(sa.name.c_str(), depFn) * multiplier));
    }
    if (sa.type == "int")
    {
        paramArray.push_back(OSLParameter(sa.name.c_str(), getIntAttr(sa.name.c_str(), depFn, 0)));
    }
    if (sa.type == "bool")
    {
        paramArray.push_back(OSLParameter(sa.name.c_str(), getBoolAttr(sa.name.c_str(), depFn, false)));
    }
    if (sa.type == "matrix")
    {
        MMatrix value = getMatrix(sa.name.c_str(), depFn);
        paramArray.push_back(OSLParameter(sa.name.c_str(), value));
    }
    if (sa.type == "vector")
    {
        // next to vector attributes we can have vector (vectors can be colors) multipliers which are really useful sometimes
        MString multiplierName = MString("") + sa.name.c_str() + "Multiplier";
        MPlug multiplierPlug = depFn.findPlug(multiplierName, &stat);
        float multiplier = 1.0f;
        if (stat)
            multiplier = multiplierPlug.asFloat();
        MVector v;
        if (sa.hint == "useAsColor")
        {
            MColor c = getColorAttr(sa.name.c_str(), depFn);
            v = MVector(c.r, c.g, c.b);
        }
        else
            v = getVectorAttr(sa.name.c_str(), depFn);
        paramArray.push_back(OSLParameter(sa.name.c_str(), v * multiplier));
    }
    if (sa.type == "enumint")
    {
        int v = getEnumInt(sa.name.c_str(), depFn);
        paramArray.push_back(OSLParameter(sa.name.c_str(), v));
    }
}

bool OSLUtilClass::doesHelperNodeExist(MString& helperNode)
{
    return doesOSLNodeAlreadyExist(helperNode);
}

void OSLUtilClass::addConnectionToConnectionArray(ConnectionArray& ca, MString sourceNode, MString sourceAtt, MString destNode, MString destAttr)
{
    Connection c(sourceNode, sourceAtt, destNode, destAttr);
    ca.push_back(c);
}

void OSLUtilClass::createOSLProjectionNodes(MPlug& plug)
{
    MPlugArray pa;
    plug.connectedTo(pa, true, false);
    if (pa.length() > 0)
    {
        createOSLProjectionNodes(pa[0].node());
    }
}

void OSLUtilClass::createOSLProjectionNodes(const MObject& surfaceShaderNode)
{
    listProjectionHistory(surfaceShaderNode);

    std::vector<ProjectionUtil>::iterator it;
    std::vector<ProjectionUtil> utils = projectionNodeArray;
    for (it = utils.begin(); it != utils.end(); it++)
    {
        ProjectionUtil util = *it;
        if ((util.leafNodes.length() == 0) || (util.projectionNode == MObject::kNullObj))
            continue;

        MString projectionNodeName = getObjectName(util.projectionNode);
        MFnDependencyNode proNode(util.projectionNode);
        MPlug pm = proNode.findPlug("placementMatrix");
        MPlugArray pma;
        pm.connectedTo(pma, true, false);
        if (pma.length() == 0)
            continue;
        MObject placementNode = pma[0].node();

        ShadingNode pn;
        if (!ShaderDefinitions::findShadingNode(placementNode, pn))
            continue;
        pn.fullName = pn.fullName + "_ProjUtil";
        createOSLShadingNode(pn);

        ShadingNode sn;
        if (!ShaderDefinitions::findShadingNode(placementNode, sn))
            continue;
        sn.fullName = sn.fullName + "_ProjUtil";
        createOSLShadingNode(sn);

        ConnectionArray ca;
        ca.push_back(Connection(pn.fullName, "worldInverseMatrix", sn.fullName, "placementMatrix"));
        connectOSLShaders(ca);

        Logging::debug(MString("Found projection node: ") + projectionNodeName);
        for (uint lId = 0; lId < util.leafNodes.length(); lId++)
        {
            Logging::debug(projectionNodeName + " has to be connected to " + getObjectName(util.leafNodes[lId]));
            projectionNodes.push_back(util.projectionNode);
            projectionConnectNodes.push_back(util.projectionNode);
        }
    }
}

bool OSLUtilClass::getConnectedPlugs(MPlug plug, MFnDependencyNode& depFn, MPlugArray& sourcePlugs, MPlugArray& destPlugs)
{
    MPlug destPlug = plug;
    if (destPlug.isNull())
        return false;

    MString dp = destPlug.name();

    MPlugArray pa;
    if (destPlug.isConnected() && !destPlug.isArray())
    {
        destPlug.connectedTo(pa, true, false);
        if (pa.length() == 0)
            return false;
        sourcePlugs.append(pa[0]);
        destPlugs.append(destPlug);
    }

    if (destPlug.isCompound() && (destPlug.numConnectedChildren() > 0))
    {
        for (uint chId = 0; chId < destPlug.numChildren(); chId++)
        {
            getConnectedPlugs(destPlug.child(chId), depFn, sourcePlugs, destPlugs);
        }
    }

    return true;
}

bool OSLUtilClass::handleSpecialPlugs(MString attributeName, MFnDependencyNode& depFn, MPlugArray& sourcePlugs, MPlugArray& destPlugs)
{
    if (attributeName == "colorEntryList")
    {
        MPlug destPlug = depFn.findPlug(attributeName);
        if (destPlug.isNull())
            return false;

        bool result;
        for (uint i = 0; i < destPlug.numElements(); i++)
        {
            MPlug colorEntry = destPlug[i];
            MPlug positionPlug = colorEntry.child(0);
            MPlug colorPlug = colorEntry.child(1);
            result = getConnectedPlugs(positionPlug, depFn, sourcePlugs, destPlugs);
            result = result && getConnectedPlugs(colorPlug, depFn, sourcePlugs, destPlugs);
        }
        return result;
    }

    // mayas bump2d node is terrible. It is no problem if you want to use a default bump.
    // but as soon as you want to use a normalMap, you have to find out the other node and connect it correctly by yourself.
    if (attributeName == "bumpValue")
    {
        if (depFn.typeName() == "bump2d")
        {
            int bumpType = getEnumInt("bumpInterp", depFn);
            // bumpType 0 == default texture bump, type 1 == tangentBased normal, type 2 == object based normal map
            if (bumpType > 0)
            {
                MPlug normalMap = depFn.findPlug("normalMap", true);
                // what I try to do is to find the source node and find something like outColor which we can connect to our normalMap attribute
                MObject inNode = getConnectedInNode(depFn.object(), "bumpDepth");
                if (inNode != MObject::kNullObj)
                {
                    Logging::debug(MString("Found connection to bump Node: ") + getObjectName(inNode) + " trying to find a color output");
                    MFnDependencyNode inDepFn(inNode);
                    MStatus stat;
                    MPlug outColor = inDepFn.findPlug("outColor", true, &stat);
                    if (!outColor.isNull())
                    {
                        Logging::debug(MString("Found outColor plug: ") + outColor.name());
                        sourcePlugs.append(outColor);
                        destPlugs.append(normalMap);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// check if the attribute is okay.
// check if it is connected and if the source attribute is okay and supported
bool OSLUtilClass::getConnectedPlugs(MString attributeName, MFnDependencyNode& depFn, MPlugArray& sourcePlugs, MPlugArray& destPlugs)
{
    std::vector<MString>  specialPlugs;
    specialPlugs.push_back("colorEntryList");

    // the bumpValue is a special case because it can be both a normal attribute or a special attribute
    if (attributeName == "bumpValue")
    {
        if (depFn.typeName() == "bump2d")
        {
            int bumpType = getEnumInt("bumpInterp", depFn);
            if (bumpType > 0)
            {
                specialPlugs.push_back("bumpValue");
            }
        }
    }
    std::vector<MString>::iterator it;
    std::vector<MString> nodesList = specialPlugs;
    for (it = nodesList.begin(); it != nodesList.end(); it++)
    {
        if (attributeName == *it)
        {
            return handleSpecialPlugs(attributeName, depFn, sourcePlugs, destPlugs);
        }
    }
    MPlug destPlug = depFn.findPlug(attributeName);
    if (destPlug.isNull())
        return false;

    // check if the plug is connected. This can be in multiple ways:
    // 1. The plug itself is connected (should be ignored if the plug is an array plug)
    // 2. The plug is an array plug and has connected elements
    // 3. The plug is a compound plug and has connected children
    // To keep things easy the ShadingNetwork only contains the main attribute, children and arrays should be handled automatically.

    // 1. plug is connected and is a scalar
    MPlugArray pa;
    if (destPlug.isConnected() && !destPlug.isArray())
    {
        destPlug.connectedTo(pa, true, false);
        if (pa.length() == 0)
            return false;
        sourcePlugs.append(pa[0]);
        destPlugs.append(destPlug);
    }
    // 3. Plug itself is not connected and is compound
    if (!destPlug.isConnected() && !destPlug.isArray() && destPlug.isCompound() && (destPlug.numConnectedChildren() > 0))
    {
        getConnectedPlugs(destPlug, depFn, sourcePlugs, destPlugs);
    }

    // 2. Plug itself is not connected and is compound
    if (destPlug.isArray())
    {
        for (uint elId = 0; elId < destPlug.numElements(); elId++)
        {
            getConnectedPlugs(destPlug[elId], depFn, sourcePlugs, destPlugs);
        }
    }

    return sourcePlugs.length() == destPlugs.length() && (destPlugs.length() > 0);
}

void OSLUtilClass::checkPlugsValidity(MPlugArray& sourcePlugs, MPlugArray& destPlugs)
{
    MPlugArray validSourcePlugs, validDestPlugs;
    for (uint pId = 0; pId < sourcePlugs.length(); pId++)
    {
        Logging::debug(MString("checking plug connection ") + sourcePlugs[pId].name() + "-->" + destPlugs[pId].name());
        MPlug sourcePlug = sourcePlugs[pId];
        // in the ShadingNetwork only the main attribute names are saved, so we need to find the parent first.
        while (sourcePlug.isChild())
            sourcePlug = sourcePlug.parent();
        if (sourcePlug.isElement())
            sourcePlug = sourcePlug.array();
        MString sourcePlugName = getParamName(sourcePlug);
        ShadingNode otherSideNode;
        if (!ShaderDefinitions::findShadingNode(sourcePlug.node(), otherSideNode))
            continue;
        if (!otherSideNode.isOutPlugValid(sourcePlugs[pId]))
            continue;
        validSourcePlugs.append(sourcePlugs[pId]);
        validDestPlugs.append(destPlugs[pId]);
    }
    sourcePlugs = validSourcePlugs;
    destPlugs = validDestPlugs;
}

void OSLUtilClass::getConnectionType(MPlug sourcePlug, MPlug destPlug, ConnectionType& type)
{
    // We need to know if we have to create helper nodes.
    // Helper nodes are required if a child plug is connected.
    if (sourcePlug.isChild())
    {
        if (destPlug.isChild())
        {
            type = COMP_TO_COMP;
            return;
        }
        type = COMP_TO_SCALAR;
        return;
    }
    if (sourcePlug.isCompound())
    {
        if (destPlug.isCompound())
        {
            type = VECTOR_TO_VECTOR;
            return;
        }
    }
    if (destPlug.isChild())
    {
        type = SCALAR_TO_COMP;
        return;
    }
    type = SCALAR_TO_SCALAR;
}


MString OSLUtilClass::getCorrectOSLParameterName(MPlug plug)
{
    MString plugName = plug.name();
    MString paramName = getParamName(plug);

    int found = pystring::find(plugName.asChar(), "colorEntryList");
    if (found > -1)
    {
        // a color entrylist is a list of compounds: color and position
        // check if we have a color or position entry or if we have a component e.g. colorR
        if (plug.parent().parent().isElement()) // component
        {
            plug = plug.parent();
        }
        if (plug.parent().isElement())
        {
            paramName = getParamName(plug);
            paramName += physicalIndex(plug);
        }
        return paramName;
    }

    // except the exceptions above we only support scalar and vector/color plugs and their arrays
    if (plug.isChild())
    {
        plug = plug.parent();
        paramName = getParamName(plug);
    }
    // array parameters will add their index to the name
    if (plug.isElement())
        paramName += physicalIndex(plug);
    return paramName;
}

// for helper nodes simply convert the name to a valid string
MString OSLUtilClass::getCleanParamName(MPlug plug)
{
    std::string plugName = plug.name().asChar();
    plugName = pystring::replace(plugName, "[", "_");
    plugName = pystring::replace(plugName, "]", "_");
    plugName = pystring::replace(plugName, ".", "_");
    return plugName.c_str();
}

void OSLUtilClass::fillVectorParam(OSLParamArray& paramArray, MPlug vectorPlug)
{
    const char *inAttributes[] = { "inX", "inY", "inZ" };
    if (vectorPlug.numChildren() != 3)
    {
        Logging::error(MString("Cannot fill osl vector parameter from non vector plug: ") + vectorPlug.name());
        return;
    }
    for (uint chId = 0; chId < 3; chId++)
        paramArray.push_back(OSLParameter(inAttributes[chId], vectorPlug.child(chId).asFloat()));
}

void OSLUtilClass::createHelperNode(MPlug sourcePlug, MPlug destPlug, ConnectionType type, std::vector<OSLNodeStruct>& oslNodes, ConnectionArray& connectionArray)
{
    const char *inAttributes[] = { "inX", "inY", "inZ" };
    const char *outAttributes[] = { "outX", "outY", "outZ" };
    OSLParamArray paramArray;

    switch (type)
    {
        // the new nodes will be prefixed with "in_" or "out_". This way they can be easily recognized and sorted later.
        case SCALAR_TO_COMP:
        {
            OSLNodeStruct oslNode;
            oslNode.typeName = "floatToVector";
            MString nodeName = MString("in_") + getCleanParamName(destPlug.parent()) + "_floatToVector";
            oslNode.nodeName = nodeName;
            fillVectorParam(oslNode.paramArray, destPlug.parent());
            addNodeToList(oslNode);
            Connection c;
            c.sourceNode = getObjectName(sourcePlug.node());
            c.sourceAttribute = getAttributeNameFromPlug(sourcePlug);
            c.destNode = nodeName;
            c.destAttribute = inAttributes[getChildId(destPlug)];
            addConnectionToList(c);
            c.sourceNode = nodeName;
            c.sourceAttribute = "outVector";
            c.destNode = getObjectName(destPlug.node());
            c.destAttribute = getCorrectOSLParameterName(destPlug);
            addConnectionToList(c);
        }
        break;
        case COMP_TO_SCALAR:
        {
            OSLNodeStruct oslNode;
            oslNode.typeName = "vectorToFloat";
            MString nodeName = MString("out_") + getCleanParamName(destPlug.parent()) + "_vectorToFloat";
            oslNode.nodeName = nodeName;
            addNodeToList(oslNode);
            Connection c;
            c.sourceNode = getObjectName(sourcePlug.node());
            c.sourceAttribute = getCorrectOSLParameterName(sourcePlug);
            c.destNode = nodeName;
            c.destAttribute = "inVector";
            addConnectionToList(c);
            c.sourceNode = nodeName;
            c.sourceAttribute = inAttributes[getChildId(sourcePlug)];
            c.destNode = getObjectName(destPlug.node());
            c.destAttribute = getCorrectOSLParameterName(destPlug);
            addConnectionToList(c);
        }
        break;
        case COMP_TO_COMP:
        {
            OSLNodeStruct oslNode;
            MString helperNodeNameOut = MString("out_") + getCleanParamName(sourcePlug.parent()) + "_vectorToFloat";
            oslNode.typeName = "vectorToFloat";
            oslNode.nodeName = helperNodeNameOut;
            addNodeToList(oslNode);
            Connection c;
            c.sourceNode = getObjectName(sourcePlug.node());
            c.sourceAttribute = getCorrectOSLParameterName(sourcePlug);
            c.destNode = helperNodeNameOut;
            c.destAttribute = "inVector";
            addConnectionToList(c);

            MString helperNodeNameIn = MString("in_") + getCleanParamName(destPlug.parent()) + "_floatToVector";
            oslNode.typeName = "floatToVector";
            oslNode.nodeName = helperNodeNameIn;
            fillVectorParam(oslNode.paramArray, destPlug.parent());
            addNodeToList(oslNode);
            c.sourceNode = helperNodeNameOut;
            c.sourceAttribute = outAttributes[getChildId(sourcePlug)];
            c.destNode = helperNodeNameIn;
            c.destAttribute = inAttributes[getChildId(destPlug)];
            addConnectionToList(c);

            c.sourceNode = helperNodeNameIn;
            c.sourceAttribute = "outVector";
            c.destNode = getObjectName(destPlug.node());
            c.destAttribute = getCorrectOSLParameterName(destPlug);
            addConnectionToList(c);
        }
        break;
    }
}

// we have two lists: the local list of nodes/helpernodes for the current shading node and the global node list with all nodes in the shading group.
void OSLUtilClass::addNodeToList(OSLNodeStruct node)
{
    bool found = false;
    std::vector<OSLNodeStruct>::iterator it;
    std::vector<OSLNodeStruct> nodesList = oslNodeArray;
    for (it = nodesList.begin(); it != nodesList.end(); it++)
        if (it->nodeName == node.nodeName)
            found = true;
    if (!found)
    {
        if (!doesOSLNodeAlreadyExist(node.nodeName))
        {
            oslNodeArray.push_back(node);
            saveOSLNodeNameInArray(node.nodeName);
        }
    }
}

void OSLUtilClass::addConnectionToList(Connection c)
{
    std::vector<Connection>::iterator it;
    std::vector<Connection> conns = connectionList;
    for (it = conns.begin(); it != conns.end(); it++)
    {
        if (*it == c)
            return;
    }
    connectionList.push_back(c);
}

// we cannot avoid to add some helper nodes in a non correct order.
// e.g. if we first connect a float to a component, maybe a outAlpha to a color.r, then automatically a
// floatToVector node is created. If we then connect a component to a component, a outColor.g to a color.b
// a vectorToFloat node is created and added to the osl node list. If we now try to connect the output of the
// vectorToFloat node to the previous created node floatToVector, we get an error.
// For this reason we prefix the helperNodes with in_ and out_ and sort them in the correct order.

// a helper node always starts with in_ or out_ and with nodeName follwed by a _
// so for every node we first search for a in_nodename_ node then a out_nodename_
// this should transform the example above from:
// nodeA, in_floatToVector, out_vectorToFloat, nodeB  to nodeA, out_vectorToFloat, in_vectorToFloat, nodeB
void OSLUtilClass::cleanupShadingNodeList()
{
    MStringArray sa;
    std::vector<OSLNodeStruct> cleanNodeArray;

    std::vector<OSLNodeStruct>::iterator it;
    std::vector<OSLNodeStruct> nodesList = oslNodeArray;
    for (it = nodesList.begin(); it != nodesList.end(); it++)
    {
        OSLNodeStruct node = *it;
        if (pystring::startswith(node.nodeName.asChar(), "in_") || pystring::startswith(node.nodeName.asChar(), "out_"))
            continue;
        MString inNodePattern = MString("in_") + node.nodeName;
        MString outNodePattern = MString("out_") + node.nodeName;
        std::vector<OSLNodeStruct>::iterator hit;
        std::vector<OSLNodeStruct> hnodesList = oslNodeArray;
        for (hit = hnodesList.begin(); hit != hnodesList.end(); hit++)
        {
            OSLNodeStruct helperNode = *hit;
            if (!pystring::startswith(helperNode.nodeName.asChar(), "in_"))
                continue;
            if (pystring::startswith(helperNode.nodeName.asChar(), inNodePattern.asChar()))
            {
                cleanNodeArray.push_back(helperNode);
            }
        }
        cleanNodeArray.push_back(node);

        for (hit = hnodesList.begin(); hit != hnodesList.end(); hit++)
        {
            OSLNodeStruct helperNode = *hit;
            if (!pystring::startswith(helperNode.nodeName.asChar(), "out_"))
                continue;
            if (pystring::startswith(helperNode.nodeName.asChar(), outNodePattern.asChar()))
            {
                cleanNodeArray.push_back(helperNode);
            }
        }
    }
    oslNodeArray = cleanNodeArray;
}

void OSLUtilClass::createAndConnectShaderNodes()
{
    std::vector<OSLNodeStruct>::iterator it;
    std::vector<OSLNodeStruct> nodesList = oslNodeArray;
    for (it = nodesList.begin(); it != nodesList.end(); it++)
    {
        OSLNodeStruct node = *it;
        Logging::debug(MString("NEW: Creating shading node: ") + node.nodeName + " type: " + node.typeName);
        createOSLShader(node.typeName, node.nodeName, node.paramArray);
    }

    std::vector<Connection>::iterator cit;
    std::vector<Connection> conns = connectionList;
    for (cit = conns.begin(); cit != conns.end(); cit++)
    {
        Logging::debug(MString("NEW: Creating connection from: ") + cit->sourceNode + "." + cit->sourceAttribute + " --> " + cit->destNode + "." + cit->destAttribute);
    }
    connectOSLShaders(connectionList);
}

void OSLUtilClass::createOSLShadingNode(ShadingNode& snode)
{
    MFnDependencyNode depFn(snode.mobject);

    const char *inAttributes[] = { "inX", "inY", "inZ" };
    const char *outAttributes[] = { "outX", "outY", "outZ" };

    // we create all necessary nodes for input and output connections
    // the problem is that we have to create the nodes in the correct order,
    // e.g. create node A, create node B connect B->A is not valid, only A->B
    for (uint i = 0; i < snode.inputAttributes.size(); i++)
    {
        ShaderAttribute& sa = snode.inputAttributes[i];
        MPlugArray sourcePlugs, destPlugs;
        if (!getConnectedPlugs(sa.name.c_str(), depFn, sourcePlugs, destPlugs))
            continue;

        // now we have a bunch of source and dest plugs which should be conneced together
        // sourcePlugs[0] -> destPlugs[0]... lets see if they are all valid
        checkPlugsValidity(sourcePlugs, destPlugs);

        if (sourcePlugs.length() == 0)
            continue;

        for (uint pId = 0; pId < sourcePlugs.length(); pId++)
        {
            ConnectionType type;
            MString sp = sourcePlugs[pId].name();
            MString dp = destPlugs[pId].name();
            getConnectionType(sourcePlugs[pId], destPlugs[pId], type);

            if (type < SCALAR_TO_COMP)
            {
                Connection c;
                c.sourceNode = getObjectName(sourcePlugs[pId].node());
                c.sourceAttribute = getAttributeNameFromPlug(sourcePlugs[pId]);
                c.destNode = getObjectName(snode.mobject);
                c.destAttribute = getCorrectOSLParameterName(destPlugs[pId]);
                addConnectionToList(c);
            }
            else
            {
                OSLNodeStruct oslNode;
                // we have component connections and need helper nodes
                createHelperNode(sourcePlugs[pId], destPlugs[pId], type, oslNodeArray, connectionList);
            }
        }
    }

    OSLParamArray paramArray;
    for (uint i = 0; i < snode.inputAttributes.size(); i++)
    {
        ShaderAttribute& sa = snode.inputAttributes[i];
        defineOSLParameter(sa, depFn, paramArray);
    }
    OSLNodeStruct oslNode;
    oslNode.typeName = snode.typeName;
    oslNode.nodeName = snode.fullName;
    oslNode.paramArray = paramArray;
    addNodeToList(oslNode);
}

void OSLUtilClass::initOSLUtil()
{
    projectionNodes.clear();
    projectionConnectNodes.clear();
    definedOSLNodes.clear();
    definedOSLSWNodes.clear();
}

void OSLUtilClass::connectProjectionNodes(MObject& projNode)
{
    ConnectionArray ca;
    MFnDependencyNode pn(projNode);
    for (size_t i = 0; i < projectionConnectNodes.size(); i++)
    {
        if (projNode == projectionConnectNodes[i])
        {
            Logging::debug(MString("Place3dNode for projection input defined ") + pn.name());
            MString sourceNode = (getObjectName(projectionNodes[i]) + "_ProjUtil");
            MString sourceAttr = "outUVCoord";
            MString destNode = pn.name();
            MString destAttr = "uvCoord";
            ca.push_back(Connection(sourceNode, sourceAttr, destNode, destAttr));
            connectOSLShaders(ca);
        }
    }
}
