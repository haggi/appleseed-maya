
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

#ifndef MATATOCOMMON_UTILITIES_TOOLS
#define MATATOCOMMON_UTILITIES_TOOLS

#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MObjectArray.h>

#include <assert.h>
#include <fstream>
#include <math.h>
#include <memory>
#include <vector>

static MString RendererHome;

void setRendererHome(MString home);

MString getRendererHome();

MObject getRenderGlobalsNode();

MString makeGoodString(const MString& oldString);

bool IsVisible(MFnDagNode& node);

bool IsVisible(MDagPath& node);

bool IsTemplated(MFnDagNode& node);

bool IsPathTemplated(MDagPath& node);

bool IsInRenderLayer(MDagPath& dagPath);

bool IsLayerVisible(MDagPath& dagPath);

bool IsPathVisible(MDagPath& dagPath);

MObject getOtherSideNode(const MString& plugName, MObject& thisObject);

MObject getOtherSideNode(const MString& plugName, MObject& thisObject, MString& otherSidePlugName);

MObject getOtherSideNode(const MString& plugName, MObject& thisObject, MStringArray& otherSidePlugNames);

MObject getOtherSideNode(const MPlug& plug);

MPlug getDirectConnectedPlug(const char *attrName, MFnDependencyNode& depFn, bool dest);

MPlug getDirectConnectedPlug(const MPlug& plug, bool dest);

void getConnectedChildPlugs(const MPlug& plug, bool dest, MPlugArray& thisNodePlugs, MPlugArray& otherSidePlugs);

void getConnectedChildPlugs(const char *attrName, MFnDependencyNode& depFn, bool dest, MPlugArray& thisNodePlugs, MPlugArray& otherSidePlugs);

bool isConnected(const char *attrName, MFnDependencyNode& depFn, bool dest, bool primaryChild);

bool isConnected(const char *attrName, MObject& node, bool dest);

int physicalIndex(const MPlug& p);

bool getConnectedPlugs(const MString& plugName, MObject& thisObject, MPlug& inPlug, MPlug& outPlug);

bool getConnectedInPlugs(MObject& thisObject, MPlugArray& inPlugs);

bool getConnectedInPlugs(MObject& thisObject, MPlugArray& inPlugs, MPlugArray& otherSidePlugs);

MObject getConnectedInNode(const MObject& thisObject, const char *attrName);

MObject getConnectedInNode(MPlug& inPlug);

void getConnectedInNodes(MPlug& plug, MObjectArray& nodeList);

void getConnectedInNodes(const MString& attribute, const MObject& thisObject, MObjectArray& nodeList);

MString getObjectName(const MObject& mobject);

MString getDepNodeTypeName(const MObject& mobject);

MString lightColorAsString(MFnDependencyNode& depFn);

MObject objectFromName(MString name);

void posRotSclFromMatrix(MMatrix& matrix, MPoint& pos, MVector& rot, MVector& scl);

void posRotSclFromMatrix(MMatrix& matrix, MPoint& pos, MPoint& rot, MPoint& scl);

bool getConnectedFileTexturePath(const MString& plugName, MString& nodeName, MString& value, MObject& outFileNode);

MString getConnectedFileTexturePath(const MString& plugName, MFnDependencyNode& node);

bool findCamera(MDagPath& dagPath);

bool isLightTransform(MDagPath& dagPath);

bool isCameraRenderable(MObject obj);

void makeUniqueArray(MObjectArray&);

float rnd();

bool isPlugInList(MObject obj, MPlugArray& plugArray);

void findConnectedNodeTypes(uint nodeId, MObject thisObject, MObjectArray& connecedElements, bool upstream);
void findConnectedNodeTypes(uint nodeId, MObject thisObject, MObjectArray& connecedElements, MPlugArray& completeList, bool upstream);

MString getAttributeNameFromPlug(const MPlug& plug);

void uniqueMObjectArray(MObjectArray& cleanMe);

bool isSunLight(MObject& obj);

template <typename T>
MString to_mstring(const T& val)
{
    MString result;
    result += val;
    return result;
}

template <typename T1>
MString format(const MString& fmt, const T1& arg1)
{
    MString result;
#ifndef NDEBUG
    const MStatus status =
#endif
        result.format(fmt, to_mstring(arg1));
    assert(status == MS::kSuccess);
    return result;
}

template <typename T1>
MString format(const MString& fmt, const T1& arg1, const T1& arg2)
{
    MString result;
#ifndef NDEBUG
    const MStatus status =
#endif
        result.format(fmt, to_mstring(arg1), to_mstring(arg2));
    assert(status == MS::kSuccess);
    return result;
}

template <typename T1>
MString format(const MString& fmt, const T1& arg1, const T1& arg2, const T1& arg3)
{
    MString result;
#ifndef NDEBUG
    const MStatus status =
#endif
        result.format(fmt, to_mstring(arg1), to_mstring(arg2), to_mstring(arg3));
    assert(status == MS::kSuccess);
    return result;
}

template <typename T1>
MString format(const MString& fmt, const T1& arg1, const T1& arg2, const T1& arg3, const T1& arg4)
{
    MString result;
#ifndef NDEBUG
    const MStatus status =
#endif
        result.format(fmt, to_mstring(arg1), to_mstring(arg2), to_mstring(arg3), to_mstring(arg4));
    assert(status == MS::kSuccess);
    return result;
}


#endif  // !MATATOCOMMON_UTILITIES_TOOLS
