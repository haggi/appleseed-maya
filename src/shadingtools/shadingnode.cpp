
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
#include "shadingnode.h"

// appleseed-maya headers.
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "utilities/pystring.h"

// Maya headers.
#include <maya/MPlugArray.h>

ShadingNode::ShadingNode()
{
}

ShadingNode::ShadingNode(const MObject& mobj)
{
    setMObject(mobj);
}

ShadingNode::ShadingNode(const ShadingNode& other)
  : inputAttributes(other.inputAttributes)
  , outputAttributes(other.outputAttributes)
{
    typeName = other.typeName;
    fullName = other.fullName;
    setMObject(other.mobject);
}

void ShadingNode::setMObject(const MObject& mobj)
{
    mobject = mobj;
    if (mobject != MObject::kNullObj)
    {
        typeName = getDepNodeTypeName(mobject);
        fullName = getObjectName(mobject);
    }
}

bool ShadingNode::isInPlugValid(const MPlug& plug) const
{
    MPlug tmpPlug = plug;

    if (!tmpPlug.isDestination())
        return false;

    while (tmpPlug.isChild())
        tmpPlug = tmpPlug.parent();

    // if we have an array, check the main plug
    if (tmpPlug.isElement())
        tmpPlug = tmpPlug.array();

    const MString& plugName = getAttributeNameFromPlug(tmpPlug);

    for (size_t inattrId = 0; inattrId < inputAttributes.size(); inattrId++)
    {
        if (plugName == inputAttributes[inattrId].name.c_str())
            return true;
    }
    return false;
}

bool ShadingNode::isOutPlugValid(const MPlug& plug) const
{
    MPlug tmpPlug = plug;

    if (!tmpPlug.isSource())
        return false;

    while (tmpPlug.isChild())
        tmpPlug = tmpPlug.parent();

    // if we have an array, check the main plug
    if (tmpPlug.isElement())
        tmpPlug = tmpPlug.array();

    MString plugName = getAttributeNameFromPlug(tmpPlug);

    for (size_t attrId = 0; attrId < outputAttributes.size(); attrId++)
    {
        if (plugName == outputAttributes[attrId].name.c_str())
            return true;
    }
    return false;
}

bool ShadingNode::isAttributeValid(const MString& attributeName) const
{
    MFnDependencyNode depFn(mobject);
    MPlugArray pa;
    depFn.getConnections(pa);
    for (uint pId = 0; pId < pa.length(); pId++)
    {
        if (pa[pId].isDestination())
        {
            MPlug parentPlug = pa[pId];
            while (parentPlug.isChild())
                parentPlug = parentPlug.parent();
            MString plugName = getAttributeNameFromPlug(parentPlug);
            if (plugName == attributeName)
            {
                for (size_t inattrId = 0; inattrId < inputAttributes.size(); inattrId++)
                {
                    if (attributeName == inputAttributes[inattrId].name.c_str())
                        return true;
                }
            }
        }
    }

    return false;
}

void ShadingNode::getConnectedInputObjects(MObjectArray& objectArray) const
{
    MStatus stat;
    MFnDependencyNode depFn(mobject);
    MStringArray aliasArray;
    depFn.getAliasList(aliasArray);
    MObjectArray objectList;
    MPlugArray connections;
    depFn.getConnections(connections);

    for (uint connId = 0; connId < connections.length(); connId++)
    {
        MPlug p = connections[connId];
        if (!p.isDestination())
            continue;

        // a connection can be a direct connection or a child connection e.g. colorR, colorG...
        // but in a shader description file only the main attribute is listed so we go up until we have the main plug
        MPlug mainPlug = p;
        while (mainPlug.isChild())
            mainPlug = mainPlug.parent();
        if (mainPlug.isElement())
            mainPlug = mainPlug.array();
        MStringArray stringArray;
        // name contains node.attributeName, so we have to get rid of the nodeName
        mainPlug.name().split('.', stringArray);
        MString plugName = stringArray[stringArray.length() - 1];
        if (!isAttributeValid(plugName))
            continue;
        getConnectedInNodes(p, objectList);
        makeUniqueArray(objectList);
    }
    objectArray = objectList;
}
