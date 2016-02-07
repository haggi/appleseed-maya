
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

#include "meshtools.h"

#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "utilities/tools.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnMeshData.h>

namespace
{
    asr::GVector3 mpointToGVector3(const MPoint& p)
    {
        return asr::GVector3((float)p.x, (float)p.y, (float)p.z);
    }
}

void getMeshData(const MObject& meshObject, MPointArray& points, MFloatVectorArray& normals)
{
    MStatus stat;
    MMeshSmoothOptions options;
    MObject mObject = meshObject;
    MFnMesh tmpMesh(mObject);
    MFnMeshData meshData;
    MObject dataObject;
    MObject smoothedObj;

    // create smooth mesh if needed
    if (tmpMesh.findPlug("displaySmoothMesh").asBool())
    {
        stat = tmpMesh.getSmoothMeshDisplayOptions(options);
        if (stat)
        {
            if (!tmpMesh.findPlug("useSmoothPreviewForRender", false, &stat).asBool())
            {
                int smoothLevel = tmpMesh.findPlug("renderSmoothLevel", false, &stat).asInt();
                options.setDivisions(smoothLevel);
            }
            if (options.divisions() > 0)
            {
                dataObject = meshData.create();
                smoothedObj = tmpMesh.generateSmoothMesh(dataObject, &options, &stat);
                if (stat)
                {
                    mObject = smoothedObj;
                }
            }
        }
    }

    MFnMesh meshFn(mObject, &stat);
    meshFn.getPoints(points);
    meshFn.getNormals(normals, MSpace::kObject);
}


void getMeshData(const MObject& meshObject, MPointArray& points, MFloatVectorArray& normals, MFloatArray& uArray,
    MFloatArray& vArray, MIntArray& triPointIndices, MIntArray& triNormalIndices,
    MIntArray& triUvIndices, MIntArray& triMatIndices, MIntArray& perFaceAssignments)
{
    MStatus stat;
    MMeshSmoothOptions options;
    MFnMesh tmpMesh(meshObject, &stat);
    MFnMeshData meshData;
    MObject dataObject;
    MObject smoothedObj;
    MObject mObject = meshObject;
    MString meshName = tmpMesh.name();

    // create smooth mesh if needed
    if (tmpMesh.findPlug("displaySmoothMesh").asBool())
    {
        stat = tmpMesh.getSmoothMeshDisplayOptions(options);
        if (stat)
        {
            if (!tmpMesh.findPlug("useSmoothPreviewForRender", false, &stat).asBool())
            {
                int smoothLevel = tmpMesh.findPlug("renderSmoothLevel", false, &stat).asInt();
                options.setDivisions(smoothLevel);
            }
            if (options.divisions() > 0)
            {
                dataObject = meshData.create();
                smoothedObj = tmpMesh.generateSmoothMesh(dataObject, &options, &stat);
                if (stat)
                {
                    mObject = smoothedObj;
                }
            }
        }
    }

    MFnMesh meshFn(mObject, &stat);
    CHECK_MSTATUS(stat);
    MItMeshPolygon faceIt(mObject, &stat);
    CHECK_MSTATUS(stat);

    meshFn.getPoints(points);
    meshFn.getNormals(normals, MSpace::kObject);
    meshFn.getUVs(uArray, vArray);

    uint numVertices = points.length();
    uint numNormals = normals.length();
    uint numUvs = uArray.length();

    // some meshes may have no uv's
    // to avoid problems I add a default uv coordinate
    if (numUvs == 0)
    {
        uArray.append(0.0);
        vArray.append(0.0);
    }
    for (uint nid = 0; nid < numNormals; nid++)
    {
        if (normals[nid].length() < 0.1f)
            Logging::warning(MString("Malformed normal in ") + meshName);
    }
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

        int perFaceShadingGroup = 0;
        if (perFaceAssignments.length() > 0)
            perFaceShadingGroup = perFaceAssignments[faceId];

        MIntArray faceUVIndices;

        faceNormalIds.clear();
        for (uint vtxId = 0; vtxId < faceVtxIds.length(); vtxId++)
        {
            faceNormalIds.append(faceIt.normalIndex(vtxId));
            int uvIndex;
            if (numUvs == 0)
            {
                faceUVIndices.append(0);
            }
            else
            {
                faceIt.getUVIndex(vtxId, uvIndex);
                faceUVIndices.append(uvIndex);
            }
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

            triPointIndices.append(vtxId0);
            triPointIndices.append(vtxId1);
            triPointIndices.append(vtxId2);

            triNormalIndices.append(normalId0);
            triNormalIndices.append(normalId1);
            triNormalIndices.append(normalId2);

            triUvIndices.append(uvId0);
            triUvIndices.append(uvId1);
            triUvIndices.append(uvId2);

            triMatIndices.append(perFaceShadingGroup);
        }
    }
}

