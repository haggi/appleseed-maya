
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
#include "appleseedutils.h"

// appleseed-maya headers.
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "utilities/pystring.h"
#include "appleseedrenderer.h"
#include "mayaobject.h"
#include "renderglobals.h"
#include "world.h"

// appleseed.renderer headers.
#include "renderer/api/color.h"
#include "renderer/api/material.h"
#include "renderer/api/surfaceshader.h"
#include "renderer/api/bsdf.h"
#include "renderer/api/texture.h"

void defineDefaultMaterial(renderer::Project *project)
{
    renderer::Assembly *assembly = getMasterAssemblyFromProject(project);
    MColor gray(0.5f, 0.5f, 0.5f);
    defineColor(project, "gray", gray, 1.0f);

    // Create a BRDF called "diffuse_gray_brdf" and insert it into the assembly.
    assembly->bsdfs().insert(
        renderer::LambertianBRDFFactory().create(
        "diffuse_gray_brdf",
        renderer::ParamArray()
        .insert("reflectance", "gray")));

    // Create a physical surface shader and insert it into the assembly.
    assembly->surface_shaders().insert(
        renderer::PhysicalSurfaceShaderFactory().create(
        "physical_surface_shader",
        renderer::ParamArray()));

    // Create a material called "gray_material" and insert it into the assembly.
    assembly->materials().insert(
        renderer::GenericMaterialFactory().create(
        "default",
        renderer::ParamArray()
        .insert("surface_shader", "physical_surface_shader")
        .insert("bsdf", "diffuse_gray_brdf")));
}

MString getAssemblyInstanceName(MayaObject *obj)
{
    MString assemblyName = getAssemblyName(obj);
    if (obj->instancerParticleId > -1)
        assemblyName = assemblyName + "_" + obj->instancerParticleId;
    if (obj->instanceNumber > 0)
        assemblyName = assemblyName + "_" + obj->instanceNumber;

    return assemblyName + "_assInst";
}

MString getObjectInstanceName(MayaObject *obj)
{
    return obj->fullNiceName + "_objInst";
}

MString getObjectName(MayaObject* obj)
{
    return obj->dagPath.fullPathName();
}

MString getAssemblyName(MayaObject *obj)
{
    // if an obj is an instanced object, get the assembly name of the original object.
    if ((obj->instanceNumber > 0) || (obj->instancerParticleId > -1))
    {
        if (obj->origObject)
        {
            return getAssemblyName(obj->origObject.get());
        }
        else
        {
            Logging::error(MString("Object is instanced but has no orig object --> Problem: ") + obj->fullName);
            return "";
        }
    }

    // We always use the transform name as assemblyName, if we have a shape node,
    // go up one element to the transform node.
    MDagPath path = obj->dagPath;
    MStatus stat;
    while (!path.node().hasFn(MFn::kTransform) && !path.node().hasFn(MFn::kWorld))
    {
        MFn::Type pt = path.apiType();
        MFn::Type nt = path.node().apiType();
        stat = path.pop();
        if (!stat)
            break;
    }
    if (path.node().hasFn(MFn::kWorld))
        return "world";
    else
        return path.fullPathName() + "_ass";
}

void defineScene(renderer::Project *project)
{
    renderer::Scene *scenePtr = project->get_scene();
    if (scenePtr == 0)
    {
        foundation::auto_release_ptr<renderer::Scene> scene(renderer::SceneFactory::create());
        project->set_scene(scene);
    }
}

renderer::Scene *getSceneFromProject(renderer::Project *project)
{
    defineScene(project);
    return project->get_scene();
}

// we first define an assembly which contains the world assembly. This "uberMaster" contains the global transformation.
void defineMasterAssembly(renderer::Project *project)
{
    MMatrix conversionMatrix;
    conversionMatrix.setToIdentity();
    World *world = getWorldPtr();
    if (world != 0)
    {
        RenderGlobals *rg = world->mRenderGlobals.get();
        if (rg != 0)
            conversionMatrix = rg->globalConversionMatrix;
    }
    if (getSceneFromProject(project)->assemblies().get_by_name("world") == 0)
    {
        foundation::auto_release_ptr<renderer::Assembly> assembly(renderer::AssemblyFactory().create("world", renderer::ParamArray()));
        getSceneFromProject(project)->assemblies().insert(assembly);
        foundation::Matrix4d appMatrix;
        MMatrix transformMatrix;
        transformMatrix.setToIdentity();
        transformMatrix *= conversionMatrix;
        foundation::auto_release_ptr<renderer::AssemblyInstance> assemblyInstance = renderer::AssemblyInstanceFactory::create("world_Inst", renderer::ParamArray(), "world");
        MMatrixToAMatrix(transformMatrix, appMatrix);
        assemblyInstance->transform_sequence().set_transform(0.0, foundation::Transformd::from_local_to_parent(appMatrix));
        getSceneFromProject(project)->assembly_instances().insert(assemblyInstance);
    }
}

