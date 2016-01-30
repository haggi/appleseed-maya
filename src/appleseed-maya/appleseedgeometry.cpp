
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

#include "appleseedgeometry.h"
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPointArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>

#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "utilities/meshtools.h"
#include "shadingtools/shadingutils.h"

namespace asf = foundation;
namespace asr = renderer;

#define MPointToAppleseed(pt) asr::GVector3((float)pt.x, (float)pt.y, (float)pt.z)

namespace MTAP_GEOMETRY
{
    asf::auto_release_ptr<asr::MeshObject> defineStandardPlane(bool area)
    {
        asf::auto_release_ptr<asr::MeshObject> object(asr::MeshObjectFactory::create("standardPlane", asr::ParamArray()));

        if (area)
        {
            // Vertices.
            object->push_vertex(asr::GVector3(-1.0f, -1.0f, 0.0f));
            object->push_vertex(asr::GVector3(-1.0f, 1.0f, 0.0f));
            object->push_vertex(asr::GVector3(1.0f, 1.0f, 0.0f));
            object->push_vertex(asr::GVector3(1.0f, -1.0f, 0.0f));
        }
        else{
            // Vertices.
            object->push_vertex(asr::GVector3(-1.0f, 0.0f, -1.0f));
            object->push_vertex(asr::GVector3(-1.0f, 0.0f, 1.0f));
            object->push_vertex(asr::GVector3(1.0f, 0.0f, 1.0f));
            object->push_vertex(asr::GVector3(1.0f, 0.0f, -1.0f));
        }

        if (area)
        {
            object->push_vertex_normal(asr::GVector3(0.0f, 0.0f, -1.0f));
        }
        else{
            object->push_vertex_normal(asr::GVector3(0.0f, 1.0f, 0.0f));
        }

        object->push_tex_coords(asr::GVector2(0.0, 0.0));
        object->push_tex_coords(asr::GVector2(1.0, 0.0));
        object->push_tex_coords(asr::GVector2(1.0, 1.0));
        object->push_tex_coords(asr::GVector2(0.0, 1.0));

        // Triangles.
        object->push_triangle(asr::Triangle(0, 1, 2, 0, 0, 0, 0, 1, 2, 0));
        object->push_triangle(asr::Triangle(0, 2, 3, 0, 0, 0, 0, 2, 3, 0));

        return object;

    }

    asf::auto_release_ptr<asr::MeshObject> createMesh(MObject& mobject)
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
            mesh->push_vertex(MPointToAppleseed(points[vtxId]));
        }

        for (uint nId = 0; nId < normals.length(); nId++)
        {
            mesh->push_vertex_normal(MPointToAppleseed(normals[nId]));
        }

        for (uint tId = 0; tId < uArray.length(); tId++)
        {
            mesh->push_tex_coords(asr::GVector2((float)uArray[tId], (float)vArray[tId]));
        }

        const unsigned int numTris = triPointIds.length() / 3;

        for (unsigned int triId = 0; triId < numTris; triId++)
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
}
