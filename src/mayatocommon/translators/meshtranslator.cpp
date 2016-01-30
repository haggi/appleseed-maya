
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

#include "meshtranslator.h"
#include "maya/mfnmesh.h"
#include "maya/mitmeshpolygon.h"
#include "utilities/logging.h"

static Logging logger;

MeshTranslator::MeshTranslator()
{}

MeshTranslator::~MeshTranslator()
{}

MeshTranslator::MeshTranslator(MObject meshMObject)
{
    MStatus stat = MStatus::kSuccess;
    this->meshObject = meshMObject;
    this->good = false;

    MFnMesh meshFn(this->meshObject, &stat);
    CHECK_MSTATUS_AND_RETURNONLY(stat);
    MItMeshPolygon faceIt(meshObject, &stat);
    CHECK_MSTATUS_AND_RETURNONLY(stat);

    meshFn.getPoints(this->vertices);
    meshFn.getNormals( this->normals, MSpace::kWorld );


    //// Triangles.
    MPointArray triPoints;
    MIntArray triVtxIds;
    MIntArray faceVtxIds;
    MIntArray faceNormalIds;

    for(faceIt.reset(); !faceIt.isDone(); faceIt.next())
    {
        int faceId = faceIt.index();
        int numTris;
        faceIt.numTriangles(numTris);

        faceIt.getTriangles(triPoints, triVtxIds);
        faceIt.getVertices(faceVtxIds);
        faceNormalIds.clear();
        for (int vtxId = 0; vtxId < faceVtxIds.length(); vtxId++)
        {
            faceNormalIds.append(faceIt.normalIndex(vtxId));
        }

        for (int triId = 0; triId < numTris; triId++)
        {
            int faceRelIds[3];

            for (uint triVtxId = 0; triVtxId < 3; triVtxId++)
            {
                for(uint faceVtxId = 0; faceVtxId < triVtxIds.length(); faceVtxId++)
                {
                    if (triVtxIds[faceVtxId] == triVtxIds[triVtxId])
                    {
                        faceRelIds[triVtxId] = faceVtxId;
                    }
                }
            }

            Triangle t;
            t.vtxIds[0] = faceVtxIds[faceRelIds[0]];
            t.vtxIds[1] = faceVtxIds[faceRelIds[1]];
            t.vtxIds[2] = faceVtxIds[faceRelIds[2]];
            t.normalIds[0] = faceNormalIds[faceRelIds[0]];
            t.normalIds[1] = faceNormalIds[faceRelIds[1]];
            t.normalIds[2] = faceNormalIds[faceRelIds[2]];

            this->triangleArray.push_back(t);
        }

    }
    this->good = true;
}

bool MeshTranslator::isGood()
{
    return this->good;
}
