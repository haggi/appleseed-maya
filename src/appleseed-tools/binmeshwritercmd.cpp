
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

#include "binmeshwritercmd.h"

#include "foundation/mesh/genericmeshfilewriter.h"
#include "appleseedmeshwalker.h"

#include <maya/MGlobal.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>

#include "utilities/pystring.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"

static Logging logger;

#include "proxymesh.h"


void* BinMeshWriterCmd::creator()
{
    return new BinMeshWriterCmd();
}

BinMeshWriterCmd::BinMeshWriterCmd() {}
BinMeshWriterCmd::~BinMeshWriterCmd() {}

MSyntax BinMeshWriterCmd::newSyntax()
{
    MSyntax syntax;
    MStatus stat;

    stat = syntax.addFlag( "-pa", "-path", MSyntax::kString);
    stat = syntax.addFlag( "-dp", "-doProxy", MSyntax::kBoolean);
    stat = syntax.addFlag( "-p", "-percentage", MSyntax::kDouble);
    stat = syntax.addFlag( "-all", "-exportAll", MSyntax::kBoolean);
    stat = syntax.addFlag( "-fpm", "-oneFilePerMesh", MSyntax::kBoolean);
    stat = syntax.addFlag( "-dt", "-doTransform", MSyntax::kBoolean);
    stat = syntax.addFlag( "-sp", "-useSmoothPreview", MSyntax::kBoolean);
    syntax.setObjectType(MSyntax::kSelectionList);
    syntax.useSelectionAsDefault(true);

    return syntax;
}

void BinMeshWriterCmd::printUsage()
{
    MGlobal::displayInfo("BinMeshWriterCmd usage: ... ");
}

void BinMeshWriterCmd::removeSmoothMesh(MDagPath& dagPath)
{
    MObject node = dagPath.node();
    MStatus stat = MGlobal::deleteNode(node);
    if( !stat )
    {
        MGlobal::displayError(MString("removeSmoothMesh : could not delete smooth node: ") + dagPath.fullPathName());
    }
}

bool BinMeshWriterCmd::exportBinMeshes()
{
    asf::GenericMeshFileWriter globalWriter(this->path.asChar());

    ProxyMesh globalProxyMesh(this->percentage);

    // transform
    // single files

    for( uint dagPathId = 0; dagPathId < this->exportObjects.length(); dagPathId++)
    {
        MDagPath dagPath = this->exportObjects[dagPathId];
        MString partialPathName = dagPath.partialPathName();

        MDagPath origMeshPath = dagPath;

        MeshWalker walker(dagPath);

        if( this->doTransform )
            walker.setTransform();

        if( this->oneFilePerMesh )
        {
            // replace filename.binaraymesh with filename_objname.binarymesh
            MString perFileMeshPath = pystring::replace(this->path.asChar(), ".binarymesh", "").c_str();
            perFileMeshPath += makeGoodString(partialPathName) + ".binarymesh";
            logger.debug(MString("BinMeshWriterCmd::exportBinMeshes - exporting ") + partialPathName + " to  " + perFileMeshPath);
            asf::GenericMeshFileWriter writer(perFileMeshPath.asChar());
            writer.write(walker);
            if( this->doProxy )
            {
                ProxyMesh proxyMesh(this->percentage);
                proxyMesh.addMesh(walker);
                MString proxyFileName = pystring::replace(perFileMeshPath.asChar(), ".binarymesh" , ".proxymesh").c_str();
                proxyMesh.writeFile(proxyFileName);
            }
        }else{
            globalWriter.write(walker);
            if( this->doProxy )
                globalProxyMesh.addMesh(walker);
        }
    }

    if( this->doProxy && (!this->oneFilePerMesh))
    {
        MString proxyFileName = pystring::replace(this->path.asChar(), ".binarymesh" , ".proxymesh").c_str();
        globalProxyMesh.writeFile(proxyFileName);
    }
    return true;
}

