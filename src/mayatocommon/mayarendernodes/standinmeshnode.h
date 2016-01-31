
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

#ifndef STANDIN_MESH_H
#define STANDIN_MESH_H

#include <maya/MPxNode.h>
#include <maya/MObject.h>
#include <maya/MPointArray.h>
#include <maya/MIntArray.h>
#include <fstream>

#define STANDIN_MESH_NODE_NAME "standinMeshNode"

class standinMeshNode : public MPxNode
{
  public:
                    standinMeshNode() {}
    virtual         ~standinMeshNode() {}
    virtual MStatus compute(const MPlug& plug, MDataBlock& data);
    static  void*   creator();
    static  MStatus initialize();

    static MObject  time;
    static MObject  outputMesh;
    static MObject  binMeshFile;
    static MObject  percentDisplay;
    static MObject  polySizeMultiplier;

    std::fstream    pFile;

    static MTypeId  id;

    bool    checkMeshFileName(MString fileName);

    inline void read(double& value)
    {
        pFile.read(reinterpret_cast<char *>(&value), sizeof(double));
    }
    inline void read(MPoint& point)
    {
        for (uint i = 0; i < 3; i++)
            this->read(point[i]);
    }
    inline void read(MPointArray& points)
    {
        for (uint i = 0; i < points.length(); i++)
            this->read(points[i]);
    }
    inline void read(int& value)
    {
        pFile.read(reinterpret_cast<char *>(&value), sizeof(int));
    }

  private:
    MString binmesh_file;
    float   percent_display;
    float   poly_size_multiplier;
    bool    recreateProxyFile;

    // mesh data
    MPointArray points;
    MIntArray        numFaces;
    MIntArray        faceConnects;

  protected:
    MObject createMesh(const MTime& time, MObject& outData, MStatus& stat);
};


#endif
