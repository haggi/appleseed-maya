
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

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include "binmeshtranslator.h"
#include "binmeshwritercmd.h"
#include "binmeshreadercmd.h"


#define VENDOR "haggis vfx & animation"
#define VERSION "1.0"
#define TRANSLATORNAME "appleseedBinaryMesh"
#define WRITERNAME "binMeshWriterCmd"
#define READERNAME "binMeshReaderCmd"

MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, VENDOR, VERSION, "Any");
    MGlobal::displayInfo(MString("Loading plugin appleseedTools version: ") + MString(VERSION));

    status =  plugin.registerFileTranslator(TRANSLATORNAME,
                                            "", // pixmap name
                                            BinMeshTranslator::creator,
                                            "binMeshTranslatorOpts", // options display script name
                                            "", // default options which are passed to the display script
                                            true); // can use MGlobal::executeCommand ?

    // I try to avoid creating any mel scripts because I prefer python. Unfortunatly the options script seems only to work properly with
    // mel scripts so I create this converter.

    MString melToPythonCmd = "global proc binMeshTranslatorOpts(string $a, string $b, string $c, string $d)\n \
    {\n \
    python(\"import binMeshTranslator; binMeshTranslator.binMeshTranslatorOpts('\" + $a + \"','\" + $b + \"','\" + $c + \"','\" + $d + \"')\");\n \
    }\n";

    MGlobal::displayInfo(MString(" mel to python cmd: ") + melToPythonCmd);
    MGlobal::executeCommand(melToPythonCmd);

    if (!status)
    {
        status.perror("registerFileTranslator");
        return status;
    }

    status = plugin.registerCommand(WRITERNAME, BinMeshWriterCmd::creator, BinMeshWriterCmd::newSyntax );
    if (!status) {
        status.perror("cannot register command: binMeshWriterCmd");
        return status;
    }

    status = plugin.registerCommand(READERNAME, BinMeshReaderCmd::creator, BinMeshReaderCmd::newSyntax );
    if (!status) {
        status.perror("cannot register command: BinMeshReaderCmd");
        return status;
    }

    return status;
}


MStatus uninitializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin( obj );

    status =  plugin.deregisterFileTranslator(TRANSLATORNAME);
    if (!status)
    {
        status.perror("deregisterFileTranslator");
        return status;
    }

    status = plugin.deregisterCommand(WRITERNAME);
    if (!status) {
        status.perror("cannot deregister command: binMeshWriterCmd");
        return status;
    }

    status = plugin.deregisterCommand( READERNAME );
    if (!status) {
        status.perror("cannot deregister command: binMeshReaderCmd");
        return status;
    }

    return status;
}
