
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

#ifndef PROXYMESH_H
#define PROXYMESH_H

#include "foundation/mesh/imeshwalker.h"

#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MIntArray.h>

#include <fstream>

class ProxyMesh
{
  public:
    explicit ProxyMesh(const float percentage);

    void addMesh(const foundation::IMeshWalker& walker);

    void writeFile(const MString& fileName);

  private:
    void write(const int value);
    void write(const double value);
    void write(const MPoint& point);
    void write(const MPointArray& points);
    void write(const MString& value);

    void setMin(const MPoint& vtx);
    void setMax(const MPoint& vtx);
    void setBBox(const MPoint& vtx);
    void scaleFace(int firstVtxIndex, int numVertices);

    const float     mPercentage;
    const float     mPolySizeMultiplier;
    MPointArray     mPoints;
    MPoint          mMin, mMax;
    std::fstream    mProxyFile;
    MStringArray    mShadingGroupNames;
    MIntArray       mPolyShaderIds;
};

#endif  // !PROXYMESH_H
