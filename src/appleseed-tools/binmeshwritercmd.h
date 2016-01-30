
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

#ifndef BIN_MESH_WRITER_H
#define BIN_MESH_WRITER_H

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MDagPath.h>
#include <maya/MPointArray.h>
#include <maya/MObjectArray.h>
#include <maya/MDagPathArray.h>
#include <fstream>

class  BinMeshWriterCmd: public MPxCommand
{
public:
                    BinMeshWriterCmd();
    virtual         ~BinMeshWriterCmd();
    static MSyntax  newSyntax();

    MStatus         doIt(const MArgList& args);
    static void*    creator();
    void            printUsage();
    bool            exportBinMeshes();
    void            getObjectsForExport(const MArgList& args);
    void            removeSmoothMesh(MDagPath& dagPath);

    std::fstream    pFile;

    inline void write(double value)
    {
        pFile.write(reinterpret_cast<char *>(&value), sizeof(double));
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
        pFile.write(reinterpret_cast<char *>(&value), sizeof(int));
    }


private:
    bool            doProxy;
    float           percentage;
    int             nthPoly;
    MString         path;
    MString         meshName;
    MDagPathArray   exportObjects;
    bool            oneFilePerMesh;
    bool            doTransform;
    bool            exportAll;
    bool            useSmoothPreview;

};

#endif
