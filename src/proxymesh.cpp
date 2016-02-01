
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

#include "proxymesh.h"
#include "utilities/tools.h"

#include <maya/MGlobal.h>

namespace asf = foundation;

ProxyMesh::ProxyMesh(float percentage)
{
    mPercentage = percentage;
    mPolySizeMultiplier = 1.0f/percentage * 0.5;
    mMin = MPoint(1000000.0,1000000.0,1000000.0);
    mMax = MPoint(-1000000.0,-1000000.0,-1000000.0);
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
    {
        faceCenter += mPoints[firstVertexIndex + faceVtx];
    }

    faceCenter = faceCenter / numVertices;

    for (size_t faceVtx = 0; faceVtx < numVertices; faceVtx++)
    {
        MPoint scaledPoint = ((mPoints[firstVertexIndex + faceVtx] - faceCenter) * mPolySizeMultiplier) + faceCenter;
        mPoints[firstVertexIndex + faceVtx] = scaledPoint;
    }
}

void ProxyMesh::addMesh(const asf::IMeshWalker& walker)
{
    mPoints.append(0,0,0); // bbox min
    mPoints.append(0,0,0); // bbox max both will be filled later

    size_t numFaces = walker.get_face_count();

    size_t numMaterials = walker.get_material_slot_count();
    size_t materialBaseId = this->mShadingGroupNames.length();

    mObjectShaderStartId.append(this->mShadingGroupNames.length());

    for (size_t matId = 0; matId < numMaterials; matId++)
        this->mShadingGroupNames.append(walker.get_material_slot(matId));

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
                asf::Vector3d vertex = walker.get_vertex(vtxId);
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

/*
ProxyMesh File Layout:
numshaders int
for every shader
    stringlen
    string
numfaces
for every face
    shaderid
numberOfPoints int
bboxmin point
bboxmax point
for every point
    points
*/
void ProxyMesh::writeFile(const MString& fileName)
{
    mProxyFile.open(fileName.asChar(),  std::ios_base::out | std::ios_base::binary);

    if (mProxyFile.good())
    {
        // numshaders
        this->write((int)this->mShadingGroupNames.length());
        // shadingGroup names
        MGlobal::displayInfo(MString("Object has ") + this->mShadingGroupNames.length() + " materials assigned");
        for (size_t shaderId = 0; shaderId < this->mShadingGroupNames.length(); shaderId++)
        {
            this->write(this->mShadingGroupNames[shaderId]);
            MGlobal::displayInfo(MString("Material ") + shaderId + " name: " + this->mShadingGroupNames[shaderId]);
        }
        // numfaces
        this->write((int)this->mPolyShaderIds.length());
        for (size_t pId = 0; pId < this->mPolyShaderIds.length(); pId++)
        {
            this->write(this->mPolyShaderIds[pId]);
        }
        MGlobal::displayInfo(MString("Written num polyShaderIds: ") + this->mPolyShaderIds.length() + " faces " + ((mPoints.length() - 2)/3));

        // the first two points contain the bounding box of the geometry
        // here we only write the number of triangle points.
        this->write((int)(mPoints.length() - 2));
        for (uint i = 0; i < mPoints.length(); i++)
            this->write(mPoints[i]);
        MGlobal::displayInfo(MString("Written ") + ((int)(mPoints.length() - 2)) + " points == " + ((mPoints.length() - 2)/3) + " triangles");
        mProxyFile.close();
    }
    else
    {
        MGlobal::displayError(MString("ProxyMesh::write: Could not open ") + fileName + " for output");
    }
}

