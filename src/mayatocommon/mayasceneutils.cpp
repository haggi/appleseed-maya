
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
#include <maya/MGlobal.h>
#include <maya/MFileIO.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>
#include <maya/MFnDependencyNode.h>
#include "utilities/pystring.h"
#include "utilities/logging.h"
#include "utilities/attrtools.h"
#include "utilities/tools.h"
#include "world.h"


sharedPtr<MayaObject> MayaScene::getObject(MObject obj)
{
    sharedPtr<MayaObject> mo = 0;
    size_t numobjects = this->objectList.size();
    for (size_t objId = 0; objId < numobjects; objId++)
    {
        if (this->objectList[objId]->mobject == obj)
            return this->objectList[objId];
    }
    return mo;
}

sharedPtr<MayaObject> MayaScene::getObject(MDagPath dp)
{
    sharedPtr<MayaObject> mo = 0;
    size_t numobjects = this->objectList.size();
    for (size_t objId = 0; objId < numobjects; objId++)
    {
        if (this->objectList[objId]->dagPath == dp)
            return this->objectList[objId];
    }
    return mo;
}

void MayaScene::clearObjList(std::vector<sharedPtr<MayaObject> > & objList)
{
    objList.clear();
}

void MayaScene::clearObjList(std::vector<sharedPtr<MayaObject> > & objList, sharedPtr<MayaObject> notThisOne)
{
    size_t numElements = objList.size();
    sharedPtr<MayaObject> tmpCopy;
    for (size_t i = 0; i < numElements; i++)
    {
        if (objList[i] == notThisOne)
            tmpCopy = objList[i];
    }
    objList.clear();
    objList.push_back(notThisOne);
    notThisOne->index = 0;
}

// the camera from the UI is set via render command
void MayaScene::setCurrentCamera(MDagPath camDagPath)
{
    this->uiCamera = camDagPath;
}

std::vector<sharedPtr<MayaObject> >  parentList;

void MayaScene::checkParent(sharedPtr<MayaObject> obj)
{
    std::vector<sharedPtr<MayaObject> >::iterator iter;
    MFnDagNode node(obj->mobject);
    if (node.parentCount() == 0)
    {
        obj->parent = 0;
        return;
    }
    MObject parent = node.parent(0);
    for (iter = parentList.begin(); iter != parentList.end(); iter++)
    {
        if (parent == (*iter)->mobject)
        {
            obj->parent = *iter;
            break;
        }
    }
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

    MFnDependencyNode depFn(obj);
    uint nodeId = depFn.typeId().id();
    for (uint lId = 0; lId < this->objectIdentifier.size(); lId++)
    {
        if (nodeId == this->objectIdentifier[lId])
        {
            Logging::debug(MString("Found external objtype: ") + depFn.name());
            return true;
        }
    }

    return false;
}

bool MayaScene::isLight(MObject obj)
{
    if (obj.hasFn(MFn::kLight))
        return true;

    MFnDependencyNode depFn(obj);
    uint nodeId = depFn.typeId().id();
    for (uint lId = 0; lId < this->lightIdentifier.size(); lId++)
    {
        if (nodeId == this->lightIdentifier[lId])
        {
            Logging::debug(MString("Found external lighttype: ") + depFn.name());
            return true;
        }
    }
    return false;
}

void  MayaScene::classifyMayaObject(sharedPtr<MayaObject> obj)
{
    if (isCamera(obj->mobject))
    {
        this->camList.push_back(obj);
        return;
    }

    if (this->isLight(obj->mobject))
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
    MString scenePath = MayaTo::getWorldPtr()->worldRenderGlobalsPtr->basePath + "/" + rendererName + "/" + currentFile.c_str() + "." + ext;
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

bool MayaScene::canDoIPR()
{
    return this->cando_ipr;
}

void MayaScene::setRenderType(RenderType rtype)
{
    this->renderType = rtype;
}
