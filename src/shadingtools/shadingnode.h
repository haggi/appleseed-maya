
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

#ifndef SHADINGTOOLS_SHADINGNODE_H
#define SHADINGTOOLS_SHADINGNODE_H

// Maya headers.
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MString.h>

// Standard headers.
#include <string>
#include <vector>

struct ShaderAttribute
{
    ShaderAttribute()
    {
        optionMenu = false;
        isArrayPlug = false;
    }

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
    std::string compAttrArrayPath;
    bool optionMenu;
    bool isArrayPlug;
    AttributeType atype;
};

class ShadingNode
{
  public:

    ShadingNode();
    explicit ShadingNode(const MObject& object);

    ShadingNode(const ShadingNode &other);

    void setMObject(const MObject& object);
    bool isAttributeValid(const MString& attributeName) const;
    bool isInPlugValid(const MPlug& plug) const;
    bool isOutPlugValid(const MPlug& plug) const;
    void getConnectedInputObjects(MObjectArray& objectArray) const;

    void addInputAttribute(const ShaderAttribute& att)
    {
        inputAttributes.push_back(att);
    }

    void addOutputAttribute(const ShaderAttribute& att)
    {
        outputAttributes.push_back(att);
    }

    bool operator==(const ShadingNode& other) const
    {
        return mobject == other.mobject;
    }

    MString typeName; //Lambert, MultiplyDivide
    MString fullName; //myLambert1, mdivi_number_123
    MObject mobject;
    std::vector<ShaderAttribute> inputAttributes;
    std::vector<ShaderAttribute> outputAttributes;
};

#endif  // !SHADINGTOOLS_SHADINGNODE_H
