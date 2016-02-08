
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
#include "proxymesh.h"

// appleseed-maya headers.
#include "utilities/tools.h"

// Maya headers.
#include <maya/MGlobal.h>

ProxyMesh::ProxyMesh(const float percentage)
  : mPercentage(percentage)
  , mPolySizeMultiplier(1.0f / percentage * 0.5f)
  , mMin(1000000.0, 1000000.0, 1000000.0)
  , mMax(-1000000.0, -1000000.0, -1000000.0)
{
}

void ProxyMesh::addMesh(const foundation::IMeshWalker& walker)
{
    mPoints.append(0,0,0); // bbox min
    mPoints.append(0,0,0); // bbox max both will be filled later

    size_t numFaces = walker.get_face_count();

    size_t numMaterials = walker.get_material_slot_count();
    size_t materialBaseId = mShadingGroupNames.length();

    for (size_t matId = 0; matId < numMaterials; matId++)
        mShadingGroupNames.append(walker.get_material_slot(matId));

    for (size_t faceId = 0; faceId < numFaces; faceId++)
    {
        float r = rnd();
        if (r < mPercentage)
        {
            size_t firstFaceVertex = mPoints.length();
            size_t numVertices = walker.get_face_vertex_count(faceId); // at the moment its always 3
            for (size_t faceVtx = 0; faceVtx < numVertices; faceVtx++)
            {
                size_t vtxId = walker.get_face_vertex(faceId, faceVtx);
                foundation::Vector3d vertex = walker.get_vertex(vtxId);
                MPoint vtx(vertex.x, vertex.y, vertex.z);
                mPoints.append(vtx);
                setBBox(vtx);
            }

            size_t matId = walker.get_face_material(faceId) + materialBaseId;
            mPolyShaderIds.append(matId);

            scaleFace(firstFaceVertex, numVertices);
        }
    }

    mPoints[0] = mMin;
    mPoints[1] = mMax;
}

void ProxyMesh::writeFile(const MString& fileName)
{
    //
    // ProxyMesh File Layout:
    //
    //   number of shading groups (int)
    //   for every shading group:
    //       length of the shading group name (int)
    //       shading group name
    //   number of faces (int)
    //   for every face:
    //       shader id (int)
    //   number of points (int)
    //   bbox min point
    //   bbox max point
    //   points
    //

    mProxyFile.open(fileName.asChar(), std::ios_base::out | std::ios_base::binary);

    if (!mProxyFile.good())
    {
        MGlobal::displayError(MString("ProxyMesh::writeFile: Could not open ") + fileName + " for output");
        return;
    }

    // Number of shading groups.
    write(static_cast<int>(mShadingGroupNames.length()));

    // Shading group names.
    for (unsigned int i = 0; i < mShadingGroupNames.length(); i++)
        write(mShadingGroupNames[i]);

    // Number of faces.
    write(static_cast<int>(mPolyShaderIds.length()));
    for (unsigned int i = 0; i < mPolyShaderIds.length(); i++)
        write(mPolyShaderIds[i]);

    // The first two points contain the bounding box of the geometry.
    // Here we only write the number of triangle points.
    write(static_cast<int>(mPoints.length() - 2));
    for (unsigned int i = 0; i < mPoints.length(); i++)
        write(mPoints[i]);

    mProxyFile.close();
}

void ProxyMesh::write(const int value)
{
    mProxyFile.write(reinterpret_cast<const char*>(&value), sizeof(int));
}

void ProxyMesh::write(const double value)
{
    mProxyFile.write(reinterpret_cast<const char*>(&value), sizeof(double));
}

void ProxyMesh::write(const MPoint& point)
{
    write(point.x);
    write(point.y);
    write(point.z);
}

void ProxyMesh::write(const MPointArray& points)
{
    for (unsigned int i = 0, e = points.length(); i < e; ++i)
        write(points[i]);
}

void ProxyMesh::write(const MString& value)
{
    write(static_cast<int>(value.length()));
    mProxyFile.write(value.asChar(), value.length());
}

void ProxyMesh::setMin(const MPoint& vtx)
{
    if (vtx.x < mMin.x)
        mMin.x = vtx.x;

    if (vtx.y < mMin.y)
        mMin.y = vtx.y;

    if (vtx.z < mMin.z)
        mMin.z = vtx.z;
}

void ProxyMesh::setMax(const MPoint& vtx)
{
    if (vtx.x > mMax.x)
        mMax.x = vtx.x;

    if (vtx.y > mMax.y)
        mMax.y = vtx.y;

    if (vtx.z > mMax.z)
        mMax.z = vtx.z;
}

void ProxyMesh::setBBox(const MPoint& vtx)
{
    setMin(vtx);
    setMax(vtx);
}

void ProxyMesh::scaleFace(int firstVertexIndex, int numVertices)
{
    MPoint faceCenter;
    for (size_t faceVtx = 0; faceVtx < numVertices; faceVtx++)
        faceCenter += mPoints[firstVertexIndex + faceVtx];

    faceCenter = faceCenter / numVertices;

    for (size_t faceVtx = 0; faceVtx < numVertices; faceVtx++)
    {
        MPoint scaledPoint = ((mPoints[firstVertexIndex + faceVtx] - faceCenter) * mPolySizeMultiplier) + faceCenter;
        mPoints[firstVertexIndex + faceVtx] = scaledPoint;
    }
}
