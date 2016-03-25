
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
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MFnRenderLayer.h>
#include <maya/MStringArray.h>
#include <maya/MString.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MSelectionList.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MFnAttribute.h>

#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "utilities/pystring.h"

bool isCameraRenderable(MObject obj)
{
    MFnDependencyNode camFn(obj);
    bool renderable = true;
    getBool(MString("renderable"), camFn, renderable);
    if (renderable)
        return true;
    return false;
}

void setRendererHome(MString home)
{
    RendererHome = home;
}

MString getRendererHome()
{
    return RendererHome;
}

MObject getRenderGlobalsNode()
{
    return objectFromName("appleseedGlobals");
}

float rnd()
{
    return static_cast<float>(rand()) / RAND_MAX;
}

// replace :,| and . by _ ... the modern version
MString makeGoodString(const MString& oldString)
{
    std::string old(oldString.asChar());
    std::string newString;
    newString = pystring::replace(old, ":", "__");
    newString = pystring::replace(newString, "|", "_");
    newString = pystring::replace(newString, ".", "_");
    return MString(newString.c_str());
}

bool IsPathVisible(MDagPath& dp)
{
   MStatus stat = MStatus::kSuccess;
   MDagPath dagPath = dp;
   while (stat == MStatus::kSuccess)
   {
      MFnDagNode node(dagPath.node());
      if (!IsVisible(node))
      {
         return false;
      }
      stat = dagPath.pop();
   }
   return true;
}

bool IsVisible(MDagPath& node)
{
    MFnDagNode dagNode(node.node());
    if (!IsVisible(dagNode) || IsTemplated(dagNode) || IsPathTemplated(node)  || !IsInRenderLayer(node) || !IsPathVisible(node) || !IsLayerVisible(node))
        return false;
    return true;
}


bool IsVisible(MFnDagNode& node)
{
    MStatus stat;

    if (node.isIntermediateObject())
        return false;

    bool visibility = true;
    MFnDependencyNode depFn(node.object(), &stat);
    if (!stat)
        MGlobal::displayInfo("Problem getting dep from " + node.name());

    if (!getBool(MString("visibility"), depFn, visibility))
        MGlobal::displayInfo("Problem getting visibility attr from " + node.name());

    if (!visibility)
        return false;

    getBool(MString("overrideVisibility"), depFn, visibility);
    if (!visibility)
        return false;

    return true;
}

bool IsTemplated(MFnDagNode& node)
{
   MStatus status;

   MFnDependencyNode depFn(node.object());
   bool isTemplate = false;
   getBool(MString("template"), depFn, isTemplate);
   if (isTemplate)
       return true;
   int intTempl = 0;
   getInt(MString("overrideDisplayType"), depFn, intTempl);
   if (intTempl == 1)
       return true;

   return false;
}

bool IsPathTemplated(MDagPath& path)
{
    MStatus stat = MStatus::kSuccess;
    while (stat == MStatus::kSuccess)
    {
        MFnDagNode node;
        node.setObject(path.node());
        if (IsTemplated(node))
            return true;
        stat = path.pop();
    }
    return false;
}

bool IsLayerVisible(MDagPath& dp)
{
   MStatus stat = MStatus::kSuccess;
   MDagPath dagPath = dp;
   while (stat == MStatus::kSuccess)
   {
      MFnDependencyNode node(dagPath.node());
      MPlug doPlug = node.findPlug("drawOverride", &stat);
      if (stat)
      {
          MObject layer = getOtherSideNode(doPlug);
          MFnDependencyNode layerNode(layer, &stat);
          if (stat)
          {
              bool visibility = true;
              if (getBool("visibility", layerNode, visibility))
                  if (!visibility)
                      return false;
              if (getEnumInt("displayType", layerNode) == 1) // template
                  return false;
          }
      }
      stat = dagPath.pop();
   }
   return true;
}

bool IsInRenderLayer(MDagPath& dagPath)
{
   MObject renderLayerObj = MFnRenderLayer::currentLayer();
   MFnRenderLayer curLayer(renderLayerObj);
   bool isInRenderLayer = curLayer.inCurrentRenderLayer(dagPath);
   if (isInRenderLayer)
      return true;
   else
      return false;
}