MStatus BinMeshWriterCmd::doIt( const MArgList& args)
{
    MStatus stat = MStatus::kSuccess;
    MGlobal::displayInfo("Executing BinMeshWriterCmd...");
    logger.setLogLevel(Logging::LevelDebug);

    MArgDatabase argData(syntax(), args);

    exportAll = false;
    if( argData.isFlagSet("-all", &stat))
    {
        argData.getFlagArgument("-exportAll", 0, exportAll);
        logger.debug(MString("export all: ") + exportAll);
    }

    path = "";
    if( argData.isFlagSet("-path", &stat))
    {
        argData.getFlagArgument("-path", 0, path);
        logger.debug(MString("path: ") + path);
    }

    this->doProxy = true;
    if( argData.isFlagSet("-doProxy", &stat))
    {
        argData.getFlagArgument("-doProxy", 0, this->doProxy);
        logger.debug(MString("Create proxyFile:") + this->doProxy);
    }

    this->percentage = 0.0f;
    if( argData.isFlagSet("-percentage", &stat))
    {
        double p = 0.0;
        argData.getFlagArgument("-percentage", 0, p);
        percentage = (float)p;
        logger.debug(MString("percentage: ") + percentage);
        if( percentage <= 0.0)
            doProxy = false;
    }

    if( (path == ""))
    {
        MGlobal::displayError("binMeshTranslator failed: no path for export.\n");
        printUsage();
        return  MStatus::kFailure;
    }

    this->oneFilePerMesh = false;
    if( argData.isFlagSet("-oneFilePerMesh", &stat))
    {
        argData.getFlagArgument("-oneFilePerMesh", 0, this->oneFilePerMesh);
        logger.debug(MString("Create one file per mesh: ") + this->oneFilePerMesh);
    }

    this->doTransform = true;
    if( argData.isFlagSet("-doTransform", &stat))
    {
        argData.getFlagArgument("-doTransform", 0, this->doTransform);
        logger.debug(MString("Use transform: ") + this->doTransform);
    }

    getObjectsForExport(args);
    if(this->exportObjects.length() == 0)
    {
        logger.error("No mesh objects for export found.");
        return MStatus::kFailure;
    }

    this->useSmoothPreview = false;
    if( argData.isFlagSet("-useSmoothPreview", &stat))
    {
        argData.getFlagArgument("-useSmoothPreview", 0, this->useSmoothPreview);
        logger.debug(MString("Use smooth preview: ") + this->useSmoothPreview);
    }

    // if we write all objects into one file, is is not useful to ignore the transformation, so we turn it on.
    if( (!this->oneFilePerMesh) && (this->exportObjects.length() > 1))
    {
        if( this->doTransform != true)
            logger.warning(MString("Do transform is off but we have several meshes in one file and ignoring transform is not useful here -> turning it on."));
        this->doTransform = true;
    }

    exportBinMeshes();

    MGlobal::displayInfo("BinMeshWriterCmd done.\n");
    return MStatus::kSuccess;
}

void BinMeshWriterCmd::getObjectsForExport(const MArgList& args)
{
    MStatus status;

    if( exportAll )
    {
        MStatus status;

        MItDag itDag(MItDag::kDepthFirst, MFn::kMesh, &status);

        if (MStatus::kFailure == status)
        {
            MGlobal::displayError("MItDag::MItDag");
            return;
        }

        for(;!itDag.isDone();itDag.next())
        {
            MDagPath dagPath;
            if (MStatus::kFailure == itDag.getPath(dagPath))
            {
                MGlobal::displayError("MDagPath::getPath");
                continue;
            }

            if( IsVisible(dagPath) )
            {
                if( dagPath.node().hasFn(MFn::kMesh))
                    this->exportObjects.append(dagPath);
            }else{
                logger.debug(MString("Node ") + dagPath.partialPathName() + " is not visible and will not be exported");
            }
        }

    }else{
        MSelectionList list;
        MArgDatabase argData(syntax(), args);
        if( argData.getObjects(list) == MS::kSuccess)
        {
            for ( MItSelectionList listIter( list ); !listIter.isDone(); listIter.next() )
            {
                MDagPath dagPath;
                MObject component;
                listIter.getDagPath( dagPath, component );
                MFnDependencyNode nodeFn;
                nodeFn.setObject(dagPath.node());
                MGlobal::displayInfo(MString("Selected object: ") + nodeFn.name());
                if( dagPath.node().hasFn(MFn::kMesh))
                    this->exportObjects.append(dagPath);
            }
            if( this->exportObjects.length() == 0)
            {
                logger.error("Not mesh objects selected for export.");
                return;
            }
        }else{
            logger.error("Nothing selected for export.");
            return;
        }
    }
}
