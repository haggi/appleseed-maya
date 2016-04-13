
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

#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlugArray.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MFnMeshData.h>

#include "mayaobject.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "shadingtools/shadingutils.h"
#include "renderglobals.h"
#include "world.h"

ObjectAttributes::ObjectAttributes()
{
    needsOwnAssembly = false;
    objectMatrix.setToIdentity();
    assemblyObject = 0;
}

ObjectAttributes::ObjectAttributes(boost::shared_ptr<ObjectAttributes> other)
{
    this->hasInstancerConnection = false;
    objectMatrix.setToIdentity();
    assemblyObject = 0;

    if (other)
    {
        hasInstancerConnection = other->hasInstancerConnection;
        objectMatrix = other->objectMatrix;
        assemblyObject = other->assemblyObject;
    }
}

static std::vector<int> objectIdentifier; // plugids for detecting new objTypes

bool MayaObject::isInstanced()
{
    return dagPath.isInstanced() || (instanceNumber > 0) || ((attributes != 0) && attributes->hasInstancerConnection);
}

bool MayaObject::isCamera()
{
    if (mobject.hasFn(MFn::kCamera))
    {
        motionBlurred = true;
        return true;
    }
    return false;
}

bool MayaObject::isGeo()
{
    if (mobject.hasFn(MFn::kMesh))
        return true;
    if (mobject.hasFn(MFn::kNurbsSurface))
        return true;
    if (mobject.hasFn(MFn::kParticle))
        return true;
    if (mobject.hasFn(MFn::kSubSurface))
        return true;
    if (mobject.hasFn(MFn::kNurbsCurve))
        return true;
    if (mobject.hasFn(MFn::kHairSystem))
        return true;

    MFnDependencyNode depFn(mobject);
    uint nodeId = depFn.typeId().id();
    for (uint lId = 0; lId < objectIdentifier.size(); lId++)
    {
        if (nodeId == objectIdentifier[lId])
        {
            Logging::debug(MString("Found external geotype: ") + depFn.name());
            return true;
        }
    }

    return false;
}

bool MayaObject::isTransform()
{
    return mobject.hasFn(MFn::kTransform);
}

void MayaObject::getShadingGroups()
{
    // get shading groups only for allowed shapes

    if (geometryShapeSupported())
    {
        Logging::debug(MString("getShadingGroups::Supported geo ") + shortName);
        // only makes sense if we have a geometry shape.
        if (mobject.hasFn(MFn::kMesh) || mobject.hasFn(MFn::kNurbsSurface) || mobject.hasFn(MFn::kParticle) || mobject.hasFn(MFn::kNParticle))
        {
            getObjectShadingGroups(dagPath, perFaceAssignments, shadingGroups, true);
        }
    }
}

// check if the node or any of its parents has a animated visibility
bool MayaObject::isVisiblityAnimated()
{
    MStatus stat = MStatus::kSuccess;
    bool visibility = true;
    MDagPath dp = dagPath;
    while (stat == MStatus::kSuccess)
    {
        MFnDependencyNode depFn(dp.node());
        MPlug vplug = depFn.findPlug("visibility");
        if (vplug.isConnected())
        {
            Logging::debug(MString("Object: ") + vplug.name() + " has animated visibility");
            return true;
        }
        stat = dp.pop();
    }

    return false;
}

bool MayaObject::isObjVisible()
{
    MFnDagNode dagNode(mobject);
    if (!IsVisible(dagNode) || IsTemplated(dagNode) || !IsInRenderLayer(dagPath) || !IsPathVisible(dagPath) || !IsLayerVisible(dagPath))
        return false;
    return true;
}

bool MayaObject::geometryShapeSupported()
{
    if (this->mobject.hasFn(MFn::kMesh))
        return true;

    if (mobject.hasFn(MFn::kLight))
        return true;

    if (this->isCamera())
        return true;

    return false;
}

