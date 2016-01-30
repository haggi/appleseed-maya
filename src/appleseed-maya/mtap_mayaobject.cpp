
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

#include "mtap_mayaobject.h"
#include <maya/MPlugArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPathArray.h>

#include "utilities/logging.h"
#include "utilities/tools.h"
#include "appleseedutils.h"
#include "definitions.h"
#include "world.h"


static Logging logger;

mtap_ObjectAttributes::mtap_ObjectAttributes()
{
    needsOwnAssembly = false;
    objectMatrix.setToIdentity();
    this->assemblyObject = nullptr;
}

mtap_ObjectAttributes::mtap_ObjectAttributes(sharedPtr<ObjectAttributes> otherAttr)
{
    sharedPtr<mtap_ObjectAttributes> other = staticPtrCast<mtap_ObjectAttributes>(otherAttr);

    this->hasInstancerConnection = false;
    objectMatrix.setToIdentity();
    this->assemblyObject = nullptr;

    if (other)
    {
        hasInstancerConnection = other->hasInstancerConnection;
        objectMatrix = other->objectMatrix;
        assemblyObject = other->assemblyObject;
    }
};

mtap_MayaObject::mtap_MayaObject(MObject& mobject) : MayaObject(mobject)
{
}

mtap_MayaObject::mtap_MayaObject(MDagPath& mobject) : MayaObject(mobject)
{
}

mtap_MayaObject::~mtap_MayaObject()
{}


bool mtap_MayaObject::geometryShapeSupported()
{
    MFn::Type type = this->mobject.apiType();
    if (this->mobject.hasFn(MFn::kMesh))
        return true;

    if (this->isLight())
        return true;

    if (this->isCamera())
        return true;

    return false;
}

//
//  The purpose of this method is to compare object attributes and inherit them if appropriate.
//  e.g. lets say we assign a color to the top node of a hierarchy. Then all child nodes will be
//  called and this method is used.
//
sharedPtr<ObjectAttributes>mtap_MayaObject::getObjectAttributes(sharedPtr<ObjectAttributes> parentAttributes)
{
    sharedPtr<mtap_ObjectAttributes> myAttributes = sharedPtr<mtap_ObjectAttributes>(new mtap_ObjectAttributes(parentAttributes));

    if (this->hasInstancerConnection)
    {
        myAttributes->hasInstancerConnection = true;
    }

    if (this->isGeo())
    {
    }

    if (this->isTransform())
    {
        MFnDagNode objNode(this->mobject);
        myAttributes->objectMatrix = objNode.transformationMatrix() * myAttributes->objectMatrix;
    }

    if (this->needsAssembly() || myAttributes->hasInstancerConnection)
    {
        myAttributes->needsOwnAssembly = true;
        myAttributes->assemblyObject = this;
        myAttributes->objectMatrix.setToIdentity();
    }

    this->attributes = myAttributes;
    return myAttributes;
}


void mtap_MayaObject::createAssembly()
{
    // instances do not need own assembly.
    if (this->instanceNumber > 0)
        return;

    asf::auto_release_ptr<asr::Assembly> assembly(asr::AssemblyFactory().create(this->fullName.asChar(),asr::ParamArray()));
}
//
// objects needs own assembly if:
//      - it is instanced
//      - it is an animated transform
//      - its polysize is large (not yet implemented)
//

bool mtap_MayaObject::needsAssembly()
{
    // Normally only a few nodes would need a own assembly.
    // In IPR we have an update problem: If in a hierarchy a transform node is manipulated,
    // there is no way to find out that a geometry node below has to be updated, at least I don't know any.
    // Maybe I have to parse the hierarchy below and check the nodes for a geometry/camera/light node.
    // So at the moment I let all transform nodes receive their own transforms. This will result in a
    // translation of the complete hierarchy as assemblies/assembly instances.
    if (MayaTo::getWorldPtr()->renderType == MayaTo::MayaToWorld::IPRRENDER)
    {
        if (this->isTransform())
        {
            return true;
        }
    }

    // this is the root of all assemblies
    if (this->mobject.hasFn(MFn::kWorld))
        return true;

    if (this->instanceNumber > 0)
        return false;

    if (this->hasInstancerConnection)
    {
        Logging::debug(MString("obj has instancer connection -> needs assembly."));
        return true;
    }

    if (this->isInstanced())
    {
        Logging::debug(MString("obj has more than 1 parent -> needs assembly."));
        return true;
    }

    if (this->isObjAnimated())
    {
        Logging::debug(MString("Object is animated -> needs assembly."));
        return true;
    }

    if (isLightTransform(this->dagPath))
    {
        Logging::debug(MString("Object is light transform -> needs assembly."));
        return true;
    }

    return false;
}