MObject getUpstreamMesh(MString& outputPlugName, MObject thisObject)
{
    return MObject::kNullObj;
}

//
// simply get the node wich is connected to the named plug.
// If we have no connection, a knullptrObject is returned.
//
MObject getOtherSideNode(const MString& plugName, MObject& thisObject)
{
    MStatus stat;
    MObject result = MObject::kNullObj;
    MFnDependencyNode depFn(thisObject, &stat); if (stat != MStatus::kSuccess) return result;
    MPlug plug = depFn.findPlug(plugName, &stat);   if (stat != MStatus::kSuccess) return result;
    MPlugArray plugArray;
    plug.connectedTo(plugArray, 1, 0, &stat);if (stat != MStatus::kSuccess) return result;
    if (plugArray.length() == 0)
        return result;
    MPlug otherSidePlug = plugArray[0];
    result = otherSidePlug.node();
    return result;
}

MObject getOtherSideNode(const MString& plugName, MObject& thisObject, MStringArray& otherSidePlugNames)
{
    MStatus stat;
    MObject result = MObject::kNullObj;
    MFnDependencyNode depFn(thisObject, &stat); if (stat != MStatus::kSuccess) return result;
    MPlug plug = depFn.findPlug(plugName, &stat);   if (stat != MStatus::kSuccess)return result;
    if (!plug.isConnected())
    {
        int numChildConnects = plug.numConnectedChildren();
        if (numChildConnects == 0)
            return result;
        else
        {
            for (int i = 0; i < numChildConnects; i++)
            {
                MPlug child = plug.child(i);
                MString otherSidePlugName;
                MObject childObj = getOtherSideNode(child.partialName(false), thisObject, otherSidePlugName);
                if (childObj != MObject::kNullObj)
                {
                    otherSidePlugNames.append(otherSidePlugName);
                    result = childObj;
                }else
                    otherSidePlugNames.append(MString(""));
            }
        }
    }
    else
    {
        MPlugArray plugArray;
        plug.connectedTo(plugArray, 1, 0, &stat);if (stat != MStatus::kSuccess) return result;
        if (plugArray.length() == 0)
            return result;
        MPlug otherSidePlug = plugArray[0];
        result = otherSidePlug.node();
        otherSidePlugNames.append(otherSidePlug.name());
    }
    return result;
}

bool isConnected(const char *attrName, MObject& node, bool dest)
{
    MFnDependencyNode depFn(node);
    return isConnected(attrName, depFn, dest, true);
}

// gives the plug on the other side of this connected plug, children are ignored
MPlug getDirectConnectedPlug(const char *attrName, MFnDependencyNode& depFn, bool dest)
{
    MPlug thisPlug = depFn.findPlug(attrName);
    MPlug returnPlug;
    MPlugArray pa;
    thisPlug.connectedTo(pa, true, false);
    if (pa.length() > 0)
    {
        returnPlug = pa[0];
    }
    return returnPlug;
}

void getConnectedChildPlugs(const MPlug& plug, bool dest, MPlugArray& thisNodePlugs, MPlugArray& otherSidePlugs)
{
    if (plug.numConnectedChildren() == 0)
        return;
    for (uint chId = 0; chId < plug.numChildren(); chId++)
        if (plug.child(chId).isConnected())
        {
            if ((plug.child(chId).isDestination() && dest) || (plug.child(chId).isSource() && !dest))
            {
                thisNodePlugs.append(plug.child(chId));
                otherSidePlugs.append(getDirectConnectedPlug(plug, dest));
            }
        }
}

void getConnectedChildPlugs(const char *attrName, MFnDependencyNode& depFn, bool dest, MPlugArray& thisNodePlugs, MPlugArray& otherSidePlugs)
{
    MPlug p = depFn.findPlug(attrName);
    if (p.isCompound() && !p.isArray())
    {
        getConnectedChildPlugs(p, dest, thisNodePlugs, otherSidePlugs);
        return;
    }
    if (p.isArray())
    {
        for (uint i = 0; i < p.numElements(); i++)
        {
            if (p[i].numConnectedChildren() == 0)
                continue;
            if (!p[i].isCompound())
                getConnectedChildPlugs(p[i], dest, thisNodePlugs, otherSidePlugs);
            else
            {
                if (getAttributeNameFromPlug(p) == MString("colorEntryList"))
                {
                    getConnectedChildPlugs(p[i].child(1), dest, thisNodePlugs, otherSidePlugs);
                }
            }
        }
    }
}

