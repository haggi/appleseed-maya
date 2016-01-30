
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

#ifndef MTM_TOOLS
#define MTM_TOOLS

#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MObjectArray.h>
#include <fstream>
#include <math.h>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#endif

static MString RendererName;
static MString RendererShortCut;
static MString RendererHome;

#define DegToRad(x) (M_PI/ 180.0 * x)
#define RadToDeg(x) (180.0/M_PI * x)

float clamp(float x, float minv, float maxv);

float smoothstep(float edge0, float edge1, float x);

void setRendererName(MString rname);

void setRendererShortCutName(MString rname);

void setRendererHome(MString home);

MString getRendererName();

MString getRendererShortCutName();

MString getRendererHome();

MObject getRenderGlobalsNode();

MDagPath getDagPathFromName(MString name);

bool checkDirectory( MString& path);

MString makeGoodString(const MString& oldString);

MString makeGoodHierarchyString(MString& oldString);

MString makeHierarchyString(MString& oldString);

MString getPlugName(MString& longPlugname);

bool IsVisible(MFnDagNode& node);

bool IsVisible(MDagPath& node);

bool IsTemplated(MFnDagNode& node);

bool IsPathTemplated(MDagPath& node);

bool IsInRenderLayer(MDagPath& dagPath);

bool IsLayerVisible(MDagPath& dagPath);

bool IsPathVisible( MDagPath& dagPath );

bool CheckVisibility( MDagPath& dagPath );

MString matrixToString(MMatrix& matrix);

MObject getOtherSideNode(const MString& plugName, MObject& thisObject);

MObject getOtherSideSourceNode(const MString& plugName, MObject& thisObject, bool checkChildren, MString& outPlugName);

MObject getOtherSideNode(const MString& plugName, MObject& thisObject, MString& otherSidePlugName);

MObject getOtherSideNode(const MString& plugName, MObject& thisObject, MStringArray& otherSidePlugNames);

MObject getOtherSideNode(const MPlug& plug);

MPlug getDirectConnectedPlug(const char *attrName, MFnDependencyNode& depFn, bool dest);

MPlug getDirectConnectedPlug(const MPlug& plug, bool dest);

void getConnectedChildPlugs(const MPlug& plug, bool dest, MPlugArray& thisNodePlugs, MPlugArray& otherSidePlugs);

void getConnectedChildPlugs(const char *attrName, MFnDependencyNode& depFn, bool dest, MPlugArray& thisNodePlugs, MPlugArray& otherSidePlugs);

void getDirectConnectedPlugs(const char *attrName, MFnDependencyNode& depFn, bool dest, MPlugArray& thisNodePlugs, MPlugArray& otherSidePlugs);

void getConnectedChildrenPlugs(const char *attrName, MFnDependencyNode& depFn, bool dest, MPlugArray& connectedChildren);

bool isConnected(const char *attrName, MFnDependencyNode& depFn, bool dest, bool primaryChild);

bool isConnected(const char *attrName, MObject& node, bool dest);

int physicalIndex(MPlug& p);

bool getOtherSidePlugName(const MString& plugName, MObject& thisObject, MString& otherSidePlugName);

bool getConnectedPlugs(const MString& plugName, MObject& thisObject, MPlug& inPlug, MPlug& outPlug);

bool getConnectedInPlugs(MObject& thisObject, MPlugArray& inPlugs);

bool getConnectedInPlugs(MObject& thisObject, MPlugArray& inPlugs, MPlugArray& otherSidePlugs);

bool getConnectedOutPlugs(MObject& thisObject, MPlugArray& outPlugs);

void getConnectedNodes(MObject& thisObject, MObjectArray& nodeList);

MObject getConnectedInNode(const MObject& thisObject, const char *attrName);

MObject getConnectedInNode(MPlug& inPlug);

void getConnectedInNodes(MPlug& plug, MObjectArray& nodeList);

void getConnectedInNodes(const MString& attribute, const MObject& thisObject, MObjectArray& nodeList);

bool hasPlug(MObject& thisObject, const MString& plugName);

MString getObjectName(MObject& mobject);

MString getObjectName(const MObject& mobject);

MString getDepNodeTypeName(MObject mobject);

MString pointToUnderscore(MString& string);

void writeTMatrixList( std::ofstream& outFile, std::vector<MMatrix>& transformMatrices, bool inverse = false, float scaleFactor = 1.0);

void writeTMatrixList( std::ofstream *outFile, std::vector<MMatrix>& transformMatrices, bool inverse = false, float scaleFactor = 1.0);

MString lightColorAsString(MFnDependencyNode& depFn);

float shadowColorIntensity(MFnDependencyNode& depFn);

void rowToColumn(MMatrix& from, MMatrix& to, bool transRev = false);

MObject objectFromName(MString name);

void posFromMatrix(MMatrix& matrix, MVector& pos);

void posRotFromMatrix(MMatrix& matrix, MPoint& pos, MVector& rot);

void posRotSclFromMatrix(MMatrix& matrix, MPoint& pos, MVector& rot, MVector& scl);

void posRotSclFromMatrix(MMatrix& matrix, MPoint& pos, MPoint& rot, MPoint& scl);

bool getConnectedFileTexturePath(const MString& plugName, MString& nodeName, MString& value, MObject& outFileNode);

MString getConnectedFileTexturePath(const MString& plugName, MFnDependencyNode& node);

MObject getConnectedFileTextureObject(const MString& plugName, MFnDependencyNode& depFn);

bool findCamera(MDagPath& dagPath);

bool isLightTransform(MDagPath& dagPath);

bool isCameraTransform(MDagPath& dagPath);

bool isCamera(MObject obj);

MObject getTransformNode(MDagPath& dagPath);

MObject getConnectedObjSet(MDagPath& dagPath);

bool isCameraRenderable(MObject obj);

void makeUniqueArray( MObjectArray& );

float rnd();

float srnd();

bool isObjectInList(MObject obj, MObjectArray& objectArray);
bool isPlugInList(MObject obj, MPlugArray& plugArray);

void findConnectedNodeTypes(uint nodeId, MObject thisObject, MObjectArray& connecedElements, bool upstream);
void findConnectedNodeTypes(uint nodeId, MObject thisObject, MObjectArray& connecedElements, MPlugArray& completeList, bool upstream);

MString getAttributeNameFromPlug(const MPlug& plug);

MObject getConnectedShadingEngine(MObject node);

void getMatrixComponents(MMatrix& matrix, MPoint& pos, MPoint& rot, MPoint& scale);

void getUVFromConnectedTexturePlacementNode(MObject fileTextureNode, float inU, float inV, float& outU, float& outV);

void uniqueMObjectArray(MObjectArray& cleanMe);

MPlug getParentPlug(MPlug& p);

bool isChildOf(MPlug& parent, MPlug& child);

bool isSunLight(MObject& obj);
#endif
