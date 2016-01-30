
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
#include <maya/MFnLight.h>
#include <maya/MPlugArray.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MEulerRotation.h>
#include <maya/MAngle.h>
#include "mayascene.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "utilities/pystring.h"
#include "world.h"
#include "appleseedutils.h"
#include "renderer/api/edf.h"

static Logging logger;

using namespace AppleRender;
#define colorAttr(c) (MString("") + c.r + " " + c.g + " " + c.g)

void AppleseedRenderer::defineLight(sharedPtr<MayaObject> obj)
{
    asr::Assembly *lightAssembly = getCreateObjectAssembly(obj);
    asr::AssemblyInstance *lightAssemblyInstance = getExistingObjectAssemblyInstance(obj.get());
    asr::Light *light = lightAssembly->lights().get_by_name(obj->shortName.asChar());
    MFnDependencyNode depFn(obj->mobject);

    if (obj->mobject.hasFn(MFn::kPointLight))
    {
        bool cast_indirect_light = getBoolAttr("mtap_cast_indirect_light", depFn, true);
        float importance_multiplier = getFloatAttr("mtap_importance_multiplier", depFn, 1.0f);
        MColor col = getColorAttr("color", depFn);
        float intensity = getFloatAttr("intensity", depFn, 1.0f);
        MString colorAttribute = obj->shortName + "_intensity";
        defineColor(project.get(), colorAttribute.asChar(), col, intensity);
        int decay = getEnumInt("decayRate", depFn);
        if (light == nullptr)
        {
            asf::auto_release_ptr<asr::Light> lp = asf::auto_release_ptr<asr::Light>(
                asr::PointLightFactory().create(
                obj->shortName.asChar(),
                asr::ParamArray()));
            lightAssembly->lights().insert(lp);
            light = lightAssembly->lights().get_by_name(obj->shortName.asChar());
        }
        asr::ParamArray& params = light->get_parameters();
        params.insert("intensity", colorAttribute);
        params.insert("intensity_multiplier", intensity);
        params.insert("importance_multiplier", importance_multiplier);
        params.insert("cast_indirect_light", cast_indirect_light);
        fillTransformMatrices(obj, light);
    }
    if (obj->mobject.hasFn(MFn::kSpotLight))
    {
        // redefinition because it is possible that this value is textured
        bool cast_indirect_light = getBoolAttr("mtap_cast_indirect_light", depFn, true);
        float importance_multiplier = getFloatAttr("mtap_importance_multiplier", depFn, 1.0f);
        MColor col = getColorAttr("color", depFn);
        float intensity = getFloatAttr("intensity", depFn, 1.0f);
        MString colorAttribute = obj->shortName + "_intensity";
        defineColor(project.get(), colorAttribute.asChar(), col, intensity);
        Logging::debug(MString("Creating spotLight: ") + depFn.name());
        float coneAngle = getDegree("coneAngle", depFn);
        float penumbraAngle = getDegree("penumbraAngle", depFn);
        float inner_angle = coneAngle;
        float outer_angle = coneAngle + penumbraAngle;

        if (light == nullptr)
        {
            asf::auto_release_ptr<asr::Light> lp = asr::SpotLightFactory().create(
                obj->shortName.asChar(),
                asr::ParamArray());
            lightAssembly->lights().insert(lp);
            light = lightAssembly->lights().get_by_name(obj->shortName.asChar());
        }

        asr::ParamArray& params = light->get_parameters();
        params.insert("radiance", colorAttribute);
        params.insert("radiance_multiplier", intensity);
        params.insert("inner_angle", inner_angle);
        params.insert("outer_angle", outer_angle);
        params.insert("importance_multiplier", importance_multiplier);
        params.insert("cast_indirect_light", cast_indirect_light);
        MMatrix matrix = obj->transformMatrices[0];
        fillTransformMatrices(obj, light);
    }
    if (obj->mobject.hasFn(MFn::kDirectionalLight))
    {
        bool cast_indirect_light = getBoolAttr("mtap_cast_indirect_light", depFn, true);
        float importance_multiplier = getFloatAttr("mtap_importance_multiplier", depFn, 1.0f);
        MVector lightDir(0, 0, -1);
        MVector lightDirTangent(1, 0, 0);
        MVector lightDirBiTangent(0, 1, 0);
        MColor col = getColorAttr("color", depFn);
        float intensity = getFloatAttr("intensity", depFn, 1.0f);
        MString colorAttribute = obj->shortName + "_intensity";
        defineColor(project.get(), colorAttribute.asChar(), col, intensity);

        if (isSunLight(obj->mobject))
        {
            //Logging::debug(MString("Found sunlight."));
            //light = asf::auto_release_ptr<asr::Light>(
            //  asr::SunLightFactory().create(
            //  "sunLight",
            //  asr::ParamArray()
            //      .insert("environment_edf", "sky_edf")
            //      .insert("turbidity", renderGlobals->sunTurbidity)
            //      .insert("radiance_multiplier", renderGlobals->sunExitanceMultiplier * intensity / 30.0f)
            //      ));
        }
        else{
            if (light == nullptr)
            {
                asf::auto_release_ptr<asr::Light> lp = asr::DirectionalLightFactory().create(
                    obj->shortName.asChar(),
                    asr::ParamArray());
                lightAssembly->lights().insert(lp);
                light = lightAssembly->lights().get_by_name(obj->shortName.asChar());
            }
            asr::ParamArray& params = light->get_parameters();
            params.insert("irradiance", colorAttribute);
            params.insert("irradiance_multiplier", intensity);
            params.insert("importance_multiplier", importance_multiplier);
            params.insert("cast_indirect_light", cast_indirect_light);
        }
        fillTransformMatrices(obj, light);
    }

    if (obj->mobject.hasFn(MFn::kAreaLight))
    {
        MString areaLightName = obj->fullNiceName;
        asf::auto_release_ptr<asr::MeshObject> plane = defineStandardPlane(true);
        plane->set_name(areaLightName.asChar());
        MayaObject *assemblyObject = getAssemblyMayaObject(obj.get());
        asr::Assembly *ass = getCreateObjectAssembly(obj);
        ass->objects().insert(asf::auto_release_ptr<asr::Object>(plane));
        asr::MeshObject *meshPtr = (asr::MeshObject *)ass->objects().get_by_name(areaLightName.asChar());
        MString objectInstanceName = getObjectInstanceName(obj.get());
        MMatrix assemblyObjectMatrix = assemblyObject->dagPath.inclusiveMatrix();
        // rotate the defalt up pointing standard plane by 90 degree to match the area light direction
        MTransformationMatrix tm;
        double rotate90Deg[3] = { -M_PI_2, 0, 0 };
        //double rotate90Deg[3] = { 0, 0, 0 };
        tm.setRotation(rotate90Deg, MTransformationMatrix::kXYZ);
        MMatrix objectMatrix = tm.asMatrix();
        MMatrix diffMatrix = objectMatrix;// *assemblyObjectMatrix;
        asf::Matrix4d appleMatrix;
        asf::Matrix4d::identity();
        MMatrixToAMatrix(diffMatrix, appleMatrix);
        appleMatrix = asf::Matrix4d::identity();
        MString areaLightMaterialName = areaLightName + "_material";

        MString physicalSurfaceName = areaLightName + "_physical_surface_shader";
        MString areaLightColorName = areaLightName + "_color";
        MString edfName = areaLightName + "_edf";
        asr::ParamArray edfParams;
        MString lightColor = lightColorAsString(depFn);
        MColor color = getColorAttr("color", depFn);
        defineColor(project.get(), areaLightColorName.asChar(), color, getFloatAttr("intensity", depFn, 1.0f));
        edfParams.insert("radiance", areaLightColorName.asChar());
        //edfParams.insert("radiance_multiplier", getFloatAttr("intensity", depFn, 1.0f));

        asf::auto_release_ptr<asr::EDF> edf = asr::DiffuseEDFFactory().create(edfName.asChar(), edfParams);
        ass->edfs().insert(edf);

        ass->surface_shaders().insert(
            asr::PhysicalSurfaceShaderFactory().create(
            physicalSurfaceName.asChar(),
            asr::ParamArray()));

        ass->materials().insert(
            asr::GenericMaterialFactory().create(
            areaLightMaterialName.asChar(),
            asr::ParamArray()
            .insert("surface_shader", physicalSurfaceName.asChar())
            .insert("edf", edfName.asChar())));

        asr::ParamArray objInstanceParamArray;
        addVisibilityFlags(obj, objInstanceParamArray);

        ass->object_instances().insert(
            asr::ObjectInstanceFactory::create(
            objectInstanceName.asChar(),
            objInstanceParamArray,
            meshPtr->get_name(),
            asf::Transformd::from_local_to_parent(appleMatrix),
            asf::StringDictionary()
            .insert("slot0", areaLightMaterialName.asChar()),
            asf::StringDictionary()
            .insert("slot0", "default")));

        if (lightAssemblyInstance != nullptr)
            fillMatrices(obj, lightAssemblyInstance->transform_sequence());
    }
}

void AppleseedRenderer::defineLights()
{
    MStatus stat;
    MFnDependencyNode rGlNode(getRenderGlobalsNode());
    // first get the globals node and serach for a directional light connection
    MObject coronaGlobals = getRenderGlobalsNode();
    sharedPtr<RenderGlobals> renderGlobals = MayaTo::getWorldPtr()->worldRenderGlobalsPtr;
    sharedPtr<MayaScene> mayaScene = MayaTo::getWorldPtr()->worldScenePtr;

    std::vector<sharedPtr<MayaObject> >::iterator oIt;
    for (oIt = mayaScene->lightList.begin(); oIt != mayaScene->lightList.end(); oIt++)
    {
        sharedPtr<MayaObject> mobj = *oIt;
        sharedPtr<mtap_MayaObject> obj(staticPtrCast<mtap_MayaObject>(mobj));

        if (!obj->visible)
            continue;

        if (isSunLight(obj->mobject))
            continue;

        defineLight(obj);
    }

}
