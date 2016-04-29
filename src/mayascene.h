
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

// appleseed-maya headers.
#include "mayaobject.h"

// Maya headers.
#include <maya/MDagPath.h>
#include <maya/MMessage.h>
#include <maya/MObject.h>

// Standard headers.
#include <map>
#include <vector>

// Forward declarations.
class MDagPathArray;

class EditableElement
{
  public:
    boost::shared_ptr<MayaObject> mayaObject;
    MObject mobj;
    MString name;
    MObject node;

    bool isDirty;
    bool isTransformed;     // to recognize if we have to update the shape or only the instance transform
    bool isDeformed;        // to recognize if a geometry is deformed, we can have both, deform and transform

    EditableElement()
      : isDirty(false)
      , isTransformed(false)
      , isDeformed(false)
    {
    }
};

class MayaScene
{
  public:
    std::vector<boost::shared_ptr<MayaObject> > objectList;
    std::vector<boost::shared_ptr<MayaObject> > camList;
    std::vector<boost::shared_ptr<MayaObject> > lightList;
    std::vector<boost::shared_ptr<MayaObject> > instancerNodeElements; // so its easier to update them

    // This map allows node callbacks to retrieve MayaObjects associated to nodes.
    typedef std::map<MCallbackId, EditableElement> EditableElementContainer;
    EditableElementContainer editableElements;

    // Was at least one editable element marked as dirty?
    bool isAnyDirty;

    MDagPath uiCamera;

    MayaScene();

    bool parseSceneHierarchy(MDagPath currentObject, int level, boost::shared_ptr<ObjectAttributes> attr, boost::shared_ptr<MayaObject> parentObject);
    bool parseScene();
    bool updateScene();

  private:
    std::vector<boost::shared_ptr<MayaObject> > origObjects;
    std::vector<MDagPath> instancerDagPathList;

    void getLightLinking();
    bool lightObjectIsInLinkedLightList(boost::shared_ptr<MayaObject> lightObject, MDagPathArray& linkedLightsArray);
    bool updateInstancer(); // update all necessary objects
    bool updateScene(MFn::Type updateElement); // update all necessary objects
    bool parseInstancerNew(); // parse only particle instancer nodes, its a bit more complex
};

#endif  // !MAYASCENE_H