int physicalIndex(const MPlug& p)
{
    MPlug parent = p;
    while (parent.isChild())
        parent = parent.parent();

    if (!parent.isElement())
        return -1;

    if (!parent.array())
        return - 1;

    MPlug arrayPlug = parent.array();

    for (uint i = 0; i < arrayPlug.numElements(); i++)
    {
        if (arrayPlug[i].logicalIndex() == parent.logicalIndex())
            return i;
    }

    return -1;
}

// gives the plug on the other side of this connected plug, children are ignored
MPlug getDirectConnectedPlug(const MPlug& plug, bool dest)
{
    MPlug thisPlug = plug;
    MPlug returnPlug;
    MPlugArray pa;
    thisPlug.connectedTo(pa, true, false);
    if (pa.length() > 0)
    {
        returnPlug = pa[0];
    }
    return returnPlug;
}

bool isConnected(const char *attrName, MFnDependencyNode& depFn, bool dest, bool primaryChild = false)
{
    MStatus stat;
    MPlugArray pa;
    depFn.getConnections(pa);
    std::vector<std::string> stringParts;
    pystring::split(attrName, stringParts, ".");
    MString attName = attrName;
    if (stringParts.size() > 1)
        attName = stringParts.back().c_str();
    if (pystring::endswith(attrName, "]"))
    {
        int found = attName.rindex('[');
        if (found >= 0)
            attName = attName.substring(0, found-1);
    }

    for (uint pId = 0; pId < pa.length(); pId++)
    {
        if (dest)
        {
            if (!pa[pId].isDestination())
                continue;
        }
        else
        {
            if (!pa[pId].isSource())
                continue;
        }
        MPlug plug = pa[pId];
        if (primaryChild)
            while (plug.isChild())
                plug = plug.parent();
        MString plugName = plug.name();
        if (plug.isElement())
            plug = plug.array();
        MString attNameFromPlug = getAttributeNameFromPlug(plug);
        if ((attNameFromPlug == attName))
            return true;
    }
    return false;
}


MObject getConnectedInNode(MPlug& inPlug)
{
    MObject result = MObject::kNullObj;
    if (!inPlug.isConnected())
        return result;

    MPlugArray connectedPlugs;
    inPlug.connectedTo(connectedPlugs, true, false);
    if (connectedPlugs.length() == 0)
        return result;

    return connectedPlugs[0].node();
}

bool getArrayIndex(MString attrib, MString& index, MString& baseName)
{
    std::string attr = attrib.asChar();
    if (pystring::endswith(attr, "]"))
    {
        int p = attrib.length() - 1;
        while ((attr[p] != '[') && (p >= 0))
            p--;
        if (p < 0)
            return false;
        index = attrib.substring(p + 1, attrib.length() - 2);
        baseName = attrib.substring(0, p - 1);
    }
    return index.length() > 0;
}

MObject getConnectedInNode(const MObject& thisObject, const char *attrName)
{
    MObject result = MObject::kNullObj;
    MString indexStr, base;
    int index = -1;
    if (getArrayIndex(attrName, indexStr, base))
    {
        index = indexStr.asInt();
        attrName = base.asChar();
    }
    MFnDependencyNode depFn(thisObject);
    MPlug inPlug = depFn.findPlug(attrName);
    if (index > -1)
        inPlug = inPlug[index];
    MString plugname = inPlug.name();

    if (!inPlug.isConnected())
        return result;

    MPlugArray connectedPlugs;
    inPlug.connectedTo(connectedPlugs, true, false);
    if (connectedPlugs.length() == 0)
        return result;

    return connectedPlugs[0].node();
}

