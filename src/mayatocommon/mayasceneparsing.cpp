
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

#include "mayascene.h"
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnInstancer.h>
#include <maya/MFnParticleSystem.h>
#include <maya/MMatrixArray.h>
#include <maya/MIntArray.h>
#include <maya/MLightLinks.h>
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/M3dView.h>
#include <maya/MRenderView.h>
#include <maya/MVectorArray.h>
#include <maya/MFileIO.h>
#include "world.h"
#include "mayaobjectfactory.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "utilities/pystring.h"
#include "rendering/renderer.h"

std::vector<sharedPtr<MayaObject> >  origObjects;

bool MayaScene::parseSceneHierarchy(MDagPath currentPath, int level, sharedPtr<ObjectAttributes> parentAttributes, sharedPtr<MayaObject> parentObject)
{
    // filter the new hypershade objects away
    if (pystring::find(currentPath.fullPathName().asChar(), "shaderBall") > -1)
        return true;

    sharedPtr<MayaObject> mo = MayaTo::MayaObjectFactory().createMayaObject(currentPath);
    sharedPtr<ObjectAttributes> currentAttributes = mo->getObjectAttributes(parentAttributes);
    mo->parent = parentObject;
    classifyMayaObject(mo);
    if (MayaTo::getWorldPtr()->renderType == MayaTo::MayaToWorld::WorldRenderType::IPRRENDER)
    {
        InteractiveElement iel;
        iel.obj = mo;
        iel.name = mo->fullName;
        iel.node = mo->mobject;
        interactiveUpdateMap[interactiveUpdateMap.size()] = iel;
    }

    //
    //  find the original mayaObject for instanced objects. Can be useful later.
    //

    if (currentPath.instanceNumber() == 0)
        origObjects.push_back(mo);
    else
    {
        MFnDagNode node(currentPath.node());
        for (size_t iId = 0; iId < origObjects.size(); iId++)
        {
            MFnDagNode onode(origObjects[iId]->mobject);
            if (onode.object() == node.object())
            {
                mo->origObject = origObjects[iId];
                break;
            }
        }
    }

    uint numChilds = currentPath.childCount();
    for (uint chId = 0; chId < numChilds; chId++)
    {
        MDagPath childPath = currentPath;
        MStatus stat = childPath.push(currentPath.child(chId));
        if (!stat)
        {
            Logging::debug(MString("Child path problem: parent: ") + currentPath.fullPathName() + " child id " + chId + " type " + currentPath.child(chId).apiTypeStr());
            continue;
        }
        MString childName = childPath.fullPathName();
        parseSceneHierarchy(childPath, level + 1, currentAttributes, mo);
    }

    return true;
}

bool MayaScene::parseScene()
{
    origObjects.clear();

    clearObjList(this->objectList);
    clearObjList(this->camList);
    clearObjList(this->lightList);

    MDagPath world = getWorld();
    if (parseSceneHierarchy(world, 0, nullptr, nullptr))
    {
        this->parseInstancerNew();
        this->getLightLinking();
        if (this->uiCamera.isValid() && (MGlobal::mayaState() != MGlobal::kBatch))
        {
            sharedPtr<MayaObject> cam = nullptr;
            for (uint camId = 0; camId < this->camList.size(); camId++)
            {
                if (this->camList[camId]->dagPath == this->uiCamera)
                    cam = this->camList[camId];
            }
            if (cam == nullptr)
            {
                Logging::error(MString("UI Camera not found: ") + this->uiCamera.fullPathName());
                return false;
            }
            clearObjList(this->camList, cam);
        }
        this->good = true;

        return true;
    }
    return false;
}

