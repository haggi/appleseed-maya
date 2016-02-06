
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
log = logging.getLogger("mtapLogger")

def removeAutoShaderNodes():
    attr = pm.ls("*.AS_AUTO_SHADER")
    nodes = []
    materials = []
    for att in attr:
        nodes.append(att.node().name())
        if att.node().nodeType() == "asMaterial":
            materials.append(att.node())

    for mat in materials:
        origName = "_".join(mat.name().split("_")[:-1])
        try:
            shadingGroup = mat.outputs(type="shadingEngine")[0]
            log.debug("Reconnecting orig node " + origName + " to shding group " + shadingGroup)
            pm.PyNode(origName).outColor >> shadingGroup.surfaceShader
        except:
            log.debug("Reconnecting of orig node " + origName + " failed.")

    if len(nodes) > 0:
        pm.delete(nodes)

def createAsMaterialNode(shadingNode):
    mat = pm.createNode("asMaterial")
    mat.rename(str(shadingNode) + "_asMaterial")
    mat.addAttr("AS_AUTO_SHADER")
    return mat

def createAsPhysicalSurfaceNode(shadingNode):
    psh = pm.createNode("asPhysical_surface_shader")
    psh.rename(str(shadingNode) + "_asPhysicalSurface")
    psh.addAttr("AS_AUTO_SHADER")
    return psh

def createAsBsdfNode(shadingNode):
    bsdfDict = {'lambert' : "asLambertian_brdf",
                'phong'   : "asSpecular_brdf"}

    try:
        bsdf = pm.createNode(bsdfDict[shadingNode.nodeType()])
    except:
        log.debug("Unable to create node for {0}".format(shadingNode))
        raise

    if len(shadingNode.color.inputs()) > 0:
        shadingNode.color.inputs(p=True)[0] >> bsdf.reflectance
    else:
        bsdf.reflectance.set(shadingNode.color.get())

    #bsdf.reflectance
    bsdf.rename(str(shadingNode) + "_bsdf")
    bsdf.addAttr("AS_AUTO_SHADER")
    return bsdf

def createAutoShaderNodes():
    removeAutoShaderNodes()

    asDependNodes = pm.pluginInfo("mayatoappleseed_maya2014", q=True, dependNode=True)
    shadingGroups = pm.ls(type="shadingEngine")
    supportedNodeTypes = ['lambert', 'phong']

    for shadingGroup in shadingGroups:
        if shadingGroup.name() == "initialParticleSE":
            continue

        log.debug("Check shadingGroup {0}".format(shadingGroup))

        inputs = shadingGroup.surfaceShader.inputs()
        for input in inputs:
            if input.nodeType() in asDependNodes:
                log.debug("Inputnode {0} is an appleseed shader.".format(input))
                continue

            if input.nodeType() in supportedNodeTypes:
                log.debug("Creating temp nodes for non appleseed nodeType {0}.".format(input.nodeType()))
                mat = createAsMaterialNode(input)
                psh = createAsPhysicalSurfaceNode(input)
                bsdf = createAsBsdfNode(input)
                psh.outColor >> mat.surface_shader
                bsdf.outColor >> mat.bsdf
                mat.outColor >> shadingGroup.surfaceShader
                print "Connections of mat", mat.inputs()
