
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


class AEappleseedNodeTemplate(BaseTemplate):
    def __init__(self, nodeName):
        BaseTemplate.__init__(self,nodeName)
        self.thisNode = None
        self.node = pm.PyNode(self.nodeName)
        self.buildBody(nodeName)
        log.debug("AEappleSeedNodeTemplate")

    def updateUI(self, nodeName):
        self.thisNode = pm.PyNode(nodeName)

        if self.thisNode.type() == "mesh":
            self.dimControl(self.thisNode, "mtap_ray_bias_distance", True)
            if self.thisNode .mtap_ray_bias_method.get() > 0:
                self.dimControl(self.thisNode, "mtap_ray_bias_distance", False)


    def buildAppleSeedTemplates(self, nodeName):
        self.thisNode = pm.PyNode(nodeName)

        if self.thisNode.type() == "camera":
            log.debug("AEappleSeedNodeTemplate:build camera AE")
            self.beginLayout("AppleSeed" ,collapse=1)
            self.addControl("mtap_cameraType", label="Camera Type")
            self.addControl("mtap_diaphragm_blades", label="Diaphragm Blades")
            self.addControl("mtap_diaphragm_tilt_angle", label="Diaphragm Tilt Angle")
            self.endLayout()

        if self.thisNode.type() == "mesh":
            self.beginLayout("AppleSeed" ,collapse=1)
            #self.addControl("mtap_mesh_useassembly", label="Use seperate Assembly")
            self.addControl("mtap_ray_bias_method", label="Ray Bias Method", changeCommand=self.updateUI)
            self.addControl("mtap_ray_bias_distance", label="Ray Bias Distance")
            #self.addControl("mtap_standin_path", label="Proxy File")
            self.addControl("mtap_visibleLights", label="Visible For Light Rays")
            self.addControl("mtap_visibleProbe", label="Visible For Probe Rays")
            self.addControl("mtap_visibleGlossy", label="Visible For Glossy Rays")
            self.addControl("mtap_visibleSpecular", label="Visible For Specular Rays")
            self.addControl("mtap_visibleDiffuse", label="Visible For Diffulse Rays")
            self.addControl("mtap_visibleTransparency", label="Visible For Transparent Rays")
            self.endLayout()

        if self.thisNode.type() == "spotLight":
            self.beginLayout("AppleSeed" ,collapse=1)
            self.addControl("mtap_cast_indirect_light", label="Cast Indirect Light")
            self.addControl("mtap_importance_multiplier", label="Importance Multiplier")
            self.endLayout()

        if self.thisNode.type() == "directionalLight":
            self.beginLayout("AppleSeed" ,collapse=1)
            self.addControl("mtap_cast_indirect_light", label="Cast Indirect Light")
            self.addControl("mtap_importance_multiplier", label="Importance Multiplier")
            self.endLayout()

        if self.thisNode.type() == "pointLight":
            self.beginLayout("AppleSeed" ,collapse=1)
            self.addControl("mtap_cast_indirect_light", label="Cast Indirect Light")
            self.addControl("mtap_importance_multiplier", label="Importance Multiplier")
            self.endLayout()

        if self.thisNode.type() == "areaLight":
            self.beginLayout("AppleSeed" ,collapse=1)
            self.addControl("primaryVisibility", label="Primary Visibility")
            self.addControl("castsShadows", label="Casts Shadows")
            self.addControl("mtap_visibleLights", label="Visible For Light Rays")
            self.addControl("mtap_visibleProbe", label="Visible For Probe Rays")
            self.addControl("mtap_visibleGlossy", label="Visible For Glossy Rays")
            self.addControl("mtap_visibleSpecular", label="Visible For Specular Rays")
            self.addControl("mtap_visibleDiffuse", label="Visible For Diffulse Rays")
            self.addControl("mtap_visibleTransparency", label="Visible For Transparent Rays")

            self.endLayout()

#        if self.thisNode.type() == "shadingEngine":
#            self.beginLayout("AppleSeed" ,collapse=1)
#            self.addControl("mtap_mat_bsdf", label="Bsdf")
#            self.addControl("mtap_mat_edf", label="Edf")
#            self.addControl("mtap_mat_surfaceShader", label="Surface Shader")
#            self.addControl("mtap_mat_alphaMap", label="Alpha Map")
#            self.addControl("mtap_mat_normalMap", label="Normal Map")
#            self.endLayout()

    def buildBody(self, nodeName):
        self.buildAppleSeedTemplates(nodeName)
        self.updateUI(nodeName)
