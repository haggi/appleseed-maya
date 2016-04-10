
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

import pymel.core as pm
import logging

log = logging.getLogger("ui")

class BaseTemplate(pm.ui.AETemplate):

    def addControl(self, control, label=None, **kwargs):
        pm.ui.AETemplate.addControl(self, control, label=label, **kwargs)

    def beginLayout(self, name, collapse=True):
        pm.ui.AETemplate.beginLayout(self, name, collapse=collapse)

class AEasDisneyMaterialTemplate(BaseTemplate):
    def __init__(self, nodeName):
        BaseTemplate.__init__(self,nodeName)
        self.thisNode = None
        self.node = pm.PyNode(self.nodeName)
        pm.mel.AEswatchDisplay(nodeName)
        self.beginScrollLayout()
        self.buildBody(nodeName)
        self.addExtraControls("ExtraControls")
        self.endScrollLayout()

    def bumpNew(self, attribute):
        pm.setUITemplate("attributeEditorTemplate", pushTemplate=True)
        attName = attribute
        self.bumpCtrl = pm.attrNavigationControlGrp(attribute=attribute, label="Bump Map")
        pm.setUITemplate("attributeEditorTemplate", popTemplate=True)

    def bumpReplace(self, attribute):
        if self.bumpCtrl is not None:
            pm.attrNavigationControlGrp(self.bumpCtrl, edit=True, attribute=attribute)

    def buildBody(self, nodeName):
        self.thisNode = pm.PyNode(nodeName)
        self.beginLayout("ShaderSettings" ,collapse=0)
        self.addControl("BaseColor", label="Base Color")
        self.addControl("Subsurface", label="Subsurface")
        self.addControl("Metallic", label="Metallic")
        self.addControl("Specular", label="Specular")
        self.addControl("SpecularTint", label="SpecularTint")
        self.addControl("Anisotropic", label="Anisotropic")
        self.addControl("Roughness", label="Roughness")
        self.addControl("Sheen", label="Sheen")
        self.addControl("SheenTint", label="SheenTint")
        self.addControl("Clearcoat", label="Clearcoat")
        self.addControl("ClearcoatGloss", label="Clearcoat Gloss")
        self.callCustom(self.bumpNew, self.bumpReplace,"normalCamera")
        self.endLayout()