bool MayaObject::shadowMapCastingLight()
{
    if (!mobject.hasFn(MFn::kLight))
        return false;

    MFnDependencyNode lightFn(mobject);

    bool useDepthMapShadows = false;
    if (!getBool(MString("useDepthMapShadows"), lightFn,  useDepthMapShadows))
        return false;

    if (!useDepthMapShadows)
        return false;

    return true;
}

void MayaObject::updateObject()
{
    visible = isObjVisible();
}

void MayaObject::initialize()
{
    removed = false;
    isInstancerObject = false;
    instancerParticleId = -1;
    instanceNumber = 0;
    attributes.reset();
    origObject.reset();
    shortName = getObjectName(mobject);
    fullName = dagPath.fullPathName();
    fullNiceName = makeGoodString(fullName);
    transformMatrices.push_back(dagPath.inclusiveMatrix());
    instanceNumber = dagPath.instanceNumber();
    MFnDependencyNode depFn(mobject);
    motionBlurred = true;
    geometryMotionblur = false;
    bool mb = true;
    if (getBool(MString("motionBlur"), depFn, mb))
        motionBlurred = mb;
    // cameras have motionBlur attribute but it is set to false by default and it is not accessible via UI
    // but we want to have a blurred camera by default.
    if (mobject.hasFn(MFn::kCamera))
        motionBlurred = true;
    perObjectTransformSteps = 1;
    perObjectDeformSteps = 1;
    shapeConnected = false;
    lightExcludeList = true; // In most cases only a few lights are ignored, so the list is shorter with excluded lights
    shadowExcludeList = true; // in most cases only a few objects ignore shadows, so the list is shorter with ignoring objects
    animated = isObjAnimated();
    shapeConnected = isShapeConnected();
    parent.reset();
    visible = true;
    hasInstancerConnection = false;
    MDagPath dp;
    dp = dagPath;
    MFnDagNode dagNode(mobject);
    if (!IsVisible(dagNode) || IsTemplated(dagNode) || !IsInRenderLayer(dp) || !IsPathVisible(dp) || !IsLayerVisible(dagPath))
        visible = false;

    // get instancer connection
    MStatus stat;
    MPlug matrixPlug = depFn.findPlug(MString("matrix"), &stat);
    if (!stat)
        Logging::debug(MString("Could not find matrix plug"));
    else
    {
        MPlugArray outputs;
        if (matrixPlug.isConnected())
        {
            matrixPlug.connectedTo(outputs, false, true);
            for (uint i = 0; i < outputs.length(); i++)
            {
                MObject otherSide = outputs[i].node();
                Logging::debug(MString("matrix is connected to ") + getObjectName(otherSide));
                if (otherSide.hasFn(MFn::kInstancer))
                {
                    Logging::debug(MString("other side is instancer"));
                    hasInstancerConnection = true;
                }
            }
        }
    }

    if (mobject.hasFn(MFn::kWorld))
    {
        shortName = fullName = fullNiceName = "world";
    }
}

MayaObject::MayaObject(MObject& mobject)
{
    mobject = mobject;
    MFnDagNode dagNode(mobject);
    dagNode.getPath(dagPath);
    initialize();
}

MayaObject::MayaObject(MDagPath& objPath)
{
    mobject = objPath.node();
    dagPath = objPath;
    initialize();
}

// to check if an object is animated, we need to check e.g. its transform inputs
// if the object is from an instancer node, always return true
bool MayaObject::isObjAnimated()
{
    MStatus stat;
    bool returnValue = false;

    if (instancerParticleId > -1)
        return true;

    if (mobject.hasFn(MFn::kTransform))
    {
        MFnDependencyNode depFn(mobject, &stat);
        if (stat)
        {
            MPlugArray connections;
            depFn.getConnections(connections);
            if (connections.length() == 0)
                returnValue = false;
            else
            {
                for (uint cId = 0; cId < connections.length(); cId++)
                {
                    if (connections[cId].isDestination())
                    {
                        returnValue = true;
                    }
                }
            }

        }
    }
    return returnValue;
}

