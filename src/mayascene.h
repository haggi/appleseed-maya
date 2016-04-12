
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

#ifndef MAYASCENE_H
#define MAYASCENE_H

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MObject.h>
#include <maya/MTransformationMatrix.h>

#include <map>
#include <vector>

#include "renderglobals.h"
#include "mayaobject.h"

class InteractiveElement
{
  public:
    boost::shared_ptr<MayaObject> obj;
    MObject mobj;
    MString name;
    MObject node;

    bool triggeredFromTransform; // to recognize if we have to update the shape or only the instance transform

    InteractiveElement()
    {
        triggeredFromTransform = false;
    }
};

class MayaScene
{
  public:
    enum RenderType
    {
        NORMAL,
        IPR,
        NONE,
        BATCH
    };

    enum RenderState
    {
        START = 0,
        TRANSLATE = 1,
        RENDERING = 2,
        UNDEF = 5
    };

    RenderType renderType;
    RenderState renderState;
    std::vector<int> lightIdentifier; // plugids for detecting new lighttypes
    std::vector<int> objectIdentifier; // plugids for detecting new objTypes
    std::vector<MObject> mObjectList;
    std::vector<boost::shared_ptr<MayaObject> >  objectList;
    std::vector<boost::shared_ptr<MayaObject> >  camList;
    std::vector<boost::shared_ptr<MayaObject> >  lightList;
    std::vector<boost::shared_ptr<MayaObject> >  instancerNodeElements; // so its easier to update them
    std::vector<MDagPath> instancerDagPathList;

    float currentFrame;
    bool parseSceneHierarchy(MDagPath currentObject, int level, boost::shared_ptr<ObjectAttributes> attr, boost::shared_ptr<MayaObject> parentObject); // new, parse whole scene as hierarchy and save/analyze objects
    bool parseScene();
    bool renderingStarted;
    bool parseInstancerNew(); // parse only particle instancer nodes, its a bit more complex

    MDagPath uiCamera;

    MFn::Type updateElement;
    bool updateScene(); // update all necessary objects
    bool updateScene(MFn::Type updateElement); // update all necessary objects
    bool updateInstancer(); // update all necessary objects
    MString getExportPath(MString ext, MString rendererName);
    MString getFileName();

    void clearInstancerNodeList();
    bool lightObjectIsInLinkedLightList(boost::shared_ptr<MayaObject> lightObject, MDagPathArray& linkedLightsArray);
    void getLightLinking();
    bool listContainsAllLights(MDagPathArray& linkedLights, MDagPathArray& excludedLights);
    MDagPath getWorld();

    void setCurrentCamera(MDagPath camera);
    void checkParent(boost::shared_ptr<MayaObject> obj);

    void classifyMayaObject(boost::shared_ptr<MayaObject> obj);
    bool isGeo(MObject obj);
    bool isLight(MObject obj);

    void setRenderType(RenderType rtype);
    boost::shared_ptr<MayaObject> getObject(MObject obj);
    boost::shared_ptr<MayaObject> getObject(MDagPath dp);

    MayaScene();

    // interactive elements
    std::map<uint, InteractiveElement> interactiveUpdateMap;
    void updateInteraciveRenderScene(std::vector<InteractiveElement *> elementList);

};

#endif  // !MAYASCENE_H
