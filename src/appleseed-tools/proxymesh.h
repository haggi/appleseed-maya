
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

#ifndef AS_TOOLS_PROXYMESH
#define AS_TOOLS_PROXYMESH

#include "foundation/mesh/imeshwalker.h"
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MIntArray.h>
#include <fstream>

namespace asf = foundation;


class ProxyMesh
{
public:
    float   percentage;
    float   polySizeMultiplier;
    MPointArray points;
    MPoint min, max;
    std::fstream proxyFile;
    MStringArray shadingGroupNames;
    MIntArray polyShaderIds;
    MIntArray objectShaderStartId;

    inline void write(double value)
    {
        proxyFile.write(reinterpret_cast<char *>(&value), sizeof(double));
    }
    inline void write(MPoint point)
    {
        for (uint i = 0; i < 3; i++)
            this->write(point[i]);
    }
    inline void write(MPointArray points)
    {
        for (uint i = 0; i < points.length(); i++)
            this->write(points[i]);
    }
    inline void write(int value)
    {
        proxyFile.write(reinterpret_cast<char *>(&value), sizeof(int));
    }
    inline void write(MString value)
    {
        this->write((int)value.length());
        proxyFile.write(value.asChar(), value.length());
    }

    void setMin(MPoint vtx);
    void setMax(MPoint vtx);
    void setBBox(MPoint vtx);
    void addMesh(asf::IMeshWalker& walker);
    void writeFile(MString fileName);
    void scaleFace(int firstVtxIndex, int numVertices);
    ProxyMesh(float percentage);
    ~ProxyMesh();
};

#endif
