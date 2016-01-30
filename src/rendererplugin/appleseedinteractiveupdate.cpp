#include "appleseed.h"
#include "appleseedutils.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "mayascene.h"

using namespace AppleRender;

void AppleseedRenderer::doInteractiveUpdate()
{
    Logging::debug("AppleseedRenderer::doInteractiveUpdate");
    if (interactiveUpdateList.empty())
        return;
    std::vector<InteractiveElement *>::iterator iaIt;
    for (iaIt = interactiveUpdateList.begin(); iaIt != interactiveUpdateList.end(); iaIt++)
    {
        InteractiveElement *iElement = *iaIt;
        if (iElement->node.hasFn(MFn::kShadingEngine))
        {
            if (iElement->obj)
            {
                Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate - found shadingEngine.") + iElement->name);
                updateMaterial(iElement->node);
            }
        }
        if (iElement->node.hasFn(MFn::kCamera))
        {
            Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate - found camera.") + iElement->name);
            if (iElement->obj)
                defineCamera(iElement->obj);
        }
        if (iElement->node.hasFn(MFn::kLight))
        {
            Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate - found light.") + iElement->name);
            if (iElement->obj)
            {
                defineLight(iElement->obj);
            }
        }
        // shading nodes
        if (iElement->node.hasFn(MFn::kPluginDependNode) || iElement->node.hasFn(MFn::kLambert))
        {
            MFnDependencyNode depFn(iElement->node);
            std::vector<MString> shaderNames = { "asLayeredShader", "uberShader", "asDisneyMaterial" };
            MString typeName = depFn.typeName();
            for (uint si = 0; si < shaderNames.size(); si++)
            {
                if (typeName == shaderNames[si])
                {
                    sharedPtr<mtap_MayaObject> obj = staticPtrCast<mtap_MayaObject>(iElement->obj);
                    Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate - found shader.") + iElement->name);
                    this->defineMaterial(obj);
                }
            }
        }
        if (iElement->node.hasFn(MFn::kMesh))
        {
            sharedPtr<mtap_MayaObject> obj = staticPtrCast<mtap_MayaObject>(iElement->obj);
            if (obj->removed)
            {
                continue;
            }

            if (iElement->triggeredFromTransform)
            {
                //Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate - found mesh triggered from transform - update instance.") + iElement->name);
                Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate mesh ") + iElement->name + " ieNodeName " + getObjectName(iElement->node) + " objDagPath " + iElement->obj->dagPath.fullPathName());
                MStatus stat;
                //MFnDagNode dn(iElement->node, &stat);
                //Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate dn ") + dn.fullPathName() + " (sollte sein: " + iElement->name + ")");
                //MDagPath mdp;
                //stat = dn.getPath(mdp);
                //Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate dp ") + mdp.fullPathName() + " (sollte sein: " + iElement->name + ")");

                asr::AssemblyInstance *assInst = getExistingObjectAssemblyInstance(obj.get());
                if (assInst == nullptr)
                    continue;

                MMatrix m = iElement->obj->dagPath.inclusiveMatrix(&stat);
                if (!stat)
                    Logging::debug(MString("Error ") + stat.errorString());
                assInst->transform_sequence().clear();
                fillTransformMatrices(m, assInst);
                assInst->bump_version_id();
            }
            else{
                if ( obj->instanceNumber == 0)
                    updateGeometry(obj);
                if (obj->instanceNumber > 0)
                    updateInstance(obj);
            }
        }
    }
    interactiveUpdateList.clear();
}
