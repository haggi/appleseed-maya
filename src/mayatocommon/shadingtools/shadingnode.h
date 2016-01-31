
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

#ifndef MT_SHADING_TOOLS_SHADINGNODE_H
#define MT_SHADING_TOOLS_SHADINGNODE_H

#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MObjectArray.h>
#include <maya/MString.h>
#include <string>
#include <vector>

struct ShaderAttribute
{
    ShaderAttribute()
    {}

    enum AttributeType
    {
        NONE=0,
        MESSAGE,
        FLOAT,
        COLOR,
        VECTOR,
        STRING,
        BOOL,
        MATRIX
    };

    std::string name;
    std::string type;
    std::string hint;
    bool optionMenu = false;
    AttributeType atype;
};

#define SPLUG_LIST std::vector<ShadingPlug>

class ShadingNode
{
  public:
    MString typeName; //Lambert, MultiplyDivide
    MString fullName; //myLambert1, mdivi_number_123
    MObject mobject;
    std::vector<ShaderAttribute> inputAttributes;
    std::vector<ShaderAttribute> outputAttributes;

    ShadingNode(MObject& object);
    ShadingNode(const ShadingNode &other);
    ShadingNode();
    ~ShadingNode();

    bool operator==(ShadingNode const& otherOne)
    {
        return mobject == otherOne.mobject;
    }

    void setMObject(MObject object);
    bool isAttributeValid(MString attributeName);
    bool isInPlugValid(MPlug plug);
    bool isOutPlugValid(MPlug plug);
    void getConnectedInputObjects(MObjectArray& objectArray);
    void getConnectedOutputObjects(MObjectArray& objectArray);
    void addInputAttribute(ShaderAttribute att)
    {
        inputAttributes.push_back(att);
    }
    void addOutputAttribute(ShaderAttribute att)
    {
        outputAttributes.push_back(att);
    }
};

#endif
