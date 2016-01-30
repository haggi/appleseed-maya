
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

#ifndef TOOLS_BINMESH_H
#define TOOLS_BINMESH_H

#include <maya/MPxFileTranslator.h>
#include <maya/MDagPath.h>
#include <maya/MObjectArray.h>

class polyWriter;
class MDagPath;
class MFnDagNode;

class BinMeshTranslator:public MPxFileTranslator {

    public:
                                BinMeshTranslator();
        virtual                 ~BinMeshTranslator();

        virtual MStatus         writer (const MFileObject& file,
                                        const MString& optionsString,
                                        MPxFileTranslator::FileAccessMode mode);
        virtual MStatus         reader ( const MFileObject& file,
                                        const MString& optionsString,
                                        FileAccessMode mode);
        virtual bool            haveWriteMethod () const;
        virtual bool            haveReadMethod () const;
        virtual bool            canBeOpened () const;
        static void*            creator();

        virtual MString         defaultExtension () const;


    protected:
        MStatus                 exportObjects(MString mode);
        MStatus                 importObjects();
        MString                 fileName;
        MString                 options;
};

#endif