asf::auto_release_ptr<asr::MeshObject> defineStandardPlane()
{
    asf::auto_release_ptr<asr::MeshObject> object(asr::MeshObjectFactory::create("standardPlane", asr::ParamArray()));

    object->push_vertex(asr::GVector3(-1.0f, 0.0f, -1.0f));
    object->push_vertex(asr::GVector3(-1.0f, 0.0f, 1.0f));
    object->push_vertex(asr::GVector3(1.0f, 0.0f, 1.0f));
    object->push_vertex(asr::GVector3(1.0f, 0.0f, -1.0f));

    object->push_vertex_normal(asr::GVector3(0.0f, 1.0f, 0.0f));

    object->push_tex_coords(asr::GVector2(0.0, 0.0));
    object->push_tex_coords(asr::GVector2(1.0, 0.0));
    object->push_tex_coords(asr::GVector2(1.0, 1.0));
    object->push_tex_coords(asr::GVector2(0.0, 1.0));

    object->push_triangle(asr::Triangle(0, 1, 2, 0, 0, 0, 0, 1, 2, 0));
    object->push_triangle(asr::Triangle(0, 2, 3, 0, 0, 0, 0, 2, 3, 0));

    return object;
}

asf::auto_release_ptr<asr::MeshObject> createMesh(const MObject& mobject)
{
    MStatus stat = MStatus::kSuccess;
    MFnMesh meshFn(mobject, &stat);
    CHECK_MSTATUS(stat);

    MPointArray points;
    MFloatVectorArray normals;
    MFloatArray uArray, vArray;
    MIntArray triPointIds, triNormalIds, triUvIds, triMatIds, perFaceAssignments;
    getMeshData(mobject, points, normals, uArray, vArray, triPointIds, triNormalIds, triUvIds, triMatIds, perFaceAssignments);

    Logging::debug(MString("Translating mesh object ") + meshFn.name().asChar());
    MString meshFullName = makeGoodString(meshFn.fullPathName());
    asf::auto_release_ptr<asr::MeshObject> mesh = asr::MeshObjectFactory::create(meshFullName.asChar(), asr::ParamArray());

    for (uint vtxId = 0; vtxId < points.length(); vtxId++)
    {
        mesh->push_vertex(mpointToGVector3(points[vtxId]));
    }

    for (uint nId = 0; nId < normals.length(); nId++)
    {
        mesh->push_vertex_normal(mpointToGVector3(normals[nId]));
    }

    for (uint tId = 0; tId < uArray.length(); tId++)
    {
        mesh->push_tex_coords(asr::GVector2((float)uArray[tId], (float)vArray[tId]));
    }

    int numTris = triPointIds.length() / 3;

    for (uint triId = 0; triId < numTris; triId++)
    {
        uint index = triId * 3;
        int perFaceShadingGroup = triMatIds[triId];
        int vtxId0 = triPointIds[index];
        int vtxId1 = triPointIds[index + 1];
        int vtxId2 = triPointIds[index + 2];
        int normalId0 = triNormalIds[index];
        int normalId1 = triNormalIds[index + 1];
        int normalId2 = triNormalIds[index + 2];
        int uvId0 = triUvIds[index];
        int uvId1 = triUvIds[index + 1];
        int uvId2 = triUvIds[index + 2];
        mesh->push_triangle(asr::Triangle(vtxId0, vtxId1, vtxId2, normalId0, normalId1, normalId2, uvId0, uvId1, uvId2, perFaceShadingGroup));
    }

    return mesh;
}
