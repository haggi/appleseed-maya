
#
# This source file is part of appleseed.
# Visit http://appleseedhq.net/ for additional information and resources.
#
# This software is released under the MIT license.
#
# Copyright (c) 2016 Haggi Krey, The appleseedhq Organization
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import appleseed.osltools as osltools

import pymel.core as pm

import logging

log = logging.getLogger("ui")

class BaseTemplate(pm.ui.AETemplate):

    def addControl(self, control, label=None, **kwargs):
        pm.ui.AETemplate.addControl(self, control, label=label, **kwargs)

    def beginLayout(self, name, collapse=True):
        pm.ui.AETemplate.beginLayout(self, name, collapse=collapse)

class AEuberShaderTemplate(BaseTemplate):
    def __init__(self, nodeName):
        osltools.readShadersXMLDescription()
        self.shaderDesc = {}
        if osltools.SHADER_DICT.has_key('uberShader'):
            inDesc = osltools.SHADER_DICT['uberShader']['inputs']
            for i in inDesc:
                self.shaderDesc[i['name']] = i['help']
        BaseTemplate.__init__(self, nodeName)
        pm.mel.AEswatchDisplay(nodeName)
        self.thisNode = None
        self.node = pm.PyNode(self.nodeName)
        self.beginScrollLayout()
        self.buildBody(nodeName)
        self.addExtraControls("ExtraControls")
        self.endScrollLayout()

    def update(self, nodeName):
        self.thisNode = pm.PyNode(nodeName)
        reflectionEDF = self.thisNode.reflectionMdf.getEnums().keys()[self.thisNode.reflectionMdf.get()]
        self.dimControl(self.thisNode, "roughness1", False)
        if reflectionEDF == "sharp":
            self.dimControl(self.thisNode, "roughness1", True)

        refractionMDF = self.thisNode.refractionMdf.getEnums().keys()[self.thisNode.refractionMdf.get()]
        self.dimControl(self.thisNode, "refractionRoughness", False)
        if refractionMDF == "sharp":
            self.dimControl(self.thisNode, "refractionRoughness", True)

        self.dimControl(self.thisNode, "SSS1Color", True)
        self.dimControl(self.thisNode, "SSS1RadiusMultiplier", True)
        if self.thisNode.SSS1.get():
            self.dimControl(self.thisNode, "SSS1Color", False)
            self.dimControl(self.thisNode, "SSS1RadiusMultiplier", False)

    def updateSpec(self, nodeName):
        self.thisNode = pm.PyNode(nodeName)

    def enableSSS(self, nodeName):
        self.thisNode = pm.PyNode(nodeName)

    def bumpNew(self, attrName):
        pm.setUITemplate('attributeEditorTemplate', pst=True)
        pm.attrNavigationControlGrp('bumpControl', label="Bump Mapping", at=attrName)
        pm.setUITemplate(ppt=True)

    def bumpReplace(self, attrName):
        pm.attrNavigationControlGrp('bumpControl', edit=True, at=attrName)

    def buildBody(self, nodeName):
        self.thisNode = pm.PyNode(nodeName)
        self.templateUI = pm.setParent(query=True)
        anno = ""
        self.beginLayout("Matte", collapse=True)
        if self.shaderDesc.has_key('matte'):
            anno = self.shaderDesc['matte']
        self.addControl("matte", label="Matte Shader", annotation = anno)
        self.endLayout()
        self.beginLayout("Diffuse", collapse=False)
        if self.shaderDesc.has_key('diffuseColor'):
            anno = self.shaderDesc['diffuseColor']
        self.addControl("diffuseColor", label="Diffuse Color", annotation = anno)
        if self.shaderDesc.has_key('diffuseValue'):
            anno = self.shaderDesc['diffuseValue']
        self.addControl("diffuseValue", label="Diffuse Value", annotation = anno)
        if self.shaderDesc.has_key('roughness'):
            anno = self.shaderDesc['roughness']
        print "AETemplate roughness anno", anno
        self.addControl("roughness", label="Roughness", annotation = anno)
        self.addSeparator()
        if self.shaderDesc.has_key('translucency'):
            anno = self.shaderDesc['translucency']
        self.addControl("translucency", label="Translucency", changeCommand=self.update)
        if self.shaderDesc.has_key('translucencyColor'):
            anno = self.shaderDesc['translucencyColor']
        self.addControl("translucencyColor", label="Translucence Color")
        self.addSeparator()
        if self.shaderDesc.has_key('emissionColor'):
            anno = self.shaderDesc['emissionColor']
        self.addControl("emissionColor", label="Emission Color", changeCommand=self.update)
        if self.shaderDesc.has_key('emissionMultiplier'):
            anno = self.shaderDesc['emissionMultiplier']
        self.addControl("emissionMultiplier", label="Emission Color Multiplier")
        self.endLayout()

        self.beginLayout("Specular", collapse=False)
        self.addControl("specular1", label="Specularity", changeCommand=self.update)
        self.addControl("specularColor1", label="Specular Color", changeCommand=self.update)
        #self.addControl("specularIOR1", label="Specular IOR", changeCommand=self.update)
        self.addControl("roughness1", label="Roughness", changeCommand=self.update)
        self.addControl("anisotropy1", label="Anisotropy", changeCommand=self.update)
        if self.shaderDesc.has_key('reflectionMdf'):
            anno = self.shaderDesc['reflectionMdf']
        self.addControl("reflectionMdf", label="Reflection MDF", changeCommand=self.update)
        self.addSeparator()
        self.addControl("specularUseFresnel1", label="Use Fresnel", changeCommand=self.update)
        self.addControl("reflectivityAtZeroDeg", label="Reflectivity at 0 Deg", changeCommand=self.update)
        self.endLayout()

        self.beginLayout("Refraction", collapse=False)
        self.addControl("refractivity", label="Refractivity", changeCommand=self.update)
        self.addControl("refractionColor", label="Refraction Color", changeCommand=self.update)
        self.addControl("refractionIOR", label="Refraction Index", changeCommand=self.update)
        self.addControl("refractionRoughness", label="Refraction Roughness", changeCommand=self.update)
        if self.shaderDesc.has_key('refractionMdf'):
            anno = self.shaderDesc['refractionMdf']
        self.addControl("refractionMdf", label="Refraction MDF", changeCommand=self.update)
        self.addSeparator()
        self.addControl("absorption", label="Absorption", changeCommand=self.update)
        self.addControl("absorptionColor", label="Absorption Color", changeCommand=self.update)
        self.addSeparator()
        self.endLayout()

        self.beginLayout("SSS", collapse=False)
        self.addControl("SSS1", label="Enable SSS", changeCommand=self.update)
        self.addControl("SSS1Color", label="SSS Color", changeCommand=self.update)
        self.addControl("SSS1RadiusMultiplier", label="Radius", changeCommand=self.update)
        self.endLayout()

        self.update(nodeName)
