
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

#ifndef BIN_MESH_READER_H
#define BIN_MESH_READER_H

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MDagPath.h>
#include <maya/MPointArray.h>
#include <maya/MFloatArray.h>
#include <maya/MObjectArray.h>
#include <maya/MDagPathArray.h>

#include <fstream>

class  BinMeshReaderCmd: public MPxCommand
{
public:
    BinMeshReaderCmd();

    static MSyntax newSyntax();

    MStatus doIt(const MArgList& args);

    static void* creator();

private:
    void printUsage();
    bool importBinMeshes();

    inline void write(const int value)
    {
        pFile.write(reinterpret_cast<const char *>(&value), sizeof(int));
    }

    inline void write(const double value)
    {
        pFile.write(reinterpret_cast<const char *>(&value), sizeof(double));
    }

    inline void write(const MPoint& point)
    {
        write(point.x);
        write(point.y);
        write(point.z);
    }

    inline void write(const MPointArray& points)
    {
        for (size_t i = 0, e = points.length(); i < e; i++)
            write(points[i]);
    }

    std::fstream    pFile;
    bool            doProxy;
    float           percentage;
    MString         path;
};

#endif
