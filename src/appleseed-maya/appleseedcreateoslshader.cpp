
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

#include "osl/oslutils.h"
#include "utilities/logging.h"
#include "world.h"
#include "appleseed.h"
#include "renderer/modeling/shadergroup/shadergroup.h"
#include "OpenImageIO/typedesc.h"
#include "OSL/oslexec.h"

MString oslTypeToMString(MAYATO_OSL::OSLParameter param)
{
    MString result;
    void *val = 0;
    if (param.type == OSL::TypeDesc::TypeFloat)
    {
        result = "float ";
        result += boost::get<float>(param.value);
    }
    if (param.type == OSL::TypeDesc::TypeInt)
    {
        result = "int ";
        result += boost::get<int>(param.value);
    }
    if (param.type == OSL::TypeDesc::TypeVector)
    {
        MAYATO_OSL::SimpleVector &v = boost::get<MAYATO_OSL::SimpleVector>(param.value);
        result = MString("vector ") + v.f[0] + " " + v.f[1] + " " + v.f[2];
    }
    if (param.type == OSL::TypeDesc::TypeColor)
    {
        MAYATO_OSL::SimpleVector &v = boost::get<MAYATO_OSL::SimpleVector>(param.value);
        result = MString("color ") + v.f[0] + " " + v.f[1] + " " + v.f[2];
    }
    if (param.type == OSL::TypeDesc::TypeString)
    {
        result = MString("string ") + boost::get<std::string>(param.value).c_str();
        if (MString("") == boost::get<std::string>(param.value).c_str())
        {
            result = MString("string black.exr");
        }
    }
    if (param.type == OSL::TypeDesc::TypeMatrix)
    {
        MAYATO_OSL::SimpleMatrix &v = boost::get<MAYATO_OSL::SimpleMatrix>(param.value);
        result = MString("matrix ") + v.f[0][0] + " " + v.f[0][1] + " " + v.f[0][2] + " " + v.f[0][3] +
            v.f[1][0] + " " + v.f[1][1] + " " + v.f[1][2] + " " + v.f[1][3] +
            v.f[2][0] + " " + v.f[2][1] + " " + v.f[2][2] + " " + v.f[2][3] +
            v.f[3][0] + " " + v.f[3][1] + " " + v.f[3][2] + " " + v.f[3][3];
    }
    return result;
}


void MAYATO_OSLUTIL::OSLUtilClass::connectOSLShaders(MAYATO_OSL::ConnectionArray& ca)
{
    std::vector<MAYATO_OSL::Connection>::iterator cIt;
    for (cIt = ca.begin(); cIt != ca.end(); cIt++)
    {
        const char *srcLayer = cIt->sourceNode.asChar();
        const char *srcAttr = cIt->sourceAttribute.asChar();
        const char *destLayer = cIt->destNode.asChar();
        MString destAttr = cIt->destAttribute;
        if (destAttr == "color")
            destAttr = "inColor";
        Logging::debug(MString("MAYATO_OSL::connectOSLShaders ") + srcLayer + "." + srcAttr + " -> " + destLayer + "." + destAttr);
        OSL::ShaderGroup *g = group;
        asr::ShaderGroup *ag = (asr::ShaderGroup *)g;
        ag->add_connection(srcLayer, srcAttr, destLayer, destAttr.asChar());
    }
}

void MAYATO_OSLUTIL::OSLUtilClass::createOSLShader(MString& shaderNodeType, MString& shaderName, MAYATO_OSL::OSLParamArray& paramArray)
{
    Logging::debug(MString("MAYATO_OSL::createOSLShader ") + shaderName);
    asr::ParamArray asParamArray;
    std::vector<MAYATO_OSL::OSLParameter>::iterator pIt;
    for (pIt = paramArray.begin(); pIt != paramArray.end(); pIt++)
    {
        MString pname = pIt->name;
        if (pname == "color")
            pname = "inColor";

        MString paramString = oslTypeToMString(*pIt);
        asParamArray.insert(pname.asChar(), paramString);
        Logging::debug(MString("\tParam ") + pIt->name + " " + paramString);
    }

    Logging::debug(MString("MAYATO_OSL::createOSLShader creating shader node "));
    OSL::ShaderGroup *g = group;
    asr::ShaderGroup *ag = (asr::ShaderGroup *)g;
    ag->add_shader("shader", shaderNodeType.asChar(), shaderName.asChar(), asParamArray);
}