// to check if shape is connected, simply test for the common shape inputs, nurbs: create mesh: inMesh
bool MayaObject::isShapeConnected()
{
    MStatus stat;
    bool returnValue = false;
    MPlug inPlug;
    MFnDependencyNode depFn(mobject, &stat);
    if (stat)
    {
        MFn::Type type = mobject.apiType();
        switch (type)
        {
        case MFn::kMesh:
            inPlug = depFn.findPlug(MString("inMesh"), &stat);
            if (stat)
                if (inPlug.isConnected())
                    returnValue = true;
            break;
        case MFn::kNurbsSurface:
            inPlug = depFn.findPlug(MString("create"), &stat);
            if (stat)
                if (inPlug.isConnected())
                    returnValue = true;
            break;
        case MFn::kNurbsCurve:
            inPlug = depFn.findPlug(MString("create"), &stat);
            if (stat)
                if (inPlug.isConnected())
                    returnValue = true;
            break;
        }
    }

    return returnValue;
}

// if we have a mesh and it is a bifrost mesh it may contain velocity vertex data
bool MayaObject::hasBifrostVelocityChannel()
{
    MFnMesh meshFn(mobject);
    int numColorSets = meshFn.numColorSets();
    MStringArray colorSetNames;
    meshFn.getColorSetNames(colorSetNames);
    for (uint i = 0; i < colorSetNames.length(); i++)
    {
        if (colorSetNames[i] == "bifrostVelocity")
            return true;
    }
    return false;
}

// get the mesh informations from the current time and save it in the meshDataList
void MayaObject::addMeshData()
{
    MeshData mdata;

    if (hasBifrostVelocityChannel())
    {
        bool doMb = motionBlurred;
        boost::shared_ptr<RenderGlobals> renderGlobals = getWorldPtr()->mRenderGlobals;
        doMb = doMb && renderGlobals->doMb;

        Logging::debug(MString("Found bifrost velocity data for object: ") + shortName);
        if ((meshDataList.size() == 2) || !doMb)
        {
            Logging::debug("Bifrost mesh already has two motion steps or mb is turned off for it -> skipping");
            return;
        }

        getMeshData(mdata.points, mdata.normals);

        // if we have a bifrost velocity channel I suppose this is a bifost mesh and there is no need to
        // save the mesh motion steps because it has changing topology what does not work in most renderers
        // so we save only the very first motion step and derive the other steps from velocity channel.
        // and we simply produce two steps only because everything else would not make sense.

        MFnMesh meshFn(mobject);
        if (meshFn.hasColorChannels("bifrostVelocity"))
        {
            MColorArray colors;
            MString colorSetName = "bifrostVelocity";
            meshFn.getVertexColors(colors, &colorSetName);

            if (colors.length() == mdata.points.length())
            {
                for (uint ptId = 0; ptId < mdata.points.length(); ptId++)
                {
                    MColor c = colors[ptId];
                    MVector v = MVector(c.r, c.g, c.b);
                    mdata.points[ptId] -= v * 0.5 / 24.0;
                }
                meshDataList.push_back(mdata);
                for (uint ptId = 0; ptId < mdata.points.length(); ptId++)
                {
                    MColor c = colors[ptId];
                    MVector v = MVector(c.r, c.g, c.b);
                    mdata.points[ptId] += v * 0.5 / 24.0;
                }
                meshDataList.push_back(mdata);
            }
        }
        else
        {
            Logging::debug("Bifrost mesh has no velocity data, no motionblur.");
            if (meshDataList.size() == 0)
                meshDataList.push_back(mdata);
        }
    }
    else
    {
        getMeshData(mdata.points, mdata.normals);
        int np = mdata.points.length();
        meshDataList.push_back(mdata);

        std::vector<MeshData>::iterator mdIt;
        for (mdIt = meshDataList.begin(); mdIt != meshDataList.end(); mdIt++)
        {
            np = mdIt->points.length();
        }
    }
}

