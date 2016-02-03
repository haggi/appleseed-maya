
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

#ifndef APPLESEED_MAYA_MESH_WALKER_H
#define APPLESEED_MAYA_MESH_WALKER_H

// appleseed-foundation headers.
#include "foundation/mesh/imeshwalker.h"

// Maya headers.
#include <maya/MDagPath.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPointArray.h>

// Standard headers.
#include <string.h>
#include <vector>

struct Face
{
    MIntArray vtxIds;
    MIntArray normalIds;
    MIntArray uvIds;
};

class MeshWalker
  : public foundation::IMeshWalker
{
  public:
    explicit MeshWalker(const MDagPath& dagPath);

    // Return the name of the mesh.
    virtual const char* get_name() const;

    // Return vertices.
    virtual size_t get_vertex_count() const;
    virtual foundation::Vector3d get_vertex(const size_t i) const;

    // Return vertex normals.
    virtual size_t get_vertex_normal_count() const;
    virtual foundation::Vector3d get_vertex_normal(const size_t i) const;

    // Return texture coordinates.
    virtual size_t get_tex_coords_count() const;
    virtual foundation::Vector2d get_tex_coords(const size_t i) const;

    // Return material slots.
    virtual size_t get_material_slot_count() const;
    virtual const char* get_material_slot(const size_t i) const;

    // Return the number of faces.
    virtual size_t get_face_count() const;

    // Return the number of vertices in a given face.
    virtual size_t get_face_vertex_count(const size_t face_index) const;

    // Return data for a given vertex of a given face.
    virtual size_t get_face_vertex(const size_t face_index, const size_t vertex_index) const;
    virtual size_t get_face_vertex_normal(const size_t face_index, const size_t vertex_index) const;
    virtual size_t get_face_tex_coords(const size_t face_index, const size_t vertex_index) const;

    // Return the material assigned to a given face.
    virtual size_t get_face_material(const size_t face_index) const;

    void setTransform();

  private:
    MFnMesh             mMeshFn;
    MDagPath            mMeshDagPath;
    MObject             mMeshObject;
    MFnMeshData         mSmoothMeshData;

    // mesh data
    MFloatArray         mU,mV;
    MPointArray         mPoints;
    MFloatVectorArray   mNormals;

    MObjectArray        mShadingGroups;
    MIntArray           mPerFaceAssignments;
    MIntArray           mPerTriangleAssignments;
    std::vector<Face>   mFaceList;
    bool                mUseSmoothMesh;

    MObject checkSmoothMesh();
};

#endif
