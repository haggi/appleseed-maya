
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

def getEnumList(attr):
    return [(i, v) for i,v in enumerate(attr.getEnums().keys())]

def addUIElement(uiType, attribute, uiLabel, callback, renderGlobalsNodeName):
    ui = None
    if uiType == 'bool':
        ui = pm.checkBoxGrp(label=uiLabel)
        if callback is not None:
            pm.checkBoxGrp(ui, edit=True, cc=callback)
    if uiType == 'int':
        ui = pm.intFieldGrp(label=uiLabel, numberOfFields = 1)
        if callback is not None:
            pm.intFieldGrp(ui, edit=True, cc = callback)
    if uiType == 'float':
        ui = pm.floatFieldGrp(label=uiLabel, numberOfFields = 1)
        if callback is not None:
            pm.floatFieldGrp(ui, edit=True, cc= callback)
    if uiType == 'enum':
        ui = pm.attrEnumOptionMenuGrp(label = uiLabel, at=attribute, ei = getEnumList(attribute))
        # attrEnumOptionGrp has no cc callback, so I create a script job
        if callback is not None:
            attribute = pm.Attribute(renderGlobalsNodeName + "." + attribute)
            pm.scriptJob(attributeChange=[attribute, callback], parent=ui)
    if uiType == 'color':
        ui = pm.attrColorSliderGrp(label=uiLabel, at=attribute)
    if uiType == 'string':
        ui = pm.textFieldGrp(label=uiLabel)
        if callback is not None:
            pm.textFieldGrp(ui, edit=True, cc=callback)
    if uiType == 'vector':
        ui = pm.floatFieldGrp(label=uiLabel, nf=3)
        if callback is not None:
            pm.floatFieldGrp(ui, edit=True, cc=callback)
    return ui

def connectUIElement(uiElement, attribute):

    if attribute.type() == 'color':
        #color is automatically connnected via attrEnumOptionMenu
        return
    if attribute.type() == 'enum':
        #enum is automatically connnected via attrEnumOptionMenu
        return
    if attribute.type() == 'float3':
        #float3 == color is automatically connnected via attrColorSliderGrp
        return
    if attribute.type() == 'message':
        #no automatic connection necessary, will be controlled by other scritps
        return
    if attribute.type() == 'vector':
        #no automatic connection necessary, will be controlled by other scritps
        return
    pm.connectControl(uiElement, attribute, index = 2)

def addRenderGlobalsUIElement(renderGlobalsNodeName = None, attName = None, uiType = None, displayName = None, default=None, data=None, uiDict=None, callback=None, anno=None):

    attribute = pm.Attribute(renderGlobalsNodeName + "." + attName)
    uiElement = addUIElement(uiType, attribute, displayName, callback, renderGlobalsNodeName)
    if anno:
        uiElement.setAnnotation(anno)
    connectUIElement(uiElement, attribute)
    uiDict[attName] = uiElement
