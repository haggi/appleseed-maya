#ifndef MTM_ATTR_TOOLS_H
#define MTM_ATTR_TOOLS_H

#include <maya/MPlug.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MString.h>
#include <maya/MColor.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>

enum ATTR_TYPE
{
    ATTR_TYPE_NONE = 0,
    ATTR_TYPE_COLOR = 1,
    ATTR_TYPE_FLOAT = 2,
    ATTR_TYPE_VECTOR = 3
};

int getChildId(const MPlug& plug);

float getDegree(const char* plugName, const MFnDependencyNode& dn);

float getRadians(const char* plugName, const MFnDependencyNode& dn);

float getFloatAttr(const char* plugName, const MFnDependencyNode& dn, const float defaultValue);

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

ATTR_TYPE getPlugAttrType(const char *plugName, const MFnDependencyNode& dn);

ATTR_TYPE getPlugAttrType(MPlug plug);

#endif
