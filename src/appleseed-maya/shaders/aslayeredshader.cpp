
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

#include "aslayeredshader.h"

#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnLightDataAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MGlobal.h>
#include <maya/MDrawRegistry.h>
#include <maya/MDGModifier.h>

MTypeId asLayeredShader::id(0x0011CF44);

void asLayeredShader::postConstructor()
{
    setMPSafe(true);
    this->setExistWithoutInConnections(true);
}

MObject asLayeredShader::baseMaterial;
MObject asLayeredShader::materialEntryMtl;
MObject asLayeredShader::materialEntryMsk;
MObject asLayeredShader::materialEntryMode;

MObject asLayeredShader::outColor;

asLayeredShader::asLayeredShader() { }
asLayeredShader::~asLayeredShader() { }

void* asLayeredShader::creator()
{
    return new asLayeredShader();
}

MStatus asLayeredShader::initialize()
{
    MFnNumericAttribute nAttr;
    MFnLightDataAttribute lAttr;
    MFnTypedAttribute tAttr;
    MFnGenericAttribute gAttr;
    MFnEnumAttribute eAttr;
    MFnCompoundAttribute cAttr;

    MStatus status;

    baseMaterial = nAttr.createColor("baseMaterial", "baseMaterial");
    CHECK_MSTATUS(addAttribute(baseMaterial));

    outColor = nAttr.createColor("outColor", "outColor");
    CHECK_MSTATUS(addAttribute(outColor));

    materialEntryMtl = nAttr.createColor("materialEntryMtl", "materialEntryMtl");
    nAttr.setMin(0.0f);
    nAttr.setSoftMax(1.0f);
    nAttr.setKeyable(true);
    nAttr.setArray(true);
    CHECK_MSTATUS(addAttribute(materialEntryMtl));

    materialEntryMsk = nAttr.create("materialEntryMsk", "materialEntryMsk", MFnNumericData::kFloat, 0.0f);
    nAttr.setMin(0.0f);
    nAttr.setMax(1.0f);
    nAttr.setKeyable(true);
    nAttr.setArray(true);
    CHECK_MSTATUS(addAttribute(materialEntryMsk));

    materialEntryMode = eAttr.create("materialEntryMode", "materialEntryMode", 0);
    eAttr.setKeyable(true);
    eAttr.addField("over", 0);
    eAttr.addField("multiply", 1);
    eAttr.setArray(true);
    CHECK_MSTATUS(addAttribute(materialEntryMode));

    CHECK_MSTATUS(attributeAffects(baseMaterial, outColor));
    return(MS::kSuccess);
}

MStatus asLayeredShader::compute(const MPlug& plug, MDataBlock& block)
{
    if (plug == outColor || plug.parent() == outColor)
    {
        MStatus status;
        MFloatVector resultColor(0.0, 0.0, 0.0);

        MFloatVector& color = block.inputValue(baseMaterial, &status).asFloatVector();
        resultColor = color;

        MDataHandle outColorHandle = block.outputValue(outColor, &status);
        MFloatVector& outColorValue = outColorHandle.asFloatVector();
        outColorValue = resultColor;
        outColorHandle.setClean();
    }
    else
    {
        return(MS::kUnknownParameter);
    }

    return(MS::kSuccess);
}
