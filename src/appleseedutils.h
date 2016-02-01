
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

#ifndef APPLESEED_UTILS_H
#define APPLESEED_UTILS_H

#include "renderer/api/light.h"
#include "renderer/api/scene.h"
#include "renderer/api/project.h"

#include "foundation/math/matrix.h"

#include <maya/MFnDependencyNode.h>

#include "mtap_mayaobject.h"
#include <maya/MMatrix.h>

namespace asr = renderer;
namespace asf = foundation;

void defineDefaultMaterial(asr::Project *project);
MString getAssemblyName(MayaObject *obj);
MString getAssemblyInstanceName(MayaObject *obj);
MString getObjectInstanceName(MayaObject *obj);
inline MString getObjectName(MayaObject *obj){ return obj->dagPath.fullPathName(); }
void defineScene(asr::Project *project);
asr::Scene *getSceneFromProject(asr::Project *project);
MayaObject *getAssemblyMayaObject(MayaObject *obj);
asr::Assembly *getCreateObjectAssembly(sharedPtr<MayaObject> obj);
void defineMasterAssembly(asr::Project *project);
asr::Assembly *getMasterAssemblyFromProject(asr::Project *project);
asr::Assembly *getSceneAssemblyFromProject(asr::Project *project);
asr::AssemblyInstance *getExistingObjectAssemblyInstance(MayaObject *obj);
void MMatrixToAMatrix(MMatrix& mayaMatrix, asf::Matrix4d& appleMatrix);
asf::Matrix4d MMatrixToAMatrix(MMatrix& mayaMatrix);
asf::Matrix4d MMatrixToAMatrix(MMatrix mayaMatrix);

void fillTransformMatrices(sharedPtr<MayaObject> obj, asr::Light *assInstance);
void fillTransformMatrices(MMatrix matrix, asr::AssemblyInstance *assInstance);
void fillMatrices(sharedPtr<MayaObject> obj, asr::TransformSequence& transformSequence);

void mayaColorToFloat(const MColor col, const float *floatCol, float *alpha);
void removeColorEntityIfItExists(const MString colorName);
void defineColor(asr::Project *project, const char *name, const MColor color, const float intensity, MString colorSpace = "srgb");
MString colorOrMap(asr::Project *project, MFnDependencyNode& shaderNode, MString& attributeName);
void removeTextureEntityIfItExists(MString& textureName);
MString defineTexture(MFnDependencyNode& shader, MString& attributeName);
void addVisibilityFlags(sharedPtr<MayaObject> obj, asr::ParamArray& paramArray);
void addVisibilityFlags(MObject& obj, asr::ParamArray& paramArray);

#endif