// get direct connections and primary children connections for such plugs like color, vector, point
void getConnectedInNodes(MPlug& plug, MObjectArray& nodeList)
{
    MPlugArray connectedPlugs;
    plug.connectedTo(connectedPlugs, true, false);
    MString plugname = plug.name();
    int numConnections = connectedPlugs.length();

    for (int i = 0; i <  numConnections; i++)
    {
        MString otherSidePlug = connectedPlugs[i].name();
        MObject plugObject = connectedPlugs[i].node();
        if (plugObject != MObject::kNullObj)
            nodeList.append(plugObject);
    }
}

void getConnectedInNodes(const MString& attribute, const MObject& thisObject, MObjectArray& nodeList)
{
    MFnDependencyNode depFn(thisObject);
    MPlug attrPlug = depFn.findPlug(attribute);
    getConnectedInNodes(attrPlug, nodeList);
}


MObject getOtherSideNode(const MPlug& plug)
{
    MStatus stat;
    MObject result = MObject::kNullObj;

    MPlugArray plugArray;
    plug.connectedTo(plugArray, 1, 0, &stat);if (stat != MStatus::kSuccess) return result;
    if (plugArray.length() == 0)
        return result;
    MPlug otherSidePlug = plugArray[0];
    result = otherSidePlug.node();
    return result;
}

MObject getOtherSideNode(const MString& plugName, MObject& thisObject, MString& otherSidePlugName)
{
    MStatus stat;
    MObject result = MObject::kNullObj;
    MFnDependencyNode depFn(thisObject, &stat); if (stat != MStatus::kSuccess) return result;
    MPlug plug = depFn.findPlug(plugName, &stat);   if (stat != MStatus::kSuccess)return result;
    if (!plug.isConnected())
        return result;
    MPlugArray plugArray;
    plug.connectedTo(plugArray, 1, 0, &stat);if (stat != MStatus::kSuccess) return result;
    if (plugArray.length() == 0)
        return result;
    MPlug otherSidePlug = plugArray[0];
    result = otherSidePlug.node();
    otherSidePlugName = otherSidePlug.name();
    otherSidePlugName = otherSidePlug.partialName(false, false, false, false, false, true);
    return result;
}

bool getConnectedPlugs(const MString& plugName, MObject& thisObject, MPlug& inPlug, MPlug& outPlug)
{
    MStatus stat;
    bool result = false;
    MFnDependencyNode depFn(thisObject, &stat); if (stat != MStatus::kSuccess) return result;
    MPlug plug = depFn.findPlug(plugName, &stat);   if (stat != MStatus::kSuccess) return result;
    MPlugArray plugArray;
    plug.connectedTo(plugArray, 1, 0, &stat);if (stat != MStatus::kSuccess) return result;
    if (plugArray.length() == 0)
        return result;
    MPlug otherSidePlug = plugArray[0];
    inPlug = plug;
    outPlug = otherSidePlug;
    return true;
}

bool getConnectedInPlugs(MObject& thisObject, MPlugArray& inPlugs, MPlugArray& otherSidePlugs)
{
    MStatus stat;
    bool result = false;
    MFnDependencyNode depFn(thisObject, &stat); if (stat != MStatus::kSuccess) return result;
    MPlugArray pa;
    depFn.getConnections(pa);
    for (uint i = 0; i < pa.length(); i++)
        if (pa[i].isDestination())
            inPlugs.append(pa[i]);

    for (uint i = 0; i < inPlugs.length(); i++)
    {
        MPlug p = inPlugs[i];
        p.connectedTo(pa, true, false);
        otherSidePlugs.append(pa[0]);
    }
    return true;
}

bool getConnectedInPlugs(MObject& thisObject, MPlugArray& inPlugs)
{
    MStatus stat;
    bool result = false;
    MFnDependencyNode depFn(thisObject, &stat); if (stat != MStatus::kSuccess) return result;
    MPlugArray pa;
    depFn.getConnections(pa);
    for (uint i = 0; i < pa.length(); i++)
        if (pa[i].isDestination())
            inPlugs.append(pa[i]);

    return true;
}

MString getObjectName(const MObject& mobject)
{
    if (mobject == MObject::kNullObj)
        return "";

    MFnDependencyNode depFn(mobject);
    return depFn.name();
}

MString getDepNodeTypeName(const MObject& mobject)
{
    MFnDependencyNode depFn(mobject);
    return depFn.typeName();
}

