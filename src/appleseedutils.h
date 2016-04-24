
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

#ifndef APPLESEEDUTILS_H
#define APPLESEEDUTILS_H

#include "foundation/math/matrix.h"

#include "renderer/api/light.h"
#include "renderer/api/project.h"
#include "renderer/api/scene.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MMatrix.h>

// Forward declarations.
class MayaObject;
class MColor;

void defineDefaultMaterial(renderer::Project* project);
MString getAssemblyName(const MayaObject* obj);
MString getAssemblyInstanceName(const MayaObject* obj);
MString getObjectInstanceName(const MayaObject* obj);
MString getObjectName(const MayaObject* obj);
void defineScene(renderer::Project* project);
renderer::Scene* getSceneFromProject(renderer::Project* project);
MayaObject* getAssemblyMayaObject(const MayaObject* obj);
renderer::Assembly* getAssembly(const MayaObject* obj);
void defineMasterAssembly(renderer::Project* project);
renderer::Assembly* getMasterAssemblyFromProject(renderer::Project* project);
renderer::Assembly* createAssembly(const MayaObject* obj);
renderer::AssemblyInstance* getAssemblyInstance(const MayaObject* obj);
renderer::AssemblyInstance* createAssemblyInstance(const MayaObject* obj);
void MMatrixToAMatrix(MMatrix& mayaMatrix, foundation::Matrix4d& appleMatrix);

void fillTransformMatrices(const MayaObject* obj, renderer::Light* assInstance);
void fillTransformMatrices(MMatrix matrix, renderer::AssemblyInstance* assInstance);
void fillMatrices(const MayaObject* obj, renderer::TransformSequence& transformSequence);

void defineColor(renderer::Project* project, const char* name, const MColor color, const float intensity, MString colorSpace = "srgb");
MString colorOrMap(renderer::Project* project, MFnDependencyNode& shaderNode, MString& attributeName);
void removeTextureEntityIfItExists(MString& textureName);
MString defineTexture(MFnDependencyNode& shader, MString& attributeName);
void addVisibilityFlags(boost::shared_ptr<MayaObject> obj, renderer::ParamArray& paramArray);

#endif  // !APPLESEEDUTILS_H
