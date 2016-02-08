
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

#include "binmeshreadercmd.h"

#include "foundation/utility/searchpaths.h"
#include "foundation/utility/autoreleaseptr.h"
#include "renderer/modeling/object/meshobject.h"
#include "renderer/utility/paramarray.h"
#include "renderer/modeling/object/meshobjectreader.h"
#include "renderer/modeling/object/triangle.h"

#include <maya/MGlobal.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>

#include "utilities/pystring.h"
#include "utilities/logging.h"
#include "utilities/tools.h"

void* BinMeshReaderCmd::creator()
{
    return new BinMeshReaderCmd();
}

MSyntax BinMeshReaderCmd::newSyntax()
{
    MSyntax syntax;
    MStatus stat = syntax.addFlag("-pa", "-path", MSyntax::kString);
    return syntax;
}

void BinMeshReaderCmd::printUsage()
{
    MGlobal::displayInfo("BinMeshReaderCmd usage: ... ");
}

bool BinMeshReaderCmd::importBinMeshes()
{
    foundation::SearchPaths searchPaths;
    renderer::MeshObjectArray meshArray;

    renderer::ParamArray params;
    params.insert("filename", mPath.asChar());
    if (!renderer::MeshObjectReader::read(searchPaths, mPath.asChar(), params, meshArray))
    {
        Logging::error(MString("Unable to read meshes from ") + mPath);
        return false;
    }

    for (size_t meshId = 0; meshId < meshArray.size(); meshId++)
    {
        const unsigned int numVertices = static_cast<unsigned int>(meshArray[meshId]->get_vertex_count());
        const unsigned int numNormals = static_cast<unsigned int>(meshArray[meshId]->get_vertex_normal_count());
        const unsigned int numTriangles = static_cast<unsigned int>(meshArray[meshId]->get_triangle_count());
        const unsigned int numMaterials = static_cast<unsigned int>(meshArray[meshId]->get_material_slot_count());
        const unsigned int numUVs = static_cast<unsigned int>(meshArray[meshId]->get_tex_coords_count());

        MPointArray points(numVertices);
        MVectorArray normals(numNormals);
        MIntArray faceVertexCounts;
        MIntArray faceConnects;
        MFloatArray uArray(numUVs), vArray(numUVs);
        MIntArray uvIds;

        for (unsigned int vtxId = 0; vtxId < numVertices; vtxId++)
        {
            renderer::GVector3 p = meshArray[meshId]->get_vertex(vtxId);
            points[vtxId] = MPoint(p.x, p.y, p.z);
        }

        for (unsigned int triId = 0; triId < numTriangles; triId++)
        {
            faceVertexCounts.append(3);
            renderer::Triangle tri = meshArray[meshId]->get_triangle(triId);
            faceConnects.append(tri.m_v0);
            faceConnects.append(tri.m_v1);
            faceConnects.append(tri.m_v2);
            uvIds.append(tri.m_a0);
            uvIds.append(tri.m_a1);
            uvIds.append(tri.m_a2);
        }

        for (unsigned int uvId = 0; uvId < numUVs; uvId++)
        {
            renderer::GVector2 uv = meshArray[meshId]->get_tex_coords(uvId);
            uArray[uvId] = uv.x;
            vArray[uvId] = uv.y;
        }

        for (unsigned int nId = 0; nId < numNormals; nId++)
        {
            renderer::GVector3 v = meshArray[meshId]->get_vertex_normal(nId);
            normals[nId] = MVector(v.x, v.y, v.z);
        }

        MFnMesh newMesh;
        MStatus stat;
        newMesh.create(numVertices, numTriangles, points, faceVertexCounts, faceConnects, uArray, vArray, MObject::kNullObj, &stat);

        stat = newMesh.clearUVs();
        stat = newMesh.setUVs(uArray, vArray);
        stat = newMesh.assignUVs(faceVertexCounts, uvIds);
    }

    return true;
}

MStatus BinMeshReaderCmd::doIt(const MArgList& args)
{
    MStatus stat = MStatus::kSuccess;
    MGlobal::displayInfo("Executing BinMeshReaderCmd...");
    Logging::setLogLevel(Logging::LevelDebug);

    MArgDatabase argData(syntax(), args);

    mPath = "";
    if (argData.isFlagSet("-path", &stat))
    {
        argData.getFlagArgument("-path", 0, mPath);
        Logging::debug(MString("path: ") + mPath);
    }

    if ((mPath == ""))
    {
        MGlobal::displayError("binMeshTranslator failed: no path for export.\n");
        printUsage();
        return  MStatus::kFailure;
    }

    importBinMeshes();

    MGlobal::displayInfo("BinMeshReaderCmd done.\n");
    return MStatus::kSuccess;
}
