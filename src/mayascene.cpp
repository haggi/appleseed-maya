
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

#include "shadingtools/shadingutils.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "utilities/pystring.h"
#include "mayascene.h"
#include "renderqueueworker.h"
#include "world.h"

#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
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
#include <maya/MRenderView.h>
#include <maya/MVectorArray.h>
#include <maya/MFileIO.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnComponent.h>

MayaScene::MayaScene()
  : renderType(NORMAL)
  , renderState(MayaScene::UNDEF)
  , renderingStarted(false)
{
}

// Here is checked if the linklist is complete, what means that we do not have to do any
// complicated light linking, but default procedures. If excludedLights is > 0 then we found
// any excluded Lights.
bool MayaScene::listContainsAllLights(MDagPathArray& linkedLights, MDagPathArray& excludedLights)
{
    excludedLights.clear();
    for (uint lId = 0; lId < this->lightList.size(); lId++)
    {
        MDagPath path = this->lightList[lId]->dagPath;
        bool found = false;
        for (uint liId = 0; liId < linkedLights.length(); liId++)
        {
            MDagPath linkedPath = linkedLights[liId];
            if (linkedPath == path)
            {
                found = true;
                break;
            }
        }
        if (found)
            continue;
        else
            excludedLights.append(path);
    }

    if (excludedLights.length() > 0)
        return false;

    return true;
}

bool MayaScene::lightObjectIsInLinkedLightList(boost::shared_ptr<MayaObject> lightObject, MDagPathArray& linkedLightsArray)
{
    for (uint lId = 0; lId < linkedLightsArray.length(); lId++)
    {
        if (lightObject->mobject == linkedLightsArray[lId].node())
            return true;
    }
    return false;
}

// we have to take care for the component assignments in light linking.
// if a mesh has per face shader assignments, we have to ask for the components to get the correct light linking
// because lightLink.getLinkedLights() will give us a wrong result in this case.

void MayaScene::getLightLinking()
{
    MLightLinks lightLink;
    bool parseStatus;
    parseStatus = lightLink.parseLinks(MObject::kNullObj);

    std::vector<boost::shared_ptr<MayaObject> >::iterator oIt;
    for (oIt = objectList.begin(); oIt != objectList.end(); oIt++)
    {
        boost::shared_ptr<MayaObject> obj = *oIt;
        MDagPathArray lightArray;

        if (!obj->mobject.hasFn(MFn::kMesh) && !obj->mobject.hasFn(MFn::kNurbsSurface) && !obj->mobject.hasFn(MFn::kNurbsCurve))
            continue;

        if (obj->mobject.hasFn(MFn::kMesh))
        {
            MObjectArray shadingGroups, components;
            MFnMesh meshFn(obj->mobject);
            meshFn.getConnectedSetsAndMembers(obj->instanceNumber, shadingGroups, components, true);
            int componentElements = 0;
            for (uint cId = 0; cId < components.length(); cId++)
            {
                MFnComponent compFn(components[cId]);
                if (compFn.componentType() == MFn::kMeshPolygonComponent)
                    componentElements += compFn.elementCount();
            }
            if ((shadingGroups.length() > 1) || (componentElements > 0))
            {
                Logging::debug(MString("Object ") + obj->shortName + " has " + components.length() + " component groups and " + shadingGroups.length() + " shading groups.");
                for (uint cId = 0; cId < components.length(); cId++)
                {
                    MDagPathArray tmpLightArray;
                    lightLink.getLinkedLights(obj->dagPath, components[cId], tmpLightArray); // Lights linked to the face component
                    for (uint lp = 0; lp < tmpLightArray.length(); lp++)
                        lightArray.append(tmpLightArray[lp]);
                }
            }
            else
            {
                lightLink.getLinkedLights(obj->dagPath, MObject::kNullObj, lightArray);
            }
        }
        else
        {
            lightLink.getLinkedLights(obj->dagPath, MObject::kNullObj, lightArray);
        }
        // if one of the light in my scene light list is NOT in the linked light list,
        // the light has either turned off "Illuminate by default" or it is explicilty not linked to this object.
        for (size_t lObjId = 0; lObjId < this->lightList.size(); lObjId++)
        {
            if (!lightObjectIsInLinkedLightList(this->lightList[lObjId], lightArray))
            {
                this->lightList[lObjId]->excludedObjects.push_back(obj);
            }
        }
    }
}

boost::shared_ptr<MayaObject> MayaScene::getObject(MObject obj)
{
    boost::shared_ptr<MayaObject> mo;
    size_t numobjects = this->objectList.size();
    for (size_t objId = 0; objId < numobjects; objId++)
    {
        if (this->objectList[objId]->mobject == obj)
            return this->objectList[objId];
    }
    return mo;
}

