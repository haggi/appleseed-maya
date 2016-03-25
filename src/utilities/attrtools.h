
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

#ifndef MTM_ATTR_TOOLS_H
#define MTM_ATTR_TOOLS_H

#include <maya/MColor.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MString.h>
#include <maya/MTypes.h>
#include <maya/MVector.h>

enum ATTR_TYPE
{
    ATTR_TYPE_NONE = 0,
    ATTR_TYPE_COLOR = 1,
    ATTR_TYPE_FLOAT = 2,
    ATTR_TYPE_VECTOR = 3
};

int getChildId(const MPlug& plug);

double getDegrees(const char* plugName, const MFnDependencyNode& dn);

float getRadians(const char* plugName, const MFnDependencyNode& dn);

float getFloatAttr(const char* plugName, const MFnDependencyNode& dn, const float defaultValue);
float getDoubleAttr(const char* plugName, const MFnDependencyNode& dn, const double defaultValue);

bool getFloat(const MString& plugName, const MFnDependencyNode& dn, float& value);

bool getFloat(const char* plugName, const MFnDependencyNode& dn, float& value);

bool getFloat2(const MString& plugName, const MFnDependencyNode& dn, float2& value);

bool getFloat2(const MString& plugName, const MFnDependencyNode& dn, MVector& value);

bool getDouble(const MString& plugName, const MFnDependencyNode& dn, double& value);

bool getString(const MString& plugName, const MFnDependencyNode& dn, MString& value);

MString getString(const char *plugName, const MFnDependencyNode& dn);

MString getStringAttr(MString plugName, const MFnDependencyNode& dn, MString default_value);

bool getInt(const MString& plugName, const MFnDependencyNode& dn, int& value);

bool getInt(const char *plugName, const MFnDependencyNode& dn, int& value);

int getIntAttr(const char *plugName, const MFnDependencyNode& dn, int defaultValue);

bool getUInt(const char *plugName, const MFnDependencyNode& dn, uint& value);

bool getBool(const MString& plugName, const MFnDependencyNode& dn, bool& value);

bool getBool(const char *plugName, const MFnDependencyNode& dn, bool& value);

bool getBoolAttr(const char *plugName, const MFnDependencyNode& dn, bool defaultValue);

bool getEnum(const MString& plugName, const MFnDependencyNode& dn, int& value);

bool getEnum(const MString& plugName, const MFnDependencyNode& dn, int& id, MString& value);

bool getEnum(const char *plugName, const MFnDependencyNode& dn, int& id, MString& value);

MString getEnumString(MString plugName, const MFnDependencyNode& dn);

MString getEnumString(const MPlug& plug);

int getEnumInt(MString plugName, const MFnDependencyNode& dn);

int getEnumInt(MPlug plug);

bool getInt2(const MString& plugName, const MFnDependencyNode& dn, int2& value);

bool getLong(const MString& plugName, const MFnDependencyNode& dn, long& value);

bool getColor(const MString& plugName, const MFnDependencyNode& dn, MColor& value);

bool getColor(const MString& plugName, const MFnDependencyNode& dn, MString& value);

bool getColor(const char *plugName, const MFnDependencyNode& dn, MColor& value);

bool getColor(const char *plugName, const MFnDependencyNode& dn, MString& value);

bool getColor(const char *plugName, const MFnDependencyNode& dn, float *value);

MColor getColorAttr(const char *plugName, const MFnDependencyNode& dn);

MColor getColorAttr(MPlug plug);

bool getVector(const MString& plugName, const MFnDependencyNode& dn, MVector& value);

MVector getVectorAttr(const char *plugName, const MFnDependencyNode& dn);

MVector getVectorAttr(MPlug plug);

bool getPoint(const MString& plugName, const MFnDependencyNode& dn, MPoint& value);

bool getPoint(const MString& plugName, const MFnDependencyNode& dn, MVector& value);

bool getMsgObj(const char *plugName, const MFnDependencyNode& dn, MObject& value);

MMatrix getMatrix(const char *plugName, const MFnDependencyNode& dn);

MMatrix getMatrix(const MPlug& plug);

ATTR_TYPE getPlugAttrType(const char *plugName, const MFnDependencyNode& dn);

ATTR_TYPE getPlugAttrType(MPlug plug);

#endif
