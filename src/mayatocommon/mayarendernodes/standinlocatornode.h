
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

#ifndef MT_STANDIN_H
#define MT_STANDIN_H

#include <maya/MPxLocatorNode.h>
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnLightDataAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MVector.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MDistance.h>
#include <math.h>

class StandinLocatorNode : public MPxLocatorNode
{
  public:
                      StandinLocatorNode();
    virtual           ~StandinLocatorNode();

    virtual void            draw(M3dView & view, const MDagPath & path,
                                  M3dView::DisplayStyle style,
                                  M3dView::DisplayStatus status);

    virtual bool            isBounded() const;
    virtual MBoundingBox    boundingBox() const;

    static MStatus    initialize();

  private:
    // Inputs
    static MObject  bboxMin;
    static MObject  bboxMax;
    static MObject  proxyFile;
    static MObject  displayType;
    static MObject  percentDisplay;
    static MObject  elementSize;

    MPoint      bboxmin, bboxmax;
    MString     proxy_file;
    float       percent_display;
    int         element_size;

    // Outputs
    static MObject  dummyOutput;

    MFloatVectorArray points;
};

#endif
