
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

#ifndef MT_OSL_UTILS_H
#define MT_OSL_UTILS_H

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
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MString.h>
#include <maya/MVector.h>
#include <maya/MColor.h>
#include <maya/MMatrix.h>
#include <maya/MFloatVector.h>
#include <maya/MFnDependencyNode.h>
#include <vector>
#include "OSL/oslexec.h"
#include "boost/variant.hpp"
#include "math.h"

#include "../shadingtools/shadingnode.h"
#include "../utilities/tools.h"

#define ARRAY_MAX_ENTRIES 10

namespace OSL
{
    class OSLShadingNetworkRenderer;
}

namespace MAYATO_OSL
{
    static std::vector<MString> DefinedOSLNodes;
    static std::vector<MString> DefinedOSLSWNodes;

    struct Connection
    {
        MString sourceNode;
        MString sourceAttribute;
        MString destNode;
        MString destAttribute;

        static MString validateParameter(MString name)
        {
            if (name == "min")
                return "inMin";
            if (name == "max")
                return "inMax";
            if (name == "vector")
                return "inVector";
            if (name == "matrix")
                return "inMatrix";
            if (name == "color")
                return "inColor";
            return name;
        }

        Connection()
        {
        }

        Connection(MString sn, MString sa, MString dn, MString da)
        {
            sourceNode = validateParameter(sn);
            sourceAttribute = validateParameter(sa);
            destNode = validateParameter(dn);
            destAttribute = validateParameter(da);
        }

        bool operator==(const Connection& otherOne)
        {
            if (sourceNode == otherOne.sourceNode)
                if (destNode == otherOne.destNode)
                    if (sourceAttribute == otherOne.sourceAttribute)
                        if (destAttribute == otherOne.destAttribute)
                            return true;
            return false;
        }
    };

    struct SimpleVector
    {
        float f[3];
    };

    struct SimpleMatrix
    {
        float f[4][4];
    };

    struct OSLParameter
    {
        MString name;
        OIIO::TypeDesc type;
        MVector mvector;
        boost::variant<int, float, SimpleVector, SimpleMatrix, std::string> value;

        static MString validateParameter(const MString& pname)
        {
            if (pname == "min")
                return "inMin";
            if (pname == "max")
                return "inMax";
            if (pname == "vector")
                return "inVector";
            if (pname == "matrix")
                return "inMatrix";
            if (pname == "color")
                return "inColor";
            return pname;
        }

        OSLParameter(const MString& pname, float pvalue)
        {
            name = pname;
            value = pvalue;
            type = OSL::TypeDesc::TypeFloat;
        }

        OSLParameter(const MString& pname, int pvalue)
        {
            name = pname;
            value = pvalue;
            type = OSL::TypeDesc::TypeInt;
        }

        OSLParameter(const MString& pname, const MString& pvalue)
        {
            name = pname;
            value = pvalue.asChar();
            type = OSL::TypeDesc::TypeString;
        }

        OSLParameter(const MString& pname, const MVector& pvalue)
        {
            name = validateParameter(pname);
            SimpleVector s;
            s.f[0] = pvalue.x;
            s.f[1] = pvalue.y;
            s.f[2] = pvalue.z;
            value = s;
            type = OSL::TypeDesc::TypeVector;
        }

        OSLParameter(const MString& pname, const MMatrix& pvalue)
        {
            name = validateParameter(pname);
            SimpleMatrix m;
            pvalue.get(m.f);
            value = m;
            type = OSL::TypeDesc::TypeMatrix;
        }

        OSLParameter(const MString& pname, const MColor& pvalue)
        {
            name = validateParameter(pname);
            SimpleVector s;
            s.f[0] = pvalue.r;
            s.f[1] = pvalue.g;
            s.f[2] = pvalue.b;
            value = s;
            type = OSL::TypeDesc::TypeVector;
        }

        OSLParameter(const MString& pname, bool pvalue)
        {
            name = pname;
            value = (int)pvalue;
            type = OSL::TypeDesc::TypeInt;
        }

        OSLParameter(const char *pname, float pvalue)
        {
            name = pname;
            value = pvalue;
            type = OSL::TypeDesc::TypeFloat;
        }

        OSLParameter(const char *pname, int pvalue)
        {
            name = pname;
            value = pvalue;
            type = OSL::TypeDesc::TypeInt;
        }

        OSLParameter(const char *pname, const MString& pvalue)
        {
            name = pname;
            value = pvalue.asChar();
            type = OSL::TypeDesc::TypeString;
        }

        OSLParameter(const char *pname, const std::string& pvalue)
        {
            name = pname;
            value = pvalue.c_str();
            type = OSL::TypeDesc::TypeString;
        }

