
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
                Logging::debug(MString("AppleseedRenderer::doInteractiveUpdate mesh ") + iElement->name + " ieNodeName " + getObjectName(iElement->node) + " objDagPath " + iElement->obj->dagPath.fullPathName());
                MStatus stat;

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
                if (obj->instanceNumber == 0)
                    updateGeometry(obj);
                if (obj->instanceNumber > 0)
                    updateInstance(obj);
            }
        }
    }
    interactiveUpdateList.clear();
}
