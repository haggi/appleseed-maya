
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

// Interface header.
#include "meshwalker.h"

// appleseed-maya headers.
#include "shadingtools/shadingutils.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"

// Maya headers.
#include <maya/MBoundingBox.h>
#include <maya/MMatrix.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MGlobal.h>

MeshWalker::MeshWalker(const MDagPath& dagPath)
  : mMeshDagPath(dagPath)
{
    mMeshObject = dagPath.node();

    MObject smoothMesh = checkSmoothMesh();
    if (smoothMesh != MObject::kNullObj)
        mMeshObject = smoothMesh;

    mMeshFn.setObject(mMeshObject);

    getObjectShadingGroups(dagPath, mPerFaceAssignments, mShadingGroups, true);

    MStatus stat;
    MItMeshPolygon faceIt(mMeshObject, &stat);
    CHECK_MSTATUS(stat);

    stat = mMeshFn.getPoints(mPoints);
    if (!stat)
        MGlobal::displayError(MString("MeshWalker: getPoints: ") + stat.errorString());

    stat = mMeshFn.getNormals(mNormals, MSpace::kObject);
    if (!stat)
        MGlobal::displayError(MString("MeshWalker: normals: ") + stat.errorString());

    stat = mMeshFn.getUVs(mU, mV);
    if (!stat)
        MGlobal::displayError(MString("MeshWalker: getUvs: ") + stat.errorString());

    MPointArray triPoints;
    MIntArray triVtxIds;
    MIntArray faceVtxIds;
    MIntArray faceNormalIds;

    for (faceIt.reset(); !faceIt.isDone(); faceIt.next())
    {
        int faceId = faceIt.index();
        int numTris;
        faceIt.numTriangles(numTris);
        faceIt.getVertices(faceVtxIds);

        MIntArray faceUVIndices;

        faceNormalIds.clear();
        for (uint vtxId = 0; vtxId < faceVtxIds.length(); vtxId++)
        {
            faceNormalIds.append(faceIt.normalIndex(vtxId));
            int uvIndex;
            faceIt.getUVIndex(vtxId, uvIndex);
            faceUVIndices.append(uvIndex);
        }

        for (int triId = 0; triId < numTris; triId++)
        {
            int faceRelIds[3];
            faceIt.getTriangle(triId, triPoints, triVtxIds);

            for (uint triVtxId = 0; triVtxId < 3; triVtxId++)
            {
                for (uint faceVtxId = 0; faceVtxId < faceVtxIds.length(); faceVtxId++)
                {
                    if (faceVtxIds[faceVtxId] == triVtxIds[triVtxId])
                        faceRelIds[triVtxId] = faceVtxId;
                }
            }

            const unsigned int vtxId0 = faceVtxIds[faceRelIds[0]];
            const unsigned int vtxId1 = faceVtxIds[faceRelIds[1]];
            const unsigned int vtxId2 = faceVtxIds[faceRelIds[2]];
            const unsigned int normalId0 = faceNormalIds[faceRelIds[0]];
            const unsigned int normalId1 = faceNormalIds[faceRelIds[1]];
            const unsigned int normalId2 = faceNormalIds[faceRelIds[2]];
            const unsigned int uvId0 = faceUVIndices[faceRelIds[0]];
            const unsigned int uvId1 = faceUVIndices[faceRelIds[1]];
            const unsigned int uvId2 = faceUVIndices[faceRelIds[2]];

            Face f;
            f.vtxIds.append(vtxId0);
            f.vtxIds.append(vtxId1);
            f.vtxIds.append(vtxId2);
            f.normalIds.append(normalId0);
            f.normalIds.append(normalId1);
            f.normalIds.append(normalId2);
            f.uvIds.append(uvId0);
            f.uvIds.append(uvId1);
            f.uvIds.append(uvId2);
            mFaceList.push_back(f);
        }
    }
}

void MeshWalker::applyTransform()
{
    MMatrix matrix = mMeshDagPath.inclusiveMatrix();

    for (unsigned int i = 0; i < mPoints.length(); ++i)
        mPoints[i] *= matrix;
}

const char* MeshWalker::get_name() const
{
    return mMeshFn.name().asChar();
}

size_t MeshWalker::get_vertex_count() const
{
    return mMeshFn.numVertices();
}

foundation::Vector3d MeshWalker::get_vertex(const size_t i) const
{
    const MPoint& p = mPoints[static_cast<unsigned int>(i)];
    return foundation::Vector3d(p.x, p.y, p.z);
}

size_t MeshWalker::get_vertex_normal_count() const
{
    return mNormals.length();
}

foundation::Vector3d MeshWalker::get_vertex_normal(const size_t i) const
{
    const MFloatVector& n = mNormals[static_cast<unsigned int>(i)];
    return foundation::Vector3d(n.x, n.y, n.z);
}

size_t MeshWalker::get_tex_coords_count() const
{
    return mU.length();
}

foundation::Vector2d MeshWalker::get_tex_coords(const size_t i) const
{
    const unsigned int j = static_cast<unsigned int>(i);
    return foundation::Vector2d(mU[j], mV[j]);
}

size_t MeshWalker::get_material_slot_count() const
{
    return mShadingGroups.length();
}

const char* MeshWalker::get_material_slot(const size_t i) const
{
    // todo: bug: returning pointer to temporary.
    MString shadingGroupName = getObjectName(mShadingGroups[static_cast<unsigned int>(i)]);
    return shadingGroupName.asChar();
}

size_t MeshWalker::get_face_count() const
{
    return mFaceList.size();
}

size_t MeshWalker::get_face_vertex_count(const size_t face_index) const
{
    return 3;
}

size_t MeshWalker::get_face_vertex(const size_t face_index, const size_t vertex_index) const
{
    return mFaceList[face_index].vtxIds[static_cast<unsigned int>(vertex_index)];
}

size_t MeshWalker::get_face_vertex_normal(const size_t face_index, const size_t vertex_index) const
{
    return mFaceList[face_index].normalIds[static_cast<unsigned int>(vertex_index)];
}

size_t MeshWalker::get_face_tex_coords(const size_t face_index, const size_t vertex_index) const
{
    return mFaceList[face_index].uvIds[static_cast<unsigned int>(vertex_index)];
}

size_t MeshWalker::get_face_material(const size_t face_index) const
{
    return 0;
}

MObject MeshWalker::checkSmoothMesh()
{
    MStatus stat;

    MFnMesh mesh(mMeshObject, &stat);
    if (!stat)
    {
        MGlobal::displayError(MString("checkSmoothMesh: could not get mesh: ") + stat.errorString());
        return MObject::kNullObj;
    }

    bool displaySmoothMesh = false;
    if (getBool("displaySmoothMesh", mesh, displaySmoothMesh))
    {
        if (!displaySmoothMesh)
            return MObject::kNullObj;
    }
    else
    {
        MGlobal::displayError("checkSmoothMesh: could not get displaySmoothMesh attr ");
        return MObject::kNullObj;
    }

    MObject meshDataObj = mSmoothMeshData.create();
    MObject smoothMeshObj = mesh.generateSmoothMesh(meshDataObj, &stat);
    if (!stat)
    {
        MGlobal::displayError("checkSmoothMesh: failed");
        return MObject::kNullObj;
    }

    MFnMesh smoothMeshDn(smoothMeshObj, &stat);
    if (!stat)
    {
        MGlobal::displayError(MString("checkSmoothMesh: could not create smoothMeshDn: ") + stat.errorString());
        return MObject::kNullObj;
    }

    return smoothMeshObj;
}