bool MayaScene::updateScene(MFn::Type updateElement)
{

    Logging::debug(MString("MayaScene::updateSceneNew."));

    for (int objId = 0; objId < this->objectList.size(); objId++)
    {
        sharedPtr<MayaObject> obj = this->objectList[objId];

        if (!obj->mobject.hasFn(updateElement))
            continue;

        obj->updateObject();
        Logging::debug(MString("updateObj ") + objId + ": " + obj->dagPath.fullPathName());

        // this part is only used if motionblur is turned on, else we have no MbElement::None
        if (!obj->motionBlurred)
        {
            if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr->currentMbElement.elementType == MbElement::MotionBlurNone)
            {
                Logging::debug(MString("found non mb element type. Updating non mb objects.") + objId + ": " + obj->dagPath.fullPathName());
                if (updateElement == MFn::kShape)
                    MayaTo::getWorldPtr()->worldRendererPtr->updateShape(obj);
                if (updateElement == MFn::kTransform)
                {
                    obj->transformMatrices.clear();
                    MayaTo::getWorldPtr()->worldRendererPtr->updateTransform(obj);
                    obj->transformMatrices.push_back(obj->dagPath.inclusiveMatrix());
                }
            }
        }

        if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isDeformStep())
            if (obj->mobject.hasFn(MFn::kShape))
                MayaTo::getWorldPtr()->worldRendererPtr->updateShape(obj);

        if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isTransformStep())
        {
            if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isMbStartStep())
                obj->transformMatrices.clear();

            obj->transformMatrices.push_back(obj->dagPath.inclusiveMatrix());
            if (obj->mobject.hasFn(MFn::kTransform))
                MayaTo::getWorldPtr()->worldRendererPtr->updateTransform(obj);
        }
    }

    return true;
}

bool MayaScene::updateScene()
{
    updateScene(MFn::kShape);
    updateScene(MFn::kTransform);

    for (size_t camId = 0; camId < this->camList.size(); camId++)
    {
        sharedPtr<MayaObject> obj = this->camList[camId];
        obj->updateObject();

        if (!MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isMbStartStep())
            if (!obj->motionBlurred)
                continue;

        if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isTransformStep())
        {
            if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isMbStartStep())
                obj->transformMatrices.clear();
            obj->transformMatrices.push_back(obj->dagPath.inclusiveMatrix());
            MayaTo::getWorldPtr()->worldRendererPtr->updateTransform(obj);
        }
    }


    std::vector<sharedPtr<MayaObject> > ::iterator mIter;
    mIter = this->lightList.begin();
    for (; mIter != this->lightList.end(); mIter++)
    {
        sharedPtr<MayaObject> obj = *mIter;
        obj->updateObject();

        if (!MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isMbStartStep())
            if (!obj->motionBlurred)
                continue;

        if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isTransformStep())
        {
            if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isMbStartStep())
                obj->transformMatrices.clear();
            obj->transformMatrices.push_back(obj->dagPath.inclusiveMatrix());
            MayaTo::getWorldPtr()->worldRendererPtr->updateShape(obj);
        }
    }

    if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isTransformStep())
    {
        this->updateInstancer();
    }

    return true;
}

void MayaScene::clearInstancerNodeList()
{
    size_t numElements = this->instancerNodeElements.size();
    this->instancerNodeElements.clear();
}

bool MayaScene::updateInstancer()
{
    Logging::debug("update instancer.");

    // updates only required for a transform step
    if (!MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isTransformStep())
        return true;

    size_t numElements = this->instancerNodeElements.size();
    for (size_t i = 0; i < numElements; i++)
    {
        sharedPtr<MayaObject> obj = this->instancerNodeElements[i];
        MMatrix origMatrix;
        origMatrix.setToIdentity();
        if (obj->origObject != nullptr)
        {
            origMatrix = obj->origObject->dagPath.inclusiveMatrix();
            origMatrix.inverse();
        }
        MFnInstancer instFn(obj->instancerDagPath);
        MDagPathArray dagPathArray;
        MMatrix matrix;
        instFn.instancesForParticle(obj->instancerParticleId, dagPathArray, matrix);
        for (uint k = 0; k < dagPathArray.length(); k++)
            Logging::debug(MString("Particle mobj id: ") + i + "particle id: " + obj->instancerParticleId + " path id " + k + " - " + dagPathArray[k].fullPathName());
        if (MayaTo::getWorldPtr()->worldRenderGlobalsPtr->isMbStartStep())
            obj->transformMatrices.clear();

        obj->transformMatrices.push_back(origMatrix * matrix);
        MayaTo::getWorldPtr()->worldRendererPtr->updateTransform(obj);
    }
    return true;
}