MObject objectFromName(MString name)
{
    MObject obj;
    MStatus stat;
    MSelectionList list;

    // Attempt to add the given name to the selection list,
    // then get the corresponding dependency node handle.
    if (!list.add(name) ||
        !list.getDependNode(0, obj))
    {
        // Failed.
        stat = MStatus::kInvalidParameter;
        return obj;
    }

    // Successful.
    stat = MStatus::kSuccess;
    return obj;
}

void posRotSclFromMatrix(MMatrix& matrix, MPoint& pos, MVector& rot, MVector& scl)
{
    MTransformationMatrix tm(matrix);
    MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;
    double rotation[3];
    double scaling[3];
    tm.getRotation(rotation, order, MSpace::kWorld);
    rot.x = rotation[0];
    rot.y = rotation[1];
    rot.z = rotation[2];
    pos = tm.getTranslation(MSpace::kWorld);
    tm.getScale(scaling, MSpace::kWorld);
    scl.x = scaling[0];
    scl.y = scaling[1];
    scl.z = scaling[2];
}

void posRotSclFromMatrix(MMatrix& matrix, MPoint& pos, MPoint& rot, MPoint& scl)
{
    MTransformationMatrix tm(matrix);
    MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;
    double rotation[3];
    double scaling[3];
    tm.getRotation(rotation, order, MSpace::kWorld);
    rot.x = rotation[0];
    rot.y = rotation[1];
    rot.z = rotation[2];
    pos = tm.getTranslation(MSpace::kWorld);
    tm.getScale(scaling, MSpace::kWorld);
    scl.x = scaling[0];
    scl.y = scaling[1];
    scl.z = scaling[2];
}

MString getConnectedFileTexturePath(const MString& plugName, MFnDependencyNode& depFn)
{
    MStatus stat;
    MString path = "";
    MPlug plug = depFn.findPlug(plugName, &stat);
    if (!stat)
        return path;
    if (plug.isConnected())
    {
        MPlugArray parray;
        plug.connectedTo(parray, true, false, &stat);
        if (!stat)
            return path;

        if (parray.length() == 0)
            return path;

        MPlug destPlug = parray[0];
        MObject fileNode = destPlug.node();
        if (!fileNode.hasFn(MFn::kFileTexture))
        {
            return path;
        }

        MFnDependencyNode fileDepFn(fileNode);
        MPlug ftn = fileDepFn.findPlug("fileTextureName", &stat);

        if (!stat)
        {
            return path;
        }
        path = ftn.asString();
    }
    return path;
}

bool getConnectedFileTexturePath(const MString& plugName, MString& nodeName, MString& value, MObject& outFileNode)
{
    MStatus stat;
    MObject obj = objectFromName(nodeName);
    if (obj == MObject::kNullObj)
        return false;

    MFnDependencyNode depFn(obj);
    MPlug plug = depFn.findPlug(plugName, &stat);
    if (!stat)
        return false;

    if (!plug.isConnected())
    {
        return false;
    }
    MPlugArray parray;
    plug.connectedTo(parray, true, false, &stat);
    if (!stat)
        return false;

    if (parray.length() == 0)
        return false;

    MPlug destPlug = parray[0];
    MObject fileNode = destPlug.node();

    if (!fileNode.hasFn(MFn::kFileTexture))
    {
        return false;
    }

    MFnDependencyNode fileDepFn(fileNode);
    MPlug ftn = fileDepFn.findPlug("fileTextureName", &stat);

    if (!stat)
    {
        return false;
    }

    MString fileTextureName = ftn.asString();
    value = fileTextureName;
    outFileNode = fileNode;
    return true;
}

bool findCamera(MDagPath& dagPath)
{
    if (dagPath.node().hasFn(MFn::kCamera))
        return true;
    uint numChilds = dagPath.childCount();
    for (uint chId = 0; chId < numChilds; chId++)
    {
        MDagPath childPath = dagPath;
        MStatus stat = childPath.push(dagPath.child(chId));
        if (!stat)
        {
            continue;
        }
        MString childName = childPath.fullPathName();
        return findCamera(childPath);
    }

    return false;
}