        OSLParameter(const char *pname, const MVector& pvalue)
        {
            name = validateParameter(pname);
            SimpleVector s;
            s.f[0] = pvalue.x;
            s.f[1] = pvalue.y;
            s.f[2] = pvalue.z;
            value = s;
            type = OSL::TypeDesc::TypeVector;
            mvector = pvalue;
        }

        OSLParameter(const char *pname, const MMatrix& pvalue)
        {
            name = validateParameter(pname);
            SimpleMatrix m;
            pvalue.get(m.f);
            value = m;
            type = OSL::TypeDesc::TypeMatrix;
        }

        OSLParameter(const char *pname, const MColor& pvalue)
        {
            name = validateParameter(pname);
            SimpleVector s;
            s.f[0] = pvalue.r;
            s.f[1] = pvalue.g;
            s.f[2] = pvalue.b;
            value = s;
            type = OSL::TypeDesc::TypeVector;
        }

        OSLParameter(const char *pname, bool pvalue)
        {
            name = pname;
            value = (int)pvalue;
            type = OSL::TypeDesc::TypeInt;
        }
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
}

namespace MAYATO_OSLUTIL{

    enum ConnectionType{
        SCALAR_TO_SCALAR = 0, // outAlpha -> maskMultiplier
        VECTOR_TO_VECTOR,     // outColor -> diffuseColor
        SCALAR_TO_COMP,       // outAlpha -> diffuseColorR
        COMP_TO_SCALAR,       // outColorR -> maskMultiplier
        COMP_TO_COMP          // outColorR -> diffuseColorG
    };
    class OSLUtilClass{
    public:
        OSLUtilClass();
        ~OSLUtilClass();

        OSL::OSLShadingNetworkRenderer *oslRenderer;
        OSL::ShaderGroup *group = nullptr;
        MAYATO_OSL::ProjectionNodeArray projectionNodeArray;
        std::vector<MObject> projectionNodes;
        std::vector<MObject> projectionConnectNodes;

        std::vector<MString> definedOSLNodes;
        std::vector<MString> definedOSLSWNodes;
        MAYATO_OSL::ConnectionArray connectionList;
        std::vector<MAYATO_OSL::OSLNodeStruct> oslNodeArray;


        bool doesHelperNodeExist(MString& helperNode);
        void listProjectionHistory(const MObject& mobject);
        void defineOSLParameter(ShaderAttribute& sa, MFnDependencyNode& depFn, MAYATO_OSL::OSLParamArray& paramArray);
        void createOSLShadingNode(ShadingNode& snode);
        void connectProjectionNodes(MObject& projNode);
        void fillVectorParam(MAYATO_OSL::OSLParamArray& params, MPlug vectorPlug);
        bool doesOSLNodeAlreadyExist(MString& oslNode);
        bool doesOSLNodeAlreadyExist(MObject& oslNode);
        void saveOSLNodeNameInArray(MString& oslNodeName);
        void addConnectionToConnectionArray(MAYATO_OSL::ConnectionArray& ca, MString sourceNode, MString sourceAtt, MString destNode, MString destAttr);
        void createOSLProjectionNodes(MPlug& plug);
        void createOSLProjectionNodes(const MObject& surfaceShaderNode);
        void createAndConnectShaderNodes();
        void initOSLUtil();

        void createOSLShader(MString& shaderNodeType, MString& shaderName, MAYATO_OSL::OSLParamArray& paramArray); //overwrite this in renderer specific version
        void connectOSLShaders(MAYATO_OSL::ConnectionArray& ca); //overwrite this in renderer specific version
        bool handleSpecialPlugs(MString attributeName, MFnDependencyNode& depFn, MPlugArray& sourcePlugs, MPlugArray& destPlugs);
        bool getConnectedPlugs(MString attributeName, MFnDependencyNode& depFn, MPlugArray& sourcePlugs, MPlugArray& destPlugs);
        bool getConnectedPlugs(MPlug plug, MFnDependencyNode& depFn, MPlugArray& sourcePlugs, MPlugArray& destPlugs);
        void checkPlugsValidity(MPlugArray& sourcePlugs, MPlugArray& destPlugs);
        void getConnectionType(MPlug sourcePlug, MPlug destPlug, ConnectionType& type);
        void createHelperNode(MPlug sourcePlug, MPlug destPlug, ConnectionType type, std::vector<MAYATO_OSL::OSLNodeStruct>& oslNodes, MAYATO_OSL::ConnectionArray& connectionArray);
        MString getCorrectOSLParameterName(MPlug plug);
        MString getCleanParamName(MPlug plug);
        void addNodeToList(MAYATO_OSL::OSLNodeStruct node);
        void addConnectionToList(MAYATO_OSL::Connection c);
        void cleanupShadingNodeList();
    };

}
#endif
