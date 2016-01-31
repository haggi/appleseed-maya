
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

#ifndef MAYA_OBJECT_H
#define MAYA_OBJECT_H

#include <vector>
#include <memory>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MIntArray.h>
#include <maya/MFloatArray.h>
#include <maya/MPointArray.h>
#include <maya/MVectorArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MDagPath.h>
#include <maya/MBoundingBox.h>
#include <maya/MString.h>
#include <maya/MMatrix.h>
#include <maya/MColor.h>

#include "definitions.h"

class MayaScene;
class Material;

// not all renderers can define a mesh and then upate it later in the
// translation process. e.g. Corona needs all motion mesh informations during
// mesh definition and cannot modify it with additional motion steps.
// So we use this struct to collect the necessary deform informations.
struct MeshData{
    MPointArray points;
    MFloatVectorArray normals;
};

// the idea of objectAttributes is to share informations down the whole hierarchy.
// an attribute will be created with its parent as argument so it can copy the interesting data
class ObjectAttributes
{
public:
    bool hasInstancerConnection;
    // perParticleInfos - first try, need to find a intelligent way to find the correct values.
    MColor colorOverride;
    bool hasColorOverride;
    float opacityOverride;
    MMatrix objectMatrix;
private:
};

class MayaObject : public MBoundingBox
{
public:
    MObject mobject;
    MString shortName;
    MString fullName;
    MString fullNiceName;
    int index;
    MDagPath dagPath;
    bool removed = false; // in IPR we simply flag an object as removed instead of really removing it
    sharedPtr<ObjectAttributes> attributes;

    std::vector<MDagPath> linkedLights; // for objects - light linking
    bool lightExcludeList; // if true the linkedLights list contains excluded lights, else included lights
    std::vector<MDagPath> shadowObjects; // for lights - shadow linking
    bool shadowExcludeList; // if true the shadowObjects contains objects which ignores shadows from the current light
    std::vector<MDagPath> castNoShadowObjects; // for lights - shadow linking
    std::vector<sharedPtr<MayaObject> >  excludedObjects; // for lights - excluded objects


    std::vector<MString> exportFileNames; // for every mb step complete filename for every exported shape file
    std::vector<MString> hierarchyNames; // for every geo mb step I have one name, /obj/cube0, /obj/cube1...
    std::vector<MMatrix> transformMatrices; // for every xmb step I have one matrix
    std::vector<MString> shadowMapFiles; // file paths for a shadow map creating light

    std::vector<MeshData> meshDataList;
    MObjectArray shadingGroups;
    MIntArray perFaceAssignments;
    std::vector<sharedPtr<Material> > materialList; // for every shading group connected to the shape, we have a material

    // instancer node attributes
    MMatrix instancerMatrix; // matrix of instancer node and paricle node
    bool isInstancerObject; // is this an object created by an instancer node
    int instancerParticleId;
    MObject instancerMObj;
    MDagPath instancerDagPath;

    bool animated;
    bool hasInstancerConnection; // if yes, then the objects below can be visible via instancer even if the original object is not
    bool shapeConnected;         // if shape connected, it can be used to determine if it has to be exported for every frame or not
    bool visible;                // important for instances: orig object can be invisible but must be exported
    uint instanceNumber;         // number of instance
    int  perObjectTransformSteps;// number of xform steps. Some renderers can use different xform/deform steps
    int  perObjectDeformSteps;   // number of deform steps.
    bool motionBlurred;          // possibility to turn off motionblur for this object
    bool geometryMotionblur;     // if object has vertex velocity informations, there is no need for real deformation blur
    bool shadowMapCastingLight();// to know if I have to add a light to render passes
    bool isObjAnimated();
    bool isShapeConnected();
    bool isObjVisible();
    bool isLight();
    bool isCamera();
    bool isTransform();
    bool isGeo();
    bool isVisiblityAnimated();
    bool isInstanced();
    void getShadingGroups();
    bool hasBifrostVelocityChannel();
    void addMeshData(); // add point/normals to the meshDataList for motionsteps
    void getMeshData(MPointArray& point, MFloatVectorArray& normals);
    void getMeshData(MPointArray& point, MFloatVectorArray& normals, MFloatArray& u,
                    MFloatArray& v, MIntArray& triPointIndices, MIntArray& triNormalIndices,
                    MIntArray& triUvIndices, MIntArray& triMatIndices); // all triIndices contain per vertex indices except the triMatIndices, this is per face
    virtual bool geometryShapeSupported();
    virtual sharedPtr<ObjectAttributes> getObjectAttributes(sharedPtr<ObjectAttributes> parentAttributes = sharedPtr<ObjectAttributes>()) = 0;

    sharedPtr<MayaObject> parent;
    sharedPtr<MayaObject> origObject; // this is necessary for instanced objects that have to access the original objects data
    MayaObject(MObject& mobject);
    MayaObject(MDagPath& objPath);
    virtual ~MayaObject();
    void initialize();
    void updateObject();
};

void addLightIdentifier(int id);
void addObjectIdentifier(int id);

#endif
