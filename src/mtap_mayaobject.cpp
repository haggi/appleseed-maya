
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
#include "world.h"

mtap_ObjectAttributes::mtap_ObjectAttributes()
{
    needsOwnAssembly = false;
    objectMatrix.setToIdentity();
    assemblyObject = 0;
}

mtap_ObjectAttributes::mtap_ObjectAttributes(boost::shared_ptr<ObjectAttributes> otherAttr)
{
    boost::shared_ptr<mtap_ObjectAttributes> other = boost::static_pointer_cast<mtap_ObjectAttributes>(otherAttr);

    this->hasInstancerConnection = false;
    objectMatrix.setToIdentity();
    assemblyObject = 0;

    if (other)
    {
        hasInstancerConnection = other->hasInstancerConnection;
        objectMatrix = other->objectMatrix;
        assemblyObject = other->assemblyObject;
    }
}