std::vector<sharedPtr<MayaObject> >  InstDoneList;
// parsing all particle instancer nodes in the scene
// here I put all nodes created by an instancer node into a array of its own.
// The reason why I don't simply add it to the main list is that I have to recreate them
// during sceneUpdate() for every frame because the list can change with the birth or death
// of a particle
bool MayaScene::parseInstancerNew()
{
    MStatus stat;
    bool result = true;
    Logging::debug(MString("parseInstancerNew"));
    MDagPath dagPath;
    size_t numInstancers = instancerDagPathList.size();
    size_t numobjects = this->objectList.size();

    // if we have any previously defined instancer elements, delete it
    this->clearInstancerNodeList();

    for (int iId = 0; iId < numInstancers; iId++)
    {
        MDagPath instPath = instancerDagPathList[iId];
        MObject instancerMObject = instPath.node();
        MString path = instPath.fullPathName();
        MFnInstancer instFn(instPath);
        int numParticles = instFn.particleCount();
        Logging::debug(MString("Detected instancer. instPath: ") + path + " it has " + numParticles + " particle instances");
        MDagPathArray allPaths;
        MMatrixArray allMatrices;
        MIntArray pathIndices;
        MIntArray pathStartIndices;
        MObjectArray nodeList;
        getConnectedInNodes(MString("inputPoints"), instPath.node(), nodeList);
        bool hasParticleSystem = false;
        MVectorArray rgbPP;
        if (nodeList.length() > 0)
        {
            if (nodeList[0].hasFn(MFn::kParticle))
            {
                Logging::debug(MString("Found a particle system called ") + getObjectName(nodeList[0]));
                MFnParticleSystem pSystem(nodeList[0], &stat);
                if (stat)
                {
                    if (pSystem.hasRgb())
                    {
                        hasParticleSystem = true;
                        pSystem.rgb(rgbPP);
                    }
                }
                else{
                    Logging::debug(MString("Could nod get a particleSystem from node "));
                }
            }
        }
        //MFnParticleSystem

        // give me all instances in this instancer
        instFn.allInstances(allPaths, allMatrices, pathStartIndices, pathIndices);

        for (int p = 0; p < numParticles; p++)
        {
            MMatrix particleMatrix = allMatrices[p];

            //  the number of paths instanced under a particle is computed by
            //  taking the difference between the starting path index for this
            //  particle and that of the next particle.  The size of the start
            //  index array is always one larger than the number of particles.
            //
            int numPaths = pathStartIndices[p + 1] - pathStartIndices[p];

            //  the values pathIndices[pathStart...pathStart+numPaths] give the
            //  indices in the allPaths array of the paths instanced under this
            //  particular particle.  Remember, different paths can be instanced
            //  under each particle.
            //
            int pathStart = pathStartIndices[p];

            //  loop through the instanced paths for this particle
            sharedPtr<ObjectAttributes> currentAttributes = nullptr;
            for (int i = pathStart; i < pathStart + numPaths; i++)
            {
                int curPathIndex = pathIndices[i];
                MDagPath curPath = allPaths[curPathIndex];

                sharedPtr<MayaObject> particleMObject = MayaTo::MayaObjectFactory().createMayaObject(curPath);
                MFnDependencyNode pOrigNode(particleMObject->mobject);
                MObject pOrigObject = pOrigNode.object();

                // search for the correct orig MayaObject element
                // TODO: visibiliy check - necessary?
                sharedPtr<MayaObject> origObj = nullptr;
                std::vector<sharedPtr<MayaObject> > ::iterator mIter = origObjects.begin();
                for (; mIter != origObjects.end(); mIter++)
                {
                    sharedPtr<MayaObject> obj = *mIter;
                    if (obj->mobject == pOrigObject)
                    {
                        origObj = obj;
                    }
                }
                if (origObj == nullptr)
                {
                    Logging::debug(MString("Orig particle instancer obj not found."));
                    continue;
                }
                currentAttributes = particleMObject->getObjectAttributes(origObj->attributes);
                particleMObject->origObject = origObj;
                particleMObject->isInstancerObject = true;
                particleMObject->visible = true;
                particleMObject->instancerParticleId = p;
                particleMObject->instanceNumber = p;
                particleMObject->instancerDagPath = instPath;
                particleMObject->instancerMObj = instancerMObject;
                particleMObject->fullName = origObj->fullName + MString("_i_") + p;
                particleMObject->shortName = origObj->shortName + MString("_i_") + p;
                this->instancerNodeElements.push_back(particleMObject);
                particleMObject->index = (int)(this->instancerNodeElements.size() - 1);
                currentAttributes->hasInstancerConnection = true;
                if (hasParticleSystem)
                {
                    if (particleMObject->attributes != nullptr)
                    {
                        particleMObject->attributes->hasColorOverride = true;
                        particleMObject->attributes->colorOverride = MColor(rgbPP[p].x, rgbPP[p].y, rgbPP[p].z);
                    }
                }
            }
        }
    }
    return true;
}