renderer::Assembly* getMasterAssemblyFromProject(renderer::Project* project)
{
    defineMasterAssembly(project);
    return getSceneFromProject(project)->assemblies().get_by_name("world");
}

// return the maya object above which has it's own assembly
MayaObject* getAssemblyMayaObject(MayaObject* mobj)
{
    MayaObject *obj = mobj;
    if (obj->instanceNumber > 0)
        obj = mobj->origObject.get();

    if (obj->attributes)
    {
        return obj->attributes->assemblyObject;
    }
    return 0; // only happens if obj is world
}

renderer::Assembly* getCreateObjectAssembly(boost::shared_ptr<MayaObject> obj)
{
    MayaObject* assemblyObject = getAssemblyMayaObject(obj.get());
    if (assemblyObject == 0)
        return 0;

    MString assemblyName = getAssemblyName(assemblyObject);
    MString assemblyInstanceName = getAssemblyInstanceName(assemblyObject);

    boost::shared_ptr<AppleseedRenderer> appleRenderer = boost::static_pointer_cast<AppleseedRenderer>(getWorldPtr()->mRenderer);
    renderer::Assembly* master = getMasterAssemblyFromProject(appleRenderer->getProjectPtr());
    if (obj->mobject.hasFn(MFn::kLight) && !obj->mobject.hasFn(MFn::kAreaLight))
        return master;

    renderer::Assembly* ass = master->assemblies().get_by_name(assemblyName.asChar());
    if (assemblyName == "world")
        ass = master;

    if (ass == 0)
    {
        foundation::auto_release_ptr<renderer::Assembly> assembly(
            renderer::AssemblyFactory().create(assemblyName.asChar(), renderer::ParamArray()));
        master->assemblies().insert(assembly);
        ass = master->assemblies().get_by_name(assemblyName.asChar());

        foundation::auto_release_ptr<renderer::AssemblyInstance> assInst(
            renderer::AssemblyInstanceFactory().create(assemblyInstanceName.asChar(), renderer::ParamArray(), assemblyName.asChar()));

        fillMatrices(obj, assInst->transform_sequence());
        master->assembly_instances().insert(assInst);
    }

    return ass;
}

renderer::AssemblyInstance *getExistingObjectAssemblyInstance(MayaObject *obj)
{
    boost::shared_ptr<AppleseedRenderer> appleRenderer = boost::static_pointer_cast<AppleseedRenderer>(getWorldPtr()->mRenderer);
    MayaObject *assemblyObject = getAssemblyMayaObject(obj);
    if (assemblyObject == 0)
    {
        Logging::debug("create mesh assemblyPtr == null");
        return 0;
    }
    MString assemblyName = getAssemblyName(obj);
    MString assemblyInstanceName = getAssemblyInstanceName(obj);
    renderer::Assembly *ass = getMasterAssemblyFromProject(appleRenderer->getProjectPtr());
    if (assemblyName == "world")
        ass = getMasterAssemblyFromProject(appleRenderer->getProjectPtr());

    if (ass == 0)
        return 0;
    return ass->assembly_instances().get_by_name(assemblyInstanceName.asChar());
}

void defineColor(renderer::Project *project, const char *name, MColor color, float intensity, MString colorSpace)
{
    renderer::Scene* scene = project->get_scene();

    renderer::ColorEntity* previous = scene->colors().get_by_name(name);
    if (previous != 0)
        scene->colors().remove(previous);

    scene->colors().insert(
        renderer::ColorEntityFactory::create(
            name,
            renderer::ParamArray()
                .insert("color_space", colorSpace)
                .insert("multiplier", intensity),
            renderer::ColorValueArray(3, &color.r),
            renderer::ColorValueArray(1, &color.a)));
}