boost::shared_ptr<MayaObject> MayaScene::getObject(MDagPath dp)
{
    boost::shared_ptr<MayaObject> mo;
    size_t numobjects = this->objectList.size();
    for (size_t objId = 0; objId < numobjects; objId++)
    {
        if (this->objectList[objId]->dagPath == dp)
            return this->objectList[objId];
    }
    return mo;
}

// the camera from the UI is set via render command
void MayaScene::setCurrentCamera(MDagPath camDagPath)
{
    this->uiCamera = camDagPath;
}

void MayaScene::checkParent(boost::shared_ptr<MayaObject> obj)
{
    MFnDagNode node(obj->mobject);
    if (node.parentCount() == 0)
        obj->parent.reset();
}

MDagPath MayaScene::getWorld()
{
    MItDag   dagIterator(MItDag::kDepthFirst, MFn::kInvalid);
    MDagPath dagPath;

    for (dagIterator.reset(); (!dagIterator.isDone()); dagIterator.next())
    {
        dagIterator.getPath(dagPath);
        if (dagPath.apiType() == MFn::kWorld)
            break;
    }
    return dagPath;
}

bool MayaScene::isGeo(MObject obj)
{
    if (obj.hasFn(MFn::kMesh))
        return true;
    if (obj.hasFn(MFn::kNurbsSurface))
        return true;
    if (obj.hasFn(MFn::kParticle))
        return true;
    if (obj.hasFn(MFn::kSubSurface))
        return true;
    if (obj.hasFn(MFn::kNurbsCurve))
        return true;
    if (obj.hasFn(MFn::kHairSystem))
        return true;

    return false;
}

void MayaScene::classifyMayaObject(boost::shared_ptr<MayaObject> obj)
{
    if (obj->mobject.hasFn(MFn::kCamera))
    {
        this->camList.push_back(obj);
        return;
    }

    if (obj->mobject.hasFn(MFn::kLight))
    {
        this->lightList.push_back(obj);
        return;
    }

    if (obj->mobject.hasFn(MFn::kInstancer))
    {
        instancerDagPathList.push_back(obj->dagPath);
        return;
    }

    this->objectList.push_back(obj);
}

MString MayaScene::getExportPath(MString ext, MString rendererName)
{
    std::string currentFile = MFileIO::currentFile().asChar();
    std::vector<std::string> parts;
    pystring::split(currentFile, parts, "/");
    currentFile = pystring::replace(parts.back(), ".ma", "");
    currentFile = pystring::replace(currentFile, ".mb", "");
    MString scenePath = getWorldPtr()->mRenderGlobals->basePath + "/" + rendererName + "/" + currentFile.c_str() + "." + ext;
    return scenePath;
}

MString MayaScene::getFileName()
{
    std::string currentFile = MFileIO::currentFile().asChar();
    std::vector<std::string> parts;
    pystring::split(currentFile, parts, "/");
    currentFile = pystring::replace(parts.back(), ".ma", "");
    currentFile = pystring::replace(currentFile, ".mb", "");
    return MString(currentFile.c_str());
}

void MayaScene::setRenderType(RenderType rtype)
{
    this->renderType = rtype;
}

std::vector<boost::shared_ptr<MayaObject> >  origObjects;

bool MayaScene::parseSceneHierarchy(MDagPath currentPath, int level, boost::shared_ptr<ObjectAttributes> parentAttributes, boost::shared_ptr<MayaObject> parentObject)
{
    // filter the new hypershade objects away
    if (pystring::find(currentPath.fullPathName().asChar(), "shaderBall") > -1)
        return true;

    boost::shared_ptr<MayaObject> mo(new MayaObject(currentPath));
    boost::shared_ptr<ObjectAttributes> currentAttributes = mo->getObjectAttributes(parentAttributes);
    mo->parent = parentObject;
    classifyMayaObject(mo);
    if (getWorldPtr()->getRenderType() == World::IPRRENDER)
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
    objectList.clear();
    camList.clear();
    lightList.clear();

    MDagPath world = getWorld();
    if (parseSceneHierarchy(world, 0, boost::shared_ptr<ObjectAttributes>(), boost::shared_ptr<MayaObject>()))
    {
        this->parseInstancerNew();
        this->getLightLinking();
        if (this->uiCamera.isValid() && (MGlobal::mayaState() != MGlobal::kBatch))
        {
            boost::shared_ptr<MayaObject> cam;
            for (uint camId = 0; camId < this->camList.size(); camId++)
            {
                if (this->camList[camId]->dagPath == this->uiCamera)
                    cam = this->camList[camId];
            }
            if (cam == 0)
            {
                Logging::error(MString("UI Camera not found: ") + this->uiCamera.fullPathName());
                return false;
            }

            camList.clear();
            camList.push_back(cam);
        }

        return true;
    }

    return false;
}

