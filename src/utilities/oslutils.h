
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

#ifndef UTILITIES_OSLUTILS_H
#define UTILITIES_OSLUTILS_H

/*
    OSL and maya projection nodes have a problem.
    A projection node normally works this way:
        - Calculate the projection of a point
        - Calculate the new uv's
        - Get the image based on these new uv's
    In OSL this is not possible. Modifying the uv's and evaluate
    an image based on these new uv's is not possible, the uv's have to
    be manipulated before they are read by the projection node what is a cycle,
    because the projection node itself calculates the uvs....
    My solution is to split the projection node into the color part and the projection part.
    The projection part has to be connected to the very first node in the chain, normally a place2dnode.
    So what is do is:
        - from an attribute/plug I search the node history for a projection node
        - if one is found, follow the chain to the leaf nodes and place the projection part before the leaf
    So I create a structure which saves the projection nodes for every leaf node
*/

#include "../shadingtools/shadingnode.h"
#include "tools.h"
#include "OSL/oslexec.h"

#include <maya/MColor.h>
#include <maya/MFloatVector.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MString.h>
#include <maya/MVector.h>

#include <cmath>
#include <vector>

#include "boost/variant.hpp"

#define ARRAY_MAX_ENTRIES 10

class OSLShadingNetworkRenderer;

static std::vector<MString> DefinedOSLNodes;
static std::vector<MString> DefinedOSLSWNodes;

class Connection
{
  public:
    Connection();
    Connection(const MString& sn, const MString& sa, const MString& dn, const MString& da);
    bool operator==(const Connection& otherOne);
    MString sourceNode;
    MString sourceAttribute;
    MString destNode;
    MString destAttribute;
};

struct SimpleVector
{
    float f[3];
};

struct SimpleMatrix
{
    float f[4][4];
};

class OSLParameter
{
  public:
    MString name;
    OIIO::TypeDesc type;
    MVector mvector;
    boost::variant<int, float, SimpleVector, SimpleMatrix, std::string> value;

    OSLParameter(const MString& pname, float pvalue);
    OSLParameter(const MString& pname, int pvalue);
    OSLParameter(const MString& pname, const MString& pvalue);
    OSLParameter(const MString& pname, const MVector& pvalue);
    OSLParameter(const MString& pname, const MMatrix& pvalue);
    OSLParameter(const MString& pname, const MColor& pvalue);
    OSLParameter(const MString& pname, bool pvalue);
    OSLParameter(const char* pname, float pvalue);
    OSLParameter(const char* pname, int pvalue);
    OSLParameter(const char* pname, const MString& pvalue);
    OSLParameter(const char* pname, const std::string& pvalue);
    OSLParameter(const char* pname, const MVector& pvalue);
    OSLParameter(const char* pname, const MMatrix& pvalue);
    OSLParameter(const char* pname, const MColor& pvalue);
    OSLParameter(const char* pname, bool pvalue);
};

struct ProjectionUtil
{
    MObjectArray leafNodes;
    MObject projectionNode;
};

typedef std::vector<ProjectionUtil> ProjectionNodeArray;
typedef std::vector<OSLParameter> OSLParamArray;
typedef std::vector<Connection> ConnectionArray;

struct OSLNodeStruct
{
    MString typeName;
    MString nodeName;
    OSLParamArray paramArray;
};

enum ConnectionType
{
    SCALAR_TO_SCALAR = 0, // outAlpha -> maskMultiplier
    VECTOR_TO_VECTOR,     // outColor -> diffuseColor
    SCALAR_TO_COMP,       // outAlpha -> diffuseColorR
    COMP_TO_SCALAR,       // outColorR -> maskMultiplier
    COMP_TO_COMP          // outColorR -> diffuseColorG
};

class OSLUtilClass
{
  public:
    OSLUtilClass();

    OSLShadingNetworkRenderer* oslRenderer;
    OSL::ShaderGroup* group;
    ProjectionNodeArray projectionNodeArray;
    std::vector<MObject> projectionNodes;
    std::vector<MObject> projectionConnectNodes;

    std::vector<MString> definedOSLNodes;
    std::vector<MString> definedOSLSWNodes;
    ConnectionArray connectionList;
    std::vector<OSLNodeStruct> oslNodeArray;

    bool doesHelperNodeExist(MString& helperNode);
    void listProjectionHistory(const MObject& mobject);
    void defineOSLParameter(ShaderAttribute& sa, MFnDependencyNode& depFn, OSLParamArray& paramArray);
    void createOSLShadingNode(ShadingNode& snode);
    void connectProjectionNodes(MObject& projNode);
    void fillVectorParam(OSLParamArray& params, MPlug vectorPlug);
    bool doesOSLNodeAlreadyExist(MString& oslNode);
    bool doesOSLNodeAlreadyExist(MObject& oslNode);
    void saveOSLNodeNameInArray(MString& oslNodeName);
    void addConnectionToConnectionArray(ConnectionArray& ca, MString sourceNode, MString sourceAtt, MString destNode, MString destAttr);
    void createOSLProjectionNodes(MPlug& plug);
    void createOSLProjectionNodes(const MObject& surfaceShaderNode);
    void createAndConnectShaderNodes();
    void initOSLUtil();

    void createOSLShader(MString& shaderNodeType, MString& shaderName, OSLParamArray& paramArray); //overwrite this in renderer specific version
    void connectOSLShaders(ConnectionArray& ca); //overwrite this in renderer specific version
    bool handleSpecialPlugs(MString attributeName, MFnDependencyNode& depFn, MPlugArray& sourcePlugs, MPlugArray& destPlugs);
    bool getConnectedPlugs(MString attributeName, MFnDependencyNode& depFn, MPlugArray& sourcePlugs, MPlugArray& destPlugs);
    bool getConnectedPlugs(MPlug plug, MFnDependencyNode& depFn, MPlugArray& sourcePlugs, MPlugArray& destPlugs);
    void checkPlugsValidity(MPlugArray& sourcePlugs, MPlugArray& destPlugs);
    void getConnectionType(MPlug sourcePlug, MPlug destPlug, ConnectionType& type);
    void createHelperNode(MPlug sourcePlug, MPlug destPlug, ConnectionType type, std::vector<OSLNodeStruct>& oslNodes, ConnectionArray& connectionArray);
    MString getCorrectOSLParameterName(MPlug plug);
    MString getCleanParamName(MPlug plug);
    void addNodeToList(OSLNodeStruct node);
    void addConnectionToList(Connection c);
    void cleanupShadingNodeList();
};

#endif  // !UTILITIES_OSLUTILS_H
