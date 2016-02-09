
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

import logging
import sys
import maya.OpenMaya as om
import maya.OpenMayaMPx as OpenMayaMPx
import renderer.osltools as osltools
import metashadernode as meta

# to get a unused id for on-the-fly nodes,  try to find an unused id
# the LAST_ID is the official last ID (0x0 - 0x7FFFF) for site internal plugins
# I use the very last one and count down.

LAST_ID = 0x7FFFF
REGISTERED_NODES = {}
log = logging.getLogger("renderLogger")

def getNextValidNodeId():
    global LAST_ID
    validId = LAST_ID
    while True:
        nodeClass = om.MNodeClass(om.MTypeId(LAST_ID))
        if nodeClass.typeName() == "":
            validId = LAST_ID
            break
        else:
            log.info("Node id {0} already used by: {1}. Trying next one.".format(LAST_ID, nodeClass.typeName()))
        LAST_ID -= 1
    return validId

def registerShadingNodes(plugin):
    global REGISTERED_NODES
    global LAST_ID
    log.info("createShadingNodes")
    shadersDict = osltools.readShadersXMLDescription()
    classifications = set()
    for key in shadersDict:
        if OpenMayaMPx.MFnPlugin.isNodeRegistered(key):
            #log.info("shading node {0} is already registered, skipping.".format(key))
            continue

        # at the moment we only care about uberShader for testing
        if key == "uberShader":
            classification = "shader/surface"
            nodeId = om.MTypeId(LAST_ID)
            shaderDict = shadersDict[key]
            if shaderDict.has_key("mayaClassification") and shaderDict['mayaClassification'] is not "":
                classification = shaderDict['mayaClassification']
            classifications.add(classification)
            if shaderDict.has_key('mayaId') and int(shaderDict['mayaId']) != 0:
                nodeId = om.MTypeId(shaderDict['mayaId'])
            else:
                id = getNextValidNodeId()
                log.info("Shader node does not have an id. Using temporary id {0}".format(id))
                nodeId = om.MTypeId(id)
                shaderDict['mayaId'] = nodeId.id()
            shaderNodeName = key
            log.info("registering shader: {0} with classification {1} and id {2}".format(shaderNodeName, classification, nodeId.id()))
            X = type(shaderNodeName, (meta.MetaClass,), dict(data=shaderDict))
            plugin.registerNode( X.__name__, nodeId, X.nodeCreator, X.nodeInitializer, OpenMayaMPx.MPxNode.kDependNode, classification)
            REGISTERED_NODES[shaderNodeName] = nodeId

    for classification in classifications:
        postCmd = "if( `window -exists createRenderNodeWindow` )  {refreshCreateRenderNodeWindow(\"%s\");}\n" % classification
        om.MGlobal.executeCommand(postCmd)


def unregisterShadingNodes(plugin):
    global REGISTERED_NODES
    for key in REGISTERED_NODES.keys():
        nodeId = om.MTypeId(REGISTERED_NODES[key])
        log.info("unregistering node: {0} with id {1}".format(key, nodeId.id()))
        plugin.deregisterNode(nodeId)