bool MayaScene::updateScene(MFn::Type updateElement)
{
    for (int objId = 0; objId < this->objectList.size(); objId++)
    {
        boost::shared_ptr<MayaObject> obj = this->objectList[objId];

        if (!obj->mobject.hasFn(updateElement))
            continue;

        obj->updateObject();

        // this part is only used if motionblur is turned on, else we have no MbElement::None
        if (!obj->motionBlurred)
        {
            if (getWorldPtr()->mRenderGlobals->currentMbElement.elementType == MbElement::MotionBlurNone)
            {
                Logging::debug(MString("found non mb element type. Updating non mb objects.") + objId + ": " + obj->dagPath.fullPathName());
                if (updateElement == MFn::kShape)
                    getWorldPtr()->mRenderer->updateShape(obj);
                if (updateElement == MFn::kTransform)
                {
                    obj->transformMatrices.clear();
                    getWorldPtr()->mRenderer->updateTransform(obj);
                    obj->transformMatrices.push_back(obj->dagPath.inclusiveMatrix());
                }
            }
        }

        if (getWorldPtr()->mRenderGlobals->isDeformStep())
            if (obj->mobject.hasFn(MFn::kShape))
                getWorldPtr()->mRenderer->updateShape(obj);

        if (getWorldPtr()->mRenderGlobals->isTransformStep())
        {
            if (getWorldPtr()->mRenderGlobals->isMbStartStep())
                obj->transformMatrices.clear();

            obj->transformMatrices.push_back(obj->dagPath.inclusiveMatrix());
            if (obj->mobject.hasFn(MFn::kTransform))
                getWorldPtr()->mRenderer->updateTransform(obj);
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
        boost::shared_ptr<MayaObject> obj = this->camList[camId];
        obj->updateObject();

        if (!getWorldPtr()->mRenderGlobals->isMbStartStep())
            if (!obj->motionBlurred)
                continue;

        if (getWorldPtr()->mRenderGlobals->isTransformStep())
        {
            if (getWorldPtr()->mRenderGlobals->isMbStartStep())
                obj->transformMatrices.clear();
            obj->transformMatrices.push_back(obj->dagPath.inclusiveMatrix());
            getWorldPtr()->mRenderer->updateTransform(obj);
        }
    }

    std::vector<boost::shared_ptr<MayaObject> > ::iterator mIter;
    mIter = this->lightList.begin();
    for (; mIter != this->lightList.end(); mIter++)
    {
        boost::shared_ptr<MayaObject> obj = *mIter;
        obj->updateObject();

        if (!getWorldPtr()->mRenderGlobals->isMbStartStep())
            if (!obj->motionBlurred)
                continue;

        if (getWorldPtr()->mRenderGlobals->isTransformStep())
        {
            if (getWorldPtr()->mRenderGlobals->isMbStartStep())
                obj->transformMatrices.clear();
            obj->transformMatrices.push_back(obj->dagPath.inclusiveMatrix());
            getWorldPtr()->mRenderer->updateShape(obj);
        }
    }

    if (getWorldPtr()->mRenderGlobals->isTransformStep())
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
    // updates only required for a transform step
    if (!getWorldPtr()->mRenderGlobals->isTransformStep())
        return true;

    size_t numElements = this->instancerNodeElements.size();
    for (size_t i = 0; i < numElements; i++)
    {
        boost::shared_ptr<MayaObject> obj = this->instancerNodeElements[i];
        MMatrix origMatrix;
        origMatrix.setToIdentity();
        if (obj->origObject != 0)
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
        if (getWorldPtr()->mRenderGlobals->isMbStartStep())
            obj->transformMatrices.clear();

        obj->transformMatrices.push_back(origMatrix * matrix);
        getWorldPtr()->mRenderer->updateTransform(obj);
    }
    return true;
}

std::vector<boost::shared_ptr<MayaObject> >  InstDoneList;

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
                else
                {
                    Logging::debug(MString("Could nod get a particleSystem from node "));
                }
            }
        }

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
            boost::shared_ptr<ObjectAttributes> currentAttributes;
            for (int i = pathStart; i < pathStart + numPaths; i++)
            {
                int curPathIndex = pathIndices[i];
                MDagPath curPath = allPaths[curPathIndex];

                boost::shared_ptr<MayaObject> particleMObject(new MayaObject(curPath));
                MFnDependencyNode pOrigNode(particleMObject->mobject);
                MObject pOrigObject = pOrigNode.object();

                // search for the correct orig MayaObject element
                // todo: visibiliy check - necessary?
                boost::shared_ptr<MayaObject> origObj;
                std::vector<boost::shared_ptr<MayaObject> > ::iterator mIter = origObjects.begin();
                for (; mIter != origObjects.end(); mIter++)
                {
                    boost::shared_ptr<MayaObject> obj = *mIter;
                    if (obj->mobject == pOrigObject)
                    {
                        origObj = obj;
                    }
                }
                if (origObj == 0)
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
                currentAttributes->hasInstancerConnection = true;
                if (hasParticleSystem)
                {
                    if (particleMObject->attributes != 0)
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