// check if a texture is connected to this attribute. If yes, return the texture name, if not define the currentColor and return the color name.
MString colorOrMap(renderer::Project *project, MFnDependencyNode& shaderNode, MString& attributeName)
{
    MString definition = defineTexture(shaderNode, attributeName);
    if (definition.length() == 0)
    {
        MColor color = getColorAttr(attributeName.asChar(), shaderNode);
        defineColor(project, (attributeName + "_color").asChar(), color, 1.0f);
        definition = attributeName + "_color";
    }
    return definition;
}

void removeTextureEntityIfItExists(MString& textureName)
{
    boost::shared_ptr<AppleseedRenderer> appleRenderer = boost::static_pointer_cast<AppleseedRenderer>(getWorldPtr()->mRenderer);
    assert(appleRenderer != 0);
    renderer::Scene* scene = getSceneFromProject(appleRenderer->getProjectPtr());

    const MString textureInstanceName = textureName + "_texInst";
    renderer::Texture* texture = scene->textures().get_by_name(textureName.asChar());
    if (texture != 0)
        scene->textures().remove(texture);

    renderer::TextureInstance* textureInstance = scene->texture_instances().get_by_name(textureInstanceName.asChar());
    if (textureInstance != 0)
        scene->texture_instances().remove(textureInstance);
}

MString defineTexture(MFnDependencyNode& shader, MString& attributeName)
{
    boost::shared_ptr<AppleseedRenderer> appleRenderer = boost::static_pointer_cast<AppleseedRenderer>(getWorldPtr()->mRenderer);
    assert(appleRenderer != 0);
    renderer::Scene *scene = getSceneFromProject(appleRenderer->getProjectPtr());
    foundation::SearchPaths &searchPaths = appleRenderer->getProjectPtr()->search_paths();

    MStatus stat;
    MString textureDefinition("");
    MPlug plug = shader.findPlug(attributeName, &stat);
    if (stat != MStatus::kSuccess)
        return textureDefinition;
    if (!plug.isConnected())
        return textureDefinition;
    MObject connectedNode = getConnectedInNode(plug);

    if (!connectedNode.hasFn(MFn::kFileTexture))
        return textureDefinition;

    MFnDependencyNode fileTextureNode(connectedNode, &stat);
    MString textureName = fileTextureNode.name() + "_texture";
    MString fileTextureName = "";
    getString(MString("fileTextureName"), fileTextureNode, fileTextureName);
    if (!pystring::endswith(fileTextureName.asChar(), ".exr") || (fileTextureName.length() == 0))
    {
        if (fileTextureName.length() == 0)
            Logging::warning(MString("FileTextureName has no content."));
        else
            Logging::warning(MString("FileTextureName does not have an .exr extension. Other filetypes are not yet supported, sorry."));
        return textureDefinition;
    }

    removeTextureEntityIfItExists(textureName);

    MString colorProfile = "srgb";

    renderer::ParamArray params;
    Logging::debug(MString("Now inserting file name: ") + fileTextureName);
    params.insert("filename", fileTextureName.asChar());      // OpenEXR only for now. The param is called filename but it can be a path
    params.insert("color_space", colorProfile.asChar());

    foundation::auto_release_ptr<renderer::Texture> textureElement(
        renderer::DiskTexture2dFactory().create(
        textureName.asChar(),
            params,
            searchPaths));    // the project holds a set of search paths to find textures and other assets
    scene->textures().insert(textureElement);

    bool alphaIsLuminance = false;
    getBool(MString("alphaIsLuminance"), fileTextureNode, alphaIsLuminance);
    renderer::ParamArray tInstParams;
    tInstParams.insert("addressing_mode", "clamp");
    tInstParams.insert("filtering_mode", "bilinear");
    if (alphaIsLuminance)
        tInstParams.insert("alpha_mode", "luminance");

    MString textureInstanceName = textureName + "_texInst";
    foundation::auto_release_ptr<renderer::TextureInstance> tinst = renderer::TextureInstanceFactory().create(
       textureInstanceName.asChar(),
       tInstParams,
       textureName.asChar());

    scene->texture_instances().insert(tinst);

    return textureInstanceName;
}

