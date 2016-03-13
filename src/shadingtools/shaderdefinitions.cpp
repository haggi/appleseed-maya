
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

#include "shadingnode.h"
#include "shaderdefinitions.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "utilities/pystring.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/optional/optional.hpp>

// Standard headers.
#include <fstream>
#include <string>

using boost::property_tree::ptree;

typedef std::vector<ShadingNode> ShadingNodeVector;

ShadingNodeVector ShaderDefinitions::shadingNodes;
bool ShaderDefinitions::readDone;

ShaderDefinitions::ShaderDefinitions()
{
    ShaderDefinitions::readDone = false;
}

ShaderDefinitions::~ShaderDefinitions()
{
}

void ShaderDefinitions::readShaderDefinitions()
{
    ptree pt;
    std::string shaderDefFile = (getRendererHome() + "resources/shaderDefinitions.xml").asChar();
    std::ifstream shaderFile(shaderDefFile.c_str());
    if (!shaderFile.good())
    {
        Logging::error("Unable to read xml shader defs.");
        shaderFile.close();
        return;
    }

    read_xml(shaderDefFile, pt);

    BOOST_FOREACH(ptree::value_type v, pt.get_child("shaders"))
    {
        Logging::debug(MString("\tShader: ") + v.second.get_child("name").data().c_str());
        ShadingNode snode;
        snode.fullName = v.second.get_child("name").data().c_str();
        snode.typeName = snode.fullName;
        BOOST_FOREACH(ptree::value_type input, v.second.get_child("inputs"))
        {
            Logging::debug(MString("\tInput: ") + input.second.get_child("name").data().c_str() + " Type: " + input.second.get_child("type").data().c_str());
            ShaderAttribute att;
            att.name = input.second.get_child("name").data();
            att.type = input.second.get_child("type").data();
            const size_t hint = input.second.count("hint");
            if (hint > 0)
            {
                att.hint = input.second.get_child("hint").data();
                att.hint = pystring::replace(att.hint, "\"", "");
                att.hint = pystring::replace(att.hint, " ", "");
            }
            const size_t compAttr = input.second.count("compAttrArrayPath");
            if (compAttr > 0)
            {
                att.compAttrArrayPath = input.second.get_child("compAttrArrayPath").data();
            }
            const size_t arrayPlug = input.second.count("isArrayPlug");
            if (arrayPlug > 0)
            {
                att.isArrayPlug = true;
            }
            const size_t options = input.second.count("options");
            if (options > 0)
            {
                att.optionMenu = true;
            }
            snode.addInputAttribute(att);
        }
        BOOST_FOREACH(ptree::value_type input, v.second.get_child("outputs"))
        {
            Logging::debug(MString("\tOutput: ") + input.second.get_child("name").data().c_str() + " Type: " + input.second.get_child("type").data().c_str());
            ShaderAttribute att;
            att.name = input.second.get_child("name").data();
            att.type = input.second.get_child("type").data();
            snode.addOutputAttribute(att);
        }
        ShaderDefinitions::shadingNodes.push_back(snode);
    }

    readDone = true;
}

bool ShaderDefinitions::findShadingNode(const MObject& node, ShadingNode& snode)
{
    const MString nodeTypeName = getDepNodeTypeName(node);

    if (findShadingNode(nodeTypeName, snode))
    {
        snode.setMObject(node);
        return true;
    }
    else return false;
}

bool ShaderDefinitions::findShadingNode(const MString& nodeTypeName, ShadingNode& snode)
{
    if (!ShaderDefinitions::readDone)
        ShaderDefinitions::readShaderDefinitions();

    for (ShadingNodeVector::iterator i = shadingNodes.begin(), e = shadingNodes.end(); i != e; ++i)
    {
        if (i->typeName == nodeTypeName)
        {
            snode = *i;
            return true;
        }
    }

    return false;
}

bool ShaderDefinitions::shadingNodeSupported(const MString& typeName)
{
    ShadingNode snode;
    return findShadingNode(typeName, snode);
}
