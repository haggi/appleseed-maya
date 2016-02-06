
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
#include "proxymesh.h"

namespace asf = foundation;

void* BinMeshWriterCmd::creator()
{
    return new BinMeshWriterCmd();
}

MSyntax BinMeshWriterCmd::newSyntax()
{
    MSyntax syntax;
    MStatus stat;

    stat = syntax.addFlag("-pa", "-path", MSyntax::kString);
    stat = syntax.addFlag("-dp", "-doProxy", MSyntax::kBoolean);
    stat = syntax.addFlag("-p", "-percentage", MSyntax::kDouble);
    stat = syntax.addFlag("-all", "-exportAll", MSyntax::kBoolean);
    stat = syntax.addFlag("-fpm", "-oneFilePerMesh", MSyntax::kBoolean);
    stat = syntax.addFlag("-dt", "-doTransform", MSyntax::kBoolean);
    stat = syntax.addFlag("-sp", "-useSmoothPreview", MSyntax::kBoolean);
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
    if (!stat)
    {
        MGlobal::displayError(MString("removeSmoothMesh : could not delete smooth node: ") + dagPath.fullPathName());
    }
}

bool BinMeshWriterCmd::exportBinMeshes()
{
    asf::GenericMeshFileWriter globalWriter(mPath.asChar());

    ProxyMesh globalProxyMesh(mPercentage);

    // transform
    // single files

    for (uint dagPathId = 0; dagPathId < mExportObjects.length(); dagPathId++)
    {
        MDagPath dagPath = mExportObjects[dagPathId];
        MString partialPathName = dagPath.partialPathName();

        MDagPath origMeshPath = dagPath;

        MeshWalker walker(dagPath);

        if (mDoTransform)
            walker.setTransform();

        if (mOneFilePerMesh)
        {
            // replace filename.binaraymesh with filename_objname.binarymesh
            MString perFileMeshPath = pystring::replace(mPath.asChar(), ".binarymesh", "").c_str();
            perFileMeshPath += makeGoodString(partialPathName) + ".binarymesh";
            Logging::debug(MString("BinMeshWriterCmd::exportBinMeshes - exporting ") + partialPathName + " to  " + perFileMeshPath);
            asf::GenericMeshFileWriter writer(perFileMeshPath.asChar());
            writer.write(walker);
            if (mDoProxy)
            {
                ProxyMesh proxyMesh(mPercentage);
                proxyMesh.addMesh(walker);
                MString proxyFileName = pystring::replace(perFileMeshPath.asChar(), ".binarymesh" , ".proxymesh").c_str();
                proxyMesh.writeFile(proxyFileName);
            }
        }
        else
        {
            globalWriter.write(walker);
            if (mDoProxy)
                globalProxyMesh.addMesh(walker);
        }
    }

    if (mDoProxy && (!mOneFilePerMesh))
    {
        MString proxyFileName = pystring::replace(mPath.asChar(), ".binarymesh" , ".proxymesh").c_str();
        globalProxyMesh.writeFile(proxyFileName);
    }
    return true;
}

MStatus BinMeshWriterCmd::doIt(const MArgList& args)
{
    MStatus stat = MStatus::kSuccess;
    MGlobal::displayInfo("Executing BinMeshWriterCmd...");
    Logging::setLogLevel(Logging::LevelDebug);

    MArgDatabase argData(syntax(), args);

    mExportAll = false;
    if (argData.isFlagSet("-all", &stat))
    {
        argData.getFlagArgument("-exportAll", 0, mExportAll);
        Logging::debug(MString("export all: ") + mExportAll);
    }

    mPath = "";
    if (argData.isFlagSet("-path", &stat))
    {
        argData.getFlagArgument("-path", 0, mPath);
        Logging::debug(MString("path: ") + mPath);
    }

    mDoProxy = true;
    if (argData.isFlagSet("-doProxy", &stat))
    {
        argData.getFlagArgument("-doProxy", 0, mDoProxy);
        Logging::debug(MString("Create proxyFile:") + mDoProxy);
    }

    mPercentage = 0.0f;
    if (argData.isFlagSet("-percentage", &stat))
    {
        double p = 0.0;
        argData.getFlagArgument("-percentage", 0, p);
        mPercentage = (float)p;
        Logging::debug(MString("percentage: ") + mPercentage);
        if (mPercentage <= 0.0)
            mDoProxy = false;
    }

    if ((mPath == ""))
    {
        MGlobal::displayError("binMeshTranslator failed: no path for export.\n");
        printUsage();
        return  MStatus::kFailure;
    }

    mOneFilePerMesh = false;
    if (argData.isFlagSet("-oneFilePerMesh", &stat))
    {
        argData.getFlagArgument("-oneFilePerMesh", 0, mOneFilePerMesh);
        Logging::debug(MString("Create one file per mesh: ") + mOneFilePerMesh);
    }

    mDoTransform = true;
    if (argData.isFlagSet("-doTransform", &stat))
    {
        argData.getFlagArgument("-doTransform", 0, mDoTransform);
        Logging::debug(MString("Use transform: ") + mDoTransform);
    }

    getObjectsForExport(args);
    if (mExportObjects.length() == 0)
    {
        Logging::error("No mesh objects for export found.");
        return MStatus::kFailure;
    }

    mUseSmoothPreview = false;
    if (argData.isFlagSet("-useSmoothPreview", &stat))
    {
        argData.getFlagArgument("-useSmoothPreview", 0, mUseSmoothPreview);
        Logging::debug(MString("Use smooth preview: ") + mUseSmoothPreview);
    }

    // if we write all objects into one file, is is not useful to ignore the transformation, so we turn it on.
    if ((!mOneFilePerMesh) && (mExportObjects.length() > 1))
    {
        if (mDoTransform != true)
            Logging::warning(MString("Do transform is off but we have several meshes in one file and ignoring transform is not useful here -> turning it on."));
        mDoTransform = true;
    }

    exportBinMeshes();

    MGlobal::displayInfo("BinMeshWriterCmd done.\n");
    return MStatus::kSuccess;
}

void BinMeshWriterCmd::getObjectsForExport(const MArgList& args)
{
    MStatus status;

    if (mExportAll)
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

            if (IsVisible(dagPath))
            {
                if (dagPath.node().hasFn(MFn::kMesh))
                    mExportObjects.append(dagPath);
            }
            else
            {
                Logging::debug(MString("Node ") + dagPath.partialPathName() + " is not visible and will not be exported");
            }
        }

    }
    else
    {
        MSelectionList list;
        MArgDatabase argData(syntax(), args);
        if (argData.getObjects(list) == MS::kSuccess)
        {
            for (MItSelectionList listIter(list); !listIter.isDone(); listIter.next())
            {
                MDagPath dagPath;
                MObject component;
                listIter.getDagPath(dagPath, component);
                MFnDependencyNode nodeFn;
                nodeFn.setObject(dagPath.node());
                MGlobal::displayInfo(MString("Selected object: ") + nodeFn.name());
                if (dagPath.node().hasFn(MFn::kMesh))
                    mExportObjects.append(dagPath);
            }
            if (mExportObjects.length() == 0)
            {
                Logging::error("Not mesh objects selected for export.");
                return;
            }
        }
        else
        {
            Logging::error("Nothing selected for export.");
            return;
        }
    }
}