void fillTransformMatrices(MMatrix matrix, renderer::AssemblyInstance *assInstance)
{
    assInstance->transform_sequence().clear();
    foundation::Matrix4d appMatrix;
    MMatrix colMatrix = matrix;
    MMatrixToAMatrix(colMatrix, appMatrix);
    assInstance->transform_sequence().set_transform(
        0.0f,
        foundation::Transformd::from_local_to_parent(appMatrix));
}

void fillMatrices(boost::shared_ptr<MayaObject> obj, renderer::TransformSequence& transformSequence)
{
    MMatrix conversionMatrix = getWorldPtr()->mRenderGlobals->globalConversionMatrix;
    float scaleFactor = getWorldPtr()->mRenderGlobals->scaleFactor;
    transformSequence.clear();

    // in ipr mode we have to update the matrix manually
    if (getWorldPtr()->getRenderType() == World::IPRRENDER)
    {
        obj->transformMatrices.clear();
        obj->transformMatrices.push_back(obj->dagPath.inclusiveMatrix());
    }
    size_t numSteps = obj->transformMatrices.size();
    size_t divSteps = numSteps;
    if (divSteps > 1)
        divSteps -= 1;
    float stepSize = 1.0f / (float)divSteps;
    float start = 0.0f;

    // cameras pos has to be scaled because it is not placed into the world assembly but directly in the scene.
    // We only scale the transform because a scaled camera will result in different renderings (e.g. dof)
    foundation::Matrix4d appMatrix;
    MMatrix transformMatrix;
    for (size_t matrixId = 0; matrixId < numSteps; matrixId++)
    {
        MMatrix colMatrix = obj->transformMatrices[matrixId];
        if (obj->mobject.hasFn(MFn::kCamera))
        {
            colMatrix.matrix[3][0] *= scaleFactor;
            colMatrix.matrix[3][1] *= scaleFactor;
            colMatrix.matrix[3][2] *= scaleFactor;
        }
        MMatrixToAMatrix(colMatrix, appMatrix);
        transformSequence.set_transform(
            start + stepSize * matrixId,
            foundation::Transformd::from_local_to_parent(appMatrix));
    }
}

void fillTransformMatrices(boost::shared_ptr<MayaObject> obj, renderer::Light *light)
{
    // in ipr mode we have to update the matrix manually
    if (getWorldPtr()->getRenderType() == World::IPRRENDER)
    {
        obj->transformMatrices.clear();
        obj->transformMatrices.push_back(obj->dagPath.inclusiveMatrix());
    }
    foundation::Matrix4d appMatrix;
    MMatrix colMatrix = obj->transformMatrices[0];
    MMatrixToAMatrix(colMatrix, appMatrix);
    light->set_transform(foundation::Transformd::from_local_to_parent(appMatrix));
}

void MMatrixToAMatrix(MMatrix& mayaMatrix, foundation::Matrix4d& appleMatrix)
{
    MMatrix rowMatrix = mayaMatrix.transpose();
    for (int i = 0; i < 4; i++)
        for (int k = 0; k < 4; k++)
            appleMatrix[i * 4 + k] = rowMatrix[i][k];
}

void addVisibilityFlags(boost::shared_ptr<MayaObject> obj, renderer::ParamArray& paramArray)
{
    MFnDependencyNode depFn(obj->mobject);

    if (obj->mobject.hasFn(MFn::kMesh) || obj->mobject.hasFn(MFn::kAreaLight))
    {
        if (!getBoolAttr("primaryVisibility", depFn, true))
            paramArray.insert_path("visibility.camera", false);

        if (!getBoolAttr("castsShadows", depFn, true))
            paramArray.insert_path("visibility.shadow", false);

        if (!getBoolAttr("visibleInRefractions", depFn, true))
            paramArray.insert_path("visibility.transparency", false);

        if (!getBoolAttr("mtap_visibleLights", depFn, true))
            paramArray.insert_path("visibility.light", false);

        if (!getBoolAttr("mtap_visibleProbe", depFn, true))
            paramArray.insert_path("visibility.probe", false);

        if (!getBoolAttr("mtap_visibleGlossy", depFn, true))
            paramArray.insert_path("visibility.glossy", false);

        if (!getBoolAttr("mtap_visibleSpecular", depFn, true))
            paramArray.insert_path("visibility.specular", false);

        if (!getBoolAttr("mtap_visibleDiffuse", depFn, true))
            paramArray.insert_path("visibility.diffuse", false);
    }
}
