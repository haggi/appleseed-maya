
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
#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MFnMeshData.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MGlobal.h>

namespace asf = foundation;

/*
    The mesh walker is used by the binary mesh writer to export data.
    I implement a possibility to write a proxy file for the same object.
    If proxy is turned on, a file with extension .proxyMesh will be saved.
    The proxyMesh does not need (at the moment) any complicated data structures, it is for
    preview only. So I simply store plain triangle points. This is an array of points, 3 for every triangle.
    Later in the mtap_standinMeshNode.cpp this triangle file is read and the triangles are recreated.
    Of course there is a bit overhead because we have no shared points, but because the proxymesh will be reduced,
    there will be much less connected vertices than in a normal mesh, so the overhead should be acceptable.
*/

MObject MeshWalker::checkSmoothMesh()
{
    MStatus stat;

    MFnMesh mesh(mMeshObject, &stat);
    if (!stat)
    {
        MGlobal::displayError(MString("checkSmoothMesh : could not get mesh: ") + stat.errorString());
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
        MGlobal::displayError(MString("generateSmoothMesh : could not get displaySmoothMesh attr "));
        return MObject::kNullObj;
    }

    MObject meshDataObj = mSmoothMeshData.create();
    MObject smoothMeshObj = mesh.generateSmoothMesh(meshDataObj, &stat);
    if (!stat)
    {
        MGlobal::displayError(MString("generateSmoothMesh : failed"));
        return MObject::kNullObj;
    }

    MFnMesh smoothMeshDn(smoothMeshObj, &stat);
    if (!stat)
    {
        MGlobal::displayError(MString("generateSmoothMesh : could not create smoothMeshDn: ") + stat.errorString());
        return MObject::kNullObj;
    }

    MPointArray points;
    stat = smoothMeshDn.getPoints(points);
    if (!stat)
    {
        MGlobal::displayError(MString("generateSmoothMesh : could not get points"));
    }

    MGlobal::displayInfo(MString("generateSmoothMesh : numPoints: ") + points.length());

    return smoothMeshObj;
}


MeshWalker::MeshWalker(const MDagPath& dagPath)
  : mMeshDagPath(dagPath)
{
    MStatus stat;
    mMeshObject = dagPath.node();
    mUseSmoothMesh = false;

    MObject smoothMesh = checkSmoothMesh();
    if (smoothMesh != MObject::kNullObj)
    {
        mMeshObject = smoothMesh;
        mUseSmoothMesh = true;
    }

    mMeshFn.setObject(mMeshObject);

    getObjectShadingGroups(dagPath, mPerFaceAssignments, mShadingGroups, true);

    MItMeshPolygon faceIt(mMeshObject, &stat);
    CHECK_MSTATUS(stat);

    stat = mMeshFn.getPoints(mPoints);
    if (!stat)
        MGlobal::displayError(MString("MeshWalker : getPoints: ") + stat.errorString());

    stat = mMeshFn.getNormals(mNormals, MSpace::kObject);
    if (!stat)
        MGlobal::displayError(MString("MeshWalker : normals: ") + stat.errorString());

    stat = mMeshFn.getUVs(mU, mV);
    if (!stat)
        MGlobal::displayError(MString("MeshWalker : getUvs: ") + stat.errorString());

    MGlobal::displayInfo(MString("MeshWalker : numU: ") + mU.length() + " numV:" + mV.length());

    for (uint i = 0; i < mV.length(); i++)
        MGlobal::displayInfo(MString("MeshWalker : V[") + i + "]: " + mU[i]);


    MPointArray triPoints;
    MIntArray triVtxIds;
    MIntArray faceVtxIds;
    MIntArray faceNormalIds;

    int triCount = 0;

    for(faceIt.reset(); !faceIt.isDone(); faceIt.next())
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
            mPerTriangleAssignments.append(mPerFaceAssignments[faceId]);

            for (uint triVtxId = 0; triVtxId < 3; triVtxId++)
            {
                for(uint faceVtxId = 0; faceVtxId < faceVtxIds.length(); faceVtxId++)
                {
                    if (faceVtxIds[faceVtxId] == triVtxIds[triVtxId])
                    {
                        faceRelIds[triVtxId] = faceVtxId;
                    }
                }
            }

            uint vtxId0 = faceVtxIds[faceRelIds[0]];
            uint vtxId1 = faceVtxIds[faceRelIds[1]];
            uint vtxId2 = faceVtxIds[faceRelIds[2]];
            uint normalId0 = faceNormalIds[faceRelIds[0]];
            uint normalId1 = faceNormalIds[faceRelIds[1]];
            uint normalId2 = faceNormalIds[faceRelIds[2]];
            uint uvId0 = faceUVIndices[faceRelIds[0]];
            uint uvId1 = faceUVIndices[faceRelIds[1]];
            uint uvId2 = faceUVIndices[faceRelIds[2]];

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

            triCount++;
        }
    }
}

void MeshWalker::setTransform()
{
    MMatrix matrix = mMeshDagPath.inclusiveMatrix();
    for (uint vtxId = 0; vtxId < mPoints.length(); vtxId++)
        mPoints[vtxId] *= matrix;
}

// Return the name of the mesh.
const char* MeshWalker::get_name() const
{
    return mMeshFn.name().asChar();
}

// Return vertices.
size_t MeshWalker::get_vertex_count() const
{
    return mMeshFn.numVertices();
}

asf::Vector3d MeshWalker::get_vertex(const size_t i) const
{
    return asf::Vector3d(mPoints[i].x, mPoints[i].y, mPoints[i].z);
}

// Return vertex normals.
size_t MeshWalker::get_vertex_normal_count() const
{
    return mNormals.length();
}

asf::Vector3d MeshWalker::get_vertex_normal(const size_t i) const
{
    return asf::Vector3d(mNormals[i].x, mNormals[i].y, mNormals[i].z);
}

// Return texture coordinates.
size_t MeshWalker::get_tex_coords_count() const
{
    return mU.length();
}

asf::Vector2d MeshWalker::get_tex_coords(const size_t i) const
{
    return asf::Vector2d(mU[i], mV[i]);
}

// Return material slots.
size_t MeshWalker::get_material_slot_count() const
{
    return mShadingGroups.length();
}

const char* MeshWalker::get_material_slot(const size_t i) const
{
    MString shadingGroupName = getObjectName(mShadingGroups[i]);
    return shadingGroupName.asChar();
}

// Return the number of faces.
size_t MeshWalker::get_face_count() const
{
    return mFaceList.size();
}

// Return the number of vertices in a given face.
size_t MeshWalker::get_face_vertex_count(const size_t face_index) const
{
    return 3;
}

// Return data for a given vertex of a given face.
size_t MeshWalker::get_face_vertex(const size_t face_index, const size_t vertex_index) const
{
    return mFaceList[face_index].vtxIds[vertex_index];
}

size_t MeshWalker::get_face_vertex_normal(const size_t face_index, const size_t vertex_index) const
{
    return mFaceList[face_index].normalIds[vertex_index];
}

size_t MeshWalker::get_face_tex_coords(const size_t face_index, const size_t vertex_index) const
{
    return mFaceList[face_index].uvIds[vertex_index];
}

// Return the material assigned to a given face.
size_t MeshWalker::get_face_material(const size_t face_index) const
{
    return 0;
}