void MayaObject::getMeshData(MPointArray& points, MFloatVectorArray& normals)
{
    MStatus stat;
    MObject meshObject = mobject;
    MMeshSmoothOptions options;
    MFnMesh tmpMesh(mobject);
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
                    meshObject = smoothedObj;
                }
            }
        }
    }
    MFnMesh meshFn(meshObject, &stat);
    if (!stat)
    {
        MString error = stat.errorString();
        Logging::error(error);
    }
    meshFn.getPoints(points);
    meshFn.getNormals(normals, MSpace::kObject);
}

void MayaObject::getMeshData(MPointArray& points, MFloatVectorArray& normals, MFloatArray& uArray, MFloatArray& vArray, MIntArray& triPointIndices, MIntArray& triNormalIndices, MIntArray& triUvIndices, MIntArray& triMatIndices)
{
    MStatus stat;
    MObject meshObject = mobject;
    MMeshSmoothOptions options;
    MFnMesh tmpMesh(mobject, &stat);

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
                    meshObject = smoothedObj;
                }
            }
        }
    }

    MFnMesh meshFn(meshObject, &stat);
    CHECK_MSTATUS(stat);
    MItMeshPolygon faceIt(meshObject, &stat);
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
        Logging::warning(MString("Object has no uv's: ") + shortName);
        uArray.append(0.0);
        vArray.append(0.0);
    }
    for (uint nid = 0; nid < numNormals; nid++)
    {
        if (normals[nid].length() < 0.1f)
            Logging::warning(MString("Malformed normal in ") + shortName);
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

//  The purpose of this method is to compare object attributes and inherit them if appropriate.
//  e.g. lets say we assign a color to the top node of a hierarchy. Then all child nodes will be
//  called and this method is used.
boost::shared_ptr<ObjectAttributes> MayaObject::getObjectAttributes(boost::shared_ptr<ObjectAttributes> parentAttributes)
{
    boost::shared_ptr<ObjectAttributes> myAttributes(new ObjectAttributes(parentAttributes));

    if (this->hasInstancerConnection)
    {
        myAttributes->hasInstancerConnection = true;
    }

    if (this->isTransform())
    {
        MFnDagNode objNode(this->mobject);
        myAttributes->objectMatrix = objNode.transformationMatrix() * myAttributes->objectMatrix;
    }

    if (this->needsAssembly() || myAttributes->hasInstancerConnection)
    {
        myAttributes->needsOwnAssembly = true;
        myAttributes->assemblyObject = this;
        myAttributes->objectMatrix.setToIdentity();
    }

    attributes = myAttributes;

    return myAttributes;
}

// objects needs own assembly if:
//      - it is instanced
//      - it is an animated transform
//      - its polysize is large (not yet implemented)
bool MayaObject::needsAssembly()
{
    // Normally only a few nodes would need a own assembly.
    // In IPR we have an update problem: If in a hierarchy a transform node is manipulated,
    // there is no way to find out that a geometry node below has to be updated, at least I don't know any.
    // Maybe I have to parse the hierarchy below and check the nodes for a geometry/camera/light node.
    // So at the moment I let all transform nodes receive their own transforms. This will result in a
    // translation of the complete hierarchy as assemblies/assembly instances.
    if (getWorldPtr()->getRenderType() == World::IPRRENDER)
    {
        if (this->isTransform())
        {
            return true;
        }
    }

    // this is the root of all assemblies
    if (this->mobject.hasFn(MFn::kWorld))
        return true;

    if (this->instanceNumber > 0)
        return false;

    if (this->hasInstancerConnection)
    {
        Logging::debug(MString("obj has instancer connection -> needs assembly."));
        return true;
    }

    if (this->isInstanced())
    {
        Logging::debug(MString("obj has more than 1 parent -> needs assembly."));
        return true;
    }

    if (this->isObjAnimated())
    {
        Logging::debug(MString("Object is animated -> needs assembly."));
        return true;
    }

    if (isLightTransform(this->dagPath))
    {
        Logging::debug(MString("Object is light transform -> needs assembly."));
        return true;
    }

    return false;
}
