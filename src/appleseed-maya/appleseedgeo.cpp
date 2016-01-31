
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

#include "appleseed.h"
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPointArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include "mayascene.h"
#include "mtap_mayaobject.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "appleseedutils.h"
#include "world.h"
#include "definitions.h"

#include "renderer/modeling/object/meshobjectreader.h"

#define MTAP_MESH_STANDIN_ID 0x0011CF7B

#define MPointToAppleseed(pt) asr::GVector3((float)pt.x, (float)pt.y, (float)pt.z)

asf::auto_release_ptr<asr::MeshObject> AppleseedRenderer::defineStandardPlane(bool area)
{
    asf::auto_release_ptr<asr::MeshObject> object(asr::MeshObjectFactory::create("stdPlane", asr::ParamArray()));

    if (area)
    {
        // Vertices.
        object->push_vertex(asr::GVector3(-1.0f, -1.0f, 0.0f));
        object->push_vertex(asr::GVector3(-1.0f, 1.0f, 0.0f));
        object->push_vertex(asr::GVector3(1.0f, 1.0f, 0.0f));
        object->push_vertex(asr::GVector3(1.0f, -1.0f, 0.0f));
    }
    else
    {
        // Vertices.
        object->push_vertex(asr::GVector3(-1.0f, 0.0f, -1.0f));
        object->push_vertex(asr::GVector3(-1.0f, 0.0f, 1.0f));
        object->push_vertex(asr::GVector3(1.0f, 0.0f, 1.0f));
        object->push_vertex(asr::GVector3(1.0f, 0.0f, -1.0f));
    }
    // Vertex normals.
    if (area)
    {
        object->push_vertex_normal(asr::GVector3(0.0f, 0.0f, -1.0f));
    }
    else
    {
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

void AppleseedRenderer::createMesh(sharedPtr<mtap_MayaObject> obj)
{

    // If the mesh has an attribute called "mtap_standin_path" and it contains a valid entry, then try to read the
    // content from the path.
    // The other way is to have a standInMeshNode which is connected to the inMesh of the mesh node.
    // In this case, get the standin node, read the path of the binmesh file and load it.

    MStatus stat = MStatus::kSuccess;
    MFnMesh meshFn(obj->mobject, &stat);
    CHECK_MSTATUS(stat);

    MPointArray points;
    MFloatVectorArray normals;
    MFloatArray uArray, vArray;
    MIntArray triPointIds, triNormalIds, triUvIds, triMatIds;
    Logging::debug("defineMesh pre getMeshData");
    obj->getMeshData(points, normals, uArray, vArray, triPointIds, triNormalIds, triUvIds, triMatIds);

    Logging::debug(MString("Translating mesh object ") + meshFn.name().asChar());
    MString meshFullName = makeGoodString(meshFn.fullPathName());

    // Create a new mesh object.
    asf::auto_release_ptr<asr::MeshObject> mesh = asr::MeshObjectFactory::create(meshFullName.asChar(), asr::ParamArray());

    Logging::debug(MString("//object ") + meshFn.name());
    for (uint vtxId = 0; vtxId < points.length(); vtxId++)
    {
        mesh->push_vertex(MPointToAppleseed(points[vtxId]));
    }

    for (uint nId = 0; nId < normals.length(); nId++)
    {
        MVector n = normals[nId];
        MStatus s = n.normalize();
        if (n.length() < .3)
            n.y = .1;
        n.normalize();
        mesh->push_vertex_normal(MPointToAppleseed(n));
    }

    for (uint tId = 0; tId < uArray.length(); tId++)
    {
        mesh->push_tex_coords(asr::GVector2((float)uArray[tId], (float)vArray[tId]));
    }

    mesh->reserve_material_slots(obj->shadingGroups.length());
    for (uint sgId = 0; sgId < obj->shadingGroups.length(); sgId++)
    {
        MString slotName = MString("slot") + sgId;
        mesh->push_material_slot(slotName.asChar());
    }

    const uint numTris = triPointIds.length() / 3;

    for (uint triId = 0; triId < numTris; triId++)
    {
        const uint index = triId * 3;
        const int perFaceShadingGroup = triMatIds[triId];
        const int vtxId0 = triPointIds[index];
        const int vtxId1 = triPointIds[index + 1];
        const int vtxId2 = triPointIds[index + 2];
        const int normalId0 = triNormalIds[index];
        const int normalId1 = triNormalIds[index + 1];
        const int normalId2 = triNormalIds[index + 2];
        const int uvId0 = triUvIds[index];
        const int uvId1 = triUvIds[index + 1];
        const int uvId2 = triUvIds[index + 2];
        mesh->push_triangle(asr::Triangle(vtxId0, vtxId1, vtxId2,  normalId0, normalId1, normalId2, uvId0, uvId1, uvId2, perFaceShadingGroup));
    }

    MayaObject *assemblyObject = getAssemblyMayaObject(obj.get());
    asr::Assembly *ass = getCreateObjectAssembly(obj);

    Logging::debug(MString("Placing mesh ") + mesh->get_name() + " into assembly " + ass->get_name());

    asr::MeshObject *meshPtr = (asr::MeshObject *)ass->objects().get_by_name(meshFullName.asChar());

    if (meshPtr != 0)
    {
        Logging::debug(MString("Mesh object ") + meshFullName + " is already defined, removing...");
        ass->objects().remove(meshPtr);
        ass->bump_version_id();
    }

    ass->objects().insert(asf::auto_release_ptr<asr::Object>(mesh));
    meshPtr = (asr::MeshObject *)ass->objects().get_by_name(meshFullName.asChar());

    MString objectInstanceName = getObjectInstanceName(obj.get());

    MMatrix assemblyObjectMatrix = assemblyObject->dagPath.inclusiveMatrix();
    MMatrix objectMatrix = obj->dagPath.inclusiveMatrix();
    MMatrix diffMatrix = objectMatrix * assemblyObjectMatrix.inverse();
    asf::Matrix4d appleMatrix;
    MMatrixToAMatrix(diffMatrix, appleMatrix);

    asr::ParamArray objInstanceParamArray;
    addVisibilityFlags(obj, objInstanceParamArray);

    asr::ObjectInstance *oinst = ass->object_instances().get_by_name(objectInstanceName.asChar());
    if (oinst != 0)
    {
        Logging::debug(MString("Mesh object instance ") + objectInstanceName + " is already defined, removing...");
        ass->object_instances().remove(oinst);
        ass->bump_version_id();
    }
    ass->object_instances().insert(
        asr::ObjectInstanceFactory::create(
        objectInstanceName.asChar(),
        objInstanceParamArray,
        meshPtr->get_name(),
        asf::Transformd::from_local_to_parent(appleMatrix),
        asf::StringDictionary()
        .insert("slot0", "default")));


}

void AppleseedRenderer::updateGeometry(sharedPtr<MayaObject> mobj)
{
    sharedPtr<mtap_MayaObject> obj = staticPtrCast<mtap_MayaObject>(mobj);

    if (!obj->isObjVisible())
        return;

    if (!mobj->mobject.hasFn(MFn::kMesh))
        return;

    if (mobj->instanceNumber == 0)
    {
        createMesh(obj);
        defineMaterial(obj);
        return;
    }
}

void AppleseedRenderer::updateInstance(sharedPtr<MayaObject> mobj)
{
    sharedPtr<mtap_MayaObject> obj = staticPtrCast<mtap_MayaObject>(mobj);
    if (obj->dagPath.node().hasFn(MFn::kWorld))
        return;

    if (!obj->isObjVisible())
        return;

    if (mobj->instanceNumber > 0)
    {
        MayaObject *assemblyObject = getAssemblyMayaObject(obj.get());
        if (assemblyObject == 0)
        {
            Logging::debug("create mesh assemblyPtr == null");
            return;
        }
        MString assemblyName = getAssemblyName(assemblyObject);
        MString assemblyInstanceName = getAssemblyInstanceName(obj.get());

        asf::auto_release_ptr<asr::AssemblyInstance> assemblyInstance(
            asr::AssemblyInstanceFactory::create(
            assemblyInstanceName.asChar(),
            asr::ParamArray(),
            assemblyName.asChar()));
        asr::TransformSequence &ts = assemblyInstance->transform_sequence();
        fillMatrices(obj, ts);
        getMasterAssemblyFromProject(this->project.get())->assembly_instances().insert(assemblyInstance);
    }
}

void AppleseedRenderer::defineGeometry()
{
    sharedPtr<MayaScene> mayaScene = getWorldPtr()->worldScenePtr;
    sharedPtr<RenderGlobals> renderGlobals = getWorldPtr()->worldRenderGlobalsPtr;
    std::vector<sharedPtr<MayaObject> >::iterator oIt;
    for (oIt = mayaScene->objectList.begin(); oIt != mayaScene->objectList.end(); oIt++)
    {
        sharedPtr<MayaObject> mobj = *oIt;
        updateGeometry(mobj);
    }

    // create assembly instances
    for (oIt = mayaScene->objectList.begin(); oIt != mayaScene->objectList.end(); oIt++)
    {
        sharedPtr<MayaObject> mobj = *oIt;
        updateInstance(mobj);
    }

    for (oIt = mayaScene->instancerNodeElements.begin(); oIt != mayaScene->instancerNodeElements.end(); oIt++)
    {
        sharedPtr<MayaObject> mobj = *oIt;
        sharedPtr<mtap_MayaObject> obj = staticPtrCast<mtap_MayaObject>(mobj);
        if (obj->dagPath.node().hasFn(MFn::kWorld))
            continue;
        if (obj->instanceNumber == 0)
            continue;

        MayaObject *assemblyObject = getAssemblyMayaObject(obj.get());
        if (assemblyObject == 0)
        {
            Logging::debug("create mesh assemblyPtr == null");
            continue;
        }
        MString assemblyName = getAssemblyName(assemblyObject);
        MString assemblyInstanceName = getAssemblyInstanceName(obj.get());

        asf::auto_release_ptr<asr::AssemblyInstance> assemblyInstance(
            asr::AssemblyInstanceFactory::create(
            assemblyInstanceName.asChar(),
            asr::ParamArray(),
            assemblyName.asChar()));
        asr::TransformSequence &ts = assemblyInstance->transform_sequence();
        fillMatrices(obj, ts);
        getMasterAssemblyFromProject(this->project.get())->assembly_instances().insert(assemblyInstance);
    }
}