// todo: extend with own light extensions.
bool isLightTransform(MDagPath& dagPath)
{
    uint numChilds = dagPath.childCount();
    for (uint chId = 0; chId < numChilds; chId++)
    {
        MDagPath childPath = dagPath;
        MStatus stat = childPath.push(dagPath.child(chId));
        if (!stat)
        {
            continue;
        }
        if (childPath.node().hasFn(MFn::kLight))
            return true;
    }
    return false;
}

void makeUniqueArray(MObjectArray& oa)
{
    MObjectArray tmpArray;
    for (uint i = 0; i < oa.length(); i++)
    {
        bool found = false;
        for (uint k = 0; k < tmpArray.length(); k++)
        {
            if (oa[i] == tmpArray[k])
            {
                found = true;
                break;
            }
        }
        if (!found)
            tmpArray.append(oa[i]);
    }
    oa = tmpArray;
}

bool isPlugInList(MObject obj, MPlugArray& plugArray)
{
    for (uint oId = 0; oId < plugArray.length(); oId++)
    {
        if (plugArray[oId] == obj)
            return true;
    }
    return false;
}

void findConnectedNodeTypes(uint nodeId, MObject thisObject, MObjectArray& connectedElements, MPlugArray& completeList, bool upstream)
{
    MGlobal::displayInfo(MString("thisNode: ") + getObjectName(thisObject));

    MString name = getObjectName(thisObject);

    MFnDependencyNode depFn(thisObject);
    if (depFn.typeId().id() == nodeId)
    {
        connectedElements.append(thisObject);
        MGlobal::displayInfo(MString("found object with correct id: ") + depFn.name());
        return;
    }

    bool downstream = !upstream;

    MPlugArray plugArray;
    depFn.getConnections(plugArray);

    int numc = plugArray.length();

    for (uint plugId = 0; plugId < plugArray.length(); plugId++)
    {
        MPlug plug = plugArray[plugId];
        if (isPlugInList(plug, completeList))
            continue;

        completeList.append(plug);

        MString pn = plug.name();
        if (upstream && plug.isDestination())
            continue;
        if (downstream && plug.isSource())
            continue;

        MPlugArray otherSidePlugs;
        bool asDest = plug.isDestination();
        bool asSrc = plug.isSource();
        MGlobal::displayInfo(MString("findConnectedNodeTypes: checking plug ") + plug.name());
        plug.connectedTo(otherSidePlugs, asDest, asSrc);
        for (uint cplugId = 0; cplugId < otherSidePlugs.length(); cplugId++)
        {
            findConnectedNodeTypes(nodeId, otherSidePlugs[cplugId].node(), connectedElements, completeList, upstream);
        }
    }
}

void findConnectedNodeTypes(uint nodeId, MObject thisObject, MObjectArray& connecedElements, bool upstream)
{
    MPlugArray completeList;
    findConnectedNodeTypes(nodeId, thisObject, connecedElements, completeList, upstream);
}

MString getAttributeNameFromPlug(const MPlug& plug)
{
    MFnAttribute att(plug.attribute());
    return att.name();
}

void uniqueMObjectArray(MObjectArray& cleanMe)
{
    MObjectArray tmpArray;
    for (uint i = 0; i < cleanMe.length(); i++)
    {
        bool found = false;
        for (uint k = 0; k < tmpArray.length(); k++)
        {
            if (cleanMe[i] == tmpArray[k])
            {
                found = true;
                break;
            }
        }
        if (!found)
            tmpArray.append(cleanMe[i]);
    }
    cleanMe = tmpArray;
}

bool isSunLight(MObject& obj)
{
    MObjectArray nodeList;
    MStatus stat;
    MObject sun = obj;

    if (obj.hasFn(MFn::kDirectionalLight))
    {
        MFnDagNode sunDagNode(obj);
        sun = sunDagNode.parent(0);
    }

    getConnectedInNodes(MString("physicalSunConnection"), getRenderGlobalsNode(), nodeList);
    if (nodeList.length() > 0)
    {
        MObject sunObj = nodeList[0];
        if (sunObj.hasFn(MFn::kTransform))
        {
            if (sunObj == sun)
                return true;
        }
    }
    return false;
}
