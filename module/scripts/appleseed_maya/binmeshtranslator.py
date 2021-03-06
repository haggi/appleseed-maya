
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
import pymel.core as pm
import maya.OpenMayaMPx as OpenMayaMPx
import sys

log = logging.getLogger("mtapLogger")

def binMeshTranslatorOpts(parent, action, initialSettings, resultCallback):
    useTransform = True
    oneFilePerMesh = False
    createProxies = True
    proxyRes = 0.1
    exportDir = ""

    if initialSettings is not None and len(initialSettings) > 0:
        #oneFilePerMesh=0;createProxies=1;proxyRes=0.1;exportDir=;createProxies=1
        opts = initialSettings.split(";")
        for opt in opts:
            name, value = opt.split("=")
            if name == "oneFilePerMesh":
                oneFilePerMesh = int(value)
            if name == "createProxies":
                createProxies = int(value)
            if name == "useTransform":
                useTransform = int(value)
            if name == "proxyRes":
                proxyRes = float(proxyRes)

    if action == "post":
        pm.setParent(parent)
        with pm.columnLayout(adj = True):
            pm.checkBox("MSH_OPTS_DOTRANSFORM", label = "Use Transform", v=useTransform)
            pm.checkBox("MSH_OPTS_ONEFILE", label = "One File Per Mesh", v=oneFilePerMesh)
            pm.checkBox("MSH_OPTS_DOPROX", label = "Create ProxyFiles", v=createProxies)
            pm.floatFieldGrp("MSH_OPTS_PROXPERC", label="Proxy Resolution", v1 = proxyRes)

    if action == "query":
        resultOptions = ""
        oneFilePerMesh = pm.checkBox("MSH_OPTS_ONEFILE", query=True, v=True)
        resultOptions += "oneFilePerMesh={0}".format(int(oneFilePerMesh))
        doProx = pm.checkBox("MSH_OPTS_DOPROX", query=True, v=True)
        resultOptions += ";createProxies={0}".format(int(doProx))
        proxyRes = pm.floatFieldGrp("MSH_OPTS_PROXPERC", query=True, v1 = True)
        resultOptions += ";proxyRes={0}".format(proxyRes)
        doTransform = pm.checkBox("MSH_OPTS_DOTRANSFORM", query=True, v=True)
        resultOptions += ";useTransform={0}".format(int(doTransform))
        melCmd = '{0} "{1}"'.format(resultCallback,resultOptions)
        pm.mel.eval(melCmd)
    return 1

def binMeshTranslatorWrite(fileName, optionString, accessMode):
    exportPath = fileName
    createProxies = False
    proxyRes = 0.1
    all = False
    oneFilePerMesh = False
    useTransform = False

    # the very first sign is a ; so the first element cannot be split again, no idea why
    opts = optionString.split(";")
    for opt in opts:
        try:
            name, value = opt.split("=")
            if name == "oneFilePerMesh":
                oneFilePerMesh = int(value)
            if name == "createProxies":
                createProxies = int(value)
            if name == "useTransform":
                useTransform = int(value)
            if name == "proxyRes":
                proxyRes = float(proxyRes)
        except:
            pass

    if accessMode == "selected":
        selection = []
        for object in pm.ls(sl=True):
            selection.extend(object.getChildren(ad=True, type="mesh"))
        if len(selection) == 0:
            raise
        pm.binMeshWriterCmd(selection, path=exportPath, doProxy=createProxies, percentage=proxyRes, doTransform=useTransform, oneFilePerMesh=oneFilePerMesh)

    if accessMode == "all":
        pm.binMeshWriterCmd(path=exportPath, doProxy=createProxies, percentage=proxyRes, doTransform=useTransform, all=True, oneFilePerMesh=oneFilePerMesh)
    return True

def binMeshTranslatorRead(fileName, optionString, accessMode):
    importPath = fileName
    createProxies = False
    proxyRes = 0.1
    # the very first sign is a ; so the first element cannot be split again, no idea why
    opts = optionString.split(";")
    for opt in opts:
        try:
            name, value = opt.split("=")
            if name == "createProxies":
                createProxies = int(value)
            if name == "proxyRes":
                proxyRes = float(proxyRes)
        except:
            pass
    pm.binMeshReaderCmd(path=importPath)
    return True

def binMeshCheckAndCreateShadingGroup(shadingGroup):
    try:
        shadingGroup = pm.PyNode(shadingGroup)
    except:
        shader = pm.shadingNode("appleseedSurfaceShader", asShader=True)
        shadingGroup = pm.sets(renderable=True, noSurfaceShader=True, empty=True, name=shadingGroup)
        shader.outColor >> shadingGroup.surfaceShader

# I'd prefer to use the connected polyshape directly, but because of the connections in a multi shader assignenment
# I do not get a directo connection from the creator node to the polymesh but it goes to some groupId nodes what makes
# it a bit complicated to search for the first connected mesh in the API. So I simply call the script with the creator node
# and let the python script search for the mesh.

def listConnections(node, allList, meshList):
    for con in node.outputs(sh=True):
        if not con in allList:
            allList.append(con)
            if con.type() == 'mesh':
                meshList.append(con)
            else:
                listConnections(con, allList, meshList)

def getConnectedPolyShape(creatorShape):
    try:
        creatorShape = pm.PyNode(creatorShape)
    except:
        return None

    meshList = []
    allList = []
    listConnections(creatorShape, allList, meshList)

    return meshList


# perFaceAssingments contains a double list, one face id list for every shading group
def binMeshAssignShader(creatorShape = None, shadingGroupList=[], perFaceAssingments=[]):
    if not creatorShape:
        return

    if pm.PyNode(creatorShape).type() != "mtap_standinMeshNode":
        log.error("binMeshAssignShader: Node {0} is not a creator shape".format(creatorShape))
        return
    polyShapeList = getConnectedPolyShape(creatorShape)

    for polyShape in polyShapeList:
        for index, shadingGroup in enumerate(shadingGroupList):
            binMeshCheckAndCreateShadingGroup(shadingGroup)

            assignments = perFaceAssingments[index]
            print assignments
            faceSelection = []

            for polyId in assignments:
                faceSelection.append(polyId)

            fs = polyShape.f[faceSelection]
            pm.sets(shadingGroup, e=True, forceElement=str(fs))
