
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

#include "binmeshtranslator.h"

#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MItSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>

#include <maya/MIOStream.h>
#include <maya/MFStream.h>

#include "utilities/tools.h"

BinMeshTranslator::BinMeshTranslator()
{
}

void* BinMeshTranslator::creator()
{
    return new BinMeshTranslator();
}

MString BinMeshTranslator::defaultExtension() const
{
    return "binarymesh";
}

//Summary:  saves a file of a type supported by this translator by traversing
//          the all or selected objects (depending on mode) in the current
//          Maya scene, and writing a representation to the given file
//Args   :  file - object containing the pathname of the file to be written to
//          options - a string representation of any file options
//          mode - the method used to write the file - export, or export active
//                 are valid values; method will fail for any other values
//Returns:  MStatus::kSuccess if the export was successful;
//          MStatus::kFailure otherwise
MStatus BinMeshTranslator::writer(
    const MFileObject&                  file,
    const MString&                      opts,
    MPxFileTranslator::FileAccessMode   mode)
{
    mOptions = opts;
    #if defined (OSMac_)
        char nameBuffer[MAXPATHLEN];
        strcpy (nameBuffer, file.fullName().asChar());
        fileName(nameBuffer);
    #else
        mFileName = file.fullName();
    #endif

    if (MPxFileTranslator::kExportAccessMode == mode)
    {
        MGlobal::displayInfo("writer - export all.");
        return exportObjects("all");
    }

    if (MPxFileTranslator::kExportActiveAccessMode == mode)
    {
        MGlobal::displayInfo("writer - export selected.");
        return exportObjects("selected");
    }

    return MS::kSuccess;
}

MStatus BinMeshTranslator::reader(
    const MFileObject&                  file,
    const MString&                      opts,
    MPxFileTranslator::FileAccessMode   mode)
{
    mOptions = opts;
    #if defined (OSMac_)
        char nameBuffer[MAXPATHLEN];
        strcpy (nameBuffer, file.fullName().asChar());
        fileName(nameBuffer);
    #else
        mFileName = file.fullName();
    #endif

    MGlobal::displayInfo("Options " + mOptions);

    return importObjects();
}

bool BinMeshTranslator::haveWriteMethod() const
{
    return true;
}


bool BinMeshTranslator::haveReadMethod() const
{
    return true;
}

//Summary:  returns true if the translator can open and import files;
//          false if it can only import files
//Returns:  true
bool BinMeshTranslator::canBeOpened() const
{
    return true;
}

MStatus BinMeshTranslator::importObjects()
{
    MString cmd = MString("import binmeshtranslator; binmeshtranslator.binMeshTranslatorRead(");
    cmd += "'" + mFileName + "',";
    cmd += "'" + mOptions + "',";
    cmd += "'read')";

    MGlobal::displayInfo(MString("translator cmd: ") + cmd);
    return MGlobal::executePythonCommand(cmd, true);
}

//Summary:  finds and outputs all selected polygonal meshes in the DAG
//Args   :  os - an output stream to write to
//Returns:  MStatus::kSuccess if the method succeeds
//          MStatus::kFailure if the method fails
MStatus BinMeshTranslator::exportObjects(const MString& mode)
{
    MString cmd = MString("import binmeshtranslator; binmeshtranslator.binMeshTranslatorWrite(");
    cmd += "'" + mFileName + "',";
    cmd += "'" + mOptions + "',";
    cmd += "'" + mode + "')";

    MGlobal::displayInfo(MString("translator cmd: ") + cmd);
    return MGlobal::executePythonCommand(cmd, true);
}
