
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

import aenodetemplates as aet
import appleseedmenu as appleseedmenu
import logging
import optimizetextures
import os
import path
import pymel.core as pm
import renderer as renderer
import sys
import tempfile
import traceback

reload(renderer)

log = logging.getLogger("mtapLogger")

RENDERER_NAME = "Appleseed"

class AppleseedRenderer(renderer.MayaToRenderer):
    theRendererInstance = None
    @staticmethod
    def theRenderer(arg=None):
        if not AppleseedRenderer.theRendererInstance:
            AppleseedRenderer.theRendererInstance = AppleseedRenderer(RENDERER_NAME , __name__)
        return AppleseedRenderer.theRendererInstance

    def __init__(self, rendererName, moduleName):
        renderer.MayaToRenderer.__init__(self, rendererName, moduleName)
        self.rendererTabUiDict = {}
        self.aovShaders = ["mtap_aoShader", "mtap_aoVoxelShader", "mtap_diagnosticShader", "mtap_fastSSSShader", "appleseedSurfaceShader"]
        self.rendererMenu = None
        self.gMainProgressBar = None

    def createRendererMenu(self):
        self.rendererMenu = appleseedmenu.AppleseedMenu()

    def removeRendererMenu(self):
        if self.rendererMenu is not None:
            pm.deleteUI(self.rendererMenu)
        self.rendererMenu = None

    def getEnumList(self, attr):
        return [(i, v) for i, v in enumerate(attr.getEnums().keys())]

    def updateTest(self, dummy=None):
        pass
    
    def addUserTabs(self):
        pm.renderer(self.rendererName, edit=True, addGlobalsTab=self.renderTabMelProcedure("AOVs"))
        pm.renderer(self.rendererName, edit=True, addGlobalsTab=self.renderTabMelProcedure("Environment"))

    def updateEnvironment(self):
        envDict = self.rendererTabUiDict['environment']
        envType = self.renderGlobalsNode.environmentType.get()

        # Simply turn all off
        for key in envDict.keys():
            envDict[key].setEnable(False)
        envDict['environmentType'].setEnable(True)

        # Constant
        if envType == 0:
            envDict['environmentColor'].setEnable(True)
        # Gradient
        if envType == 1:
            envDict['gradientHorizon'].setEnable(True)
            envDict['gradientZenit'].setEnable(True)
        # Map
        if envType == 2:
            envDict['environmentMap'].setEnable(True)
            envDict['latlongVeShift'].setEnable(True)
            envDict['latlongHoShift'].setEnable(True)

        # SphericalMap
        if envType == 3:
            envDict['environmentMap'].setEnable(True)
            envDict['latlongVeShift'].setEnable(True)
            envDict['latlongHoShift'].setEnable(True)

        # MirrorBall
        if envType == 4:
            envDict['environmentMap'].setEnable(True)
            envDict['latlongVeShift'].setEnable(True)
            envDict['latlongHoShift'].setEnable(True)

        # Pyhsical Sky
        if envType == 5:
            envDict['pskModel'].setEnable(True)
            envDict['pskUsePhySun'].setEnable(True)
            envDict['pskSunExitMulti'].setEnable(True)

            if self.renderGlobalsNode.physicalSun.get():
                try:
                    sunConnection = self.renderGlobalsNode.physicalSunConnection.listConnections()[0]
                except:
                    try:
                        sunConnection = pm.ls("asSunLight")[0]
                        sunConnection.message >> self.renderGlobalsNode.physicalSunConnection
                    except:
                        lightShape = pm.createNode("directionalLight")
                        sunConnection = lightShape.getParent()
                        sunConnection.rename("asSunLight")
                        sunConnection.message >> self.renderGlobalsNode.physicalSunConnection
                envDict['pskPhySun'].setText(str(sunConnection))

            skyModel = self.renderGlobalsNode.skyModel.get()
            if skyModel == 1:  # hosek
                envDict['pskGrAlbedo'].setEnable(True)
            envDict['pskGrHShit'].setEnable(True)
            envDict['pskLumMulti'].setEnable(True)
            envDict['pskSatMulti'].setEnable(True)
            if not self.renderGlobalsNode.physicalSun.get():
                envDict['pskSunAzi'].setEnable(True)
                envDict['pskSunEle'].setEnable(True)
            envDict['pskTurb'].setEnable(True)
            envDict['pskTurbMax'].setEnable(True)
            envDict['pskTurbMin'].setEnable(True)

        # OSL
        if envType == 6:
            pass

    def AppleseedEnvironmentCreateTab(self):
        self.createGlobalsNode()
        parentForm = pm.setParent(query=True)
        pm.setUITemplate("attributeEditorTemplate", pushTemplate=True)
        scLo = self.rendererName + "AOScrollLayout"
        envDict = {}
        self.rendererTabUiDict['environment'] = envDict
        uiDict = envDict
        with pm.scrollLayout(scLo, horizontalScrollBarThickness=0):
            with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                with pm.frameLayout(label="Environment Lighting", collapsable=False):
                    with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                        attr = pm.Attribute(self.renderGlobalsNodeName + ".environmentType")
                        envDict['environmentType'] = pm.attrEnumOptionMenuGrp(label="Environemnt Type", at=self.renderGlobalsNodeName + ".environmentType", ei=self.getEnumList(attr))

                with pm.frameLayout(label="Environment Colors", collapsable=False):
                    with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                        ui = pm.floatFieldGrp(label="Environemnt Intensity:", value1=1.0, numberOfFields=1)
                        pm.connectControl(ui, self.renderGlobalsNodeName + ".environmentIntensity", index=2)
                        envDict['environmentColor'] = pm.attrColorSliderGrp(label="Environment Color", at=self.renderGlobalsNodeName + ".environmentColor")
                        envDict['gradientHorizon'] = pm.attrColorSliderGrp(label="Gradient Horizon", at=self.renderGlobalsNodeName + ".gradientHorizon")
                        envDict['gradientZenit'] = pm.attrColorSliderGrp(label="Gradient Zenit", at=self.renderGlobalsNodeName + ".gradientZenit")
                        envDict['environmentMap'] = pm.attrColorSliderGrp(label="Environment Map", at=self.renderGlobalsNodeName + ".environmentMap")
                        self.addRenderGlobalsUIElement(attName='latlongHoShift', uiType='float', displayName='LatLong Horiz Shift:', uiDict=uiDict)
                        self.addRenderGlobalsUIElement(attName='latlongVeShift', uiType='float', displayName='LatLong Vertical Shift:', uiDict=uiDict)

                with pm.frameLayout(label="Physical Sky", collapsable=False):
                    with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                        attr = pm.Attribute(self.renderGlobalsNodeName + ".skyModel")
                        envDict['pskModel'] = pm.attrEnumOptionMenuGrp(label="Sky Model", at=self.renderGlobalsNodeName + ".skyModel", ei=self.getEnumList(attr))
                        pm.separator()
                        envDict['pskUsePhySun'] = pm.checkBoxGrp(label="Use Physical Sun:", value1=False, cc=pm.Callback(self.uiCallback, tab="environment"))
                        pm.connectControl(envDict['pskUsePhySun'], self.renderGlobalsNodeName + ".physicalSun", index=2)
                        envDict['pskPhySun'] = pm.textFieldGrp(label="Sunobject:", text="", editable=False)
                        envDict['pskSunExitMulti'] = pm.floatFieldGrp(label="sunExitance Multiplier:", value1=1.0, numberOfFields=1)
                        pm.connectControl(envDict['pskSunExitMulti'], self.renderGlobalsNodeName + ".sunExitanceMultiplier", index=2)
                        pm.separator()
                        envDict['pskGrAlbedo'] = pm.floatFieldGrp(label="Ground Albedo:", value1=1.0, numberOfFields=1)
                        pm.connectControl(envDict['pskGrAlbedo'], self.renderGlobalsNodeName + ".ground_albedo", index=2)
                        envDict['pskGrHShit'] = pm.floatFieldGrp(label="Horizon Shift:", value1=1.0, numberOfFields=1)
                        pm.connectControl(envDict['pskGrHShit'], self.renderGlobalsNodeName + ".horizon_shift", index=2)
                        envDict['pskLumMulti'] = pm.floatFieldGrp(label="Luminance Multiplier:", value1=1.0, numberOfFields=1)
                        pm.connectControl(envDict['pskLumMulti'], self.renderGlobalsNodeName + ".luminance_multiplier", index=2)
                        envDict['pskSatMulti'] = pm.floatFieldGrp(label="Saturation Multiplier:", value1=1.0, numberOfFields=1)
                        pm.connectControl(envDict['pskSatMulti'], self.renderGlobalsNodeName + ".saturation_multiplier", index=2)
                        envDict['pskTurb'] = pm.floatFieldGrp(label="Turbidity:", value1=1.0, numberOfFields=1)
                        pm.connectControl(envDict['pskTurb'], self.renderGlobalsNodeName + ".turbidity", index=2)
                        envDict['pskTurbMax'] = pm.floatFieldGrp(label="Turbidity Max:", value1=1.0, numberOfFields=1)
                        pm.connectControl(envDict['pskTurbMax'], self.renderGlobalsNodeName + ".turbidity_max", index=2)
                        envDict['pskTurbMin'] = pm.floatFieldGrp(label="Turbidity Min:", value1=1.0, numberOfFields=1)
                        pm.connectControl(envDict['pskTurbMin'], self.renderGlobalsNodeName + ".turbidity_min", index=2)

        pm.setUITemplate("attributeEditorTemplate", popTemplate=True)
        pm.formLayout(parentForm, edit=True, attachForm=[ (scLo, "top", 0), (scLo, "bottom", 0), (scLo, "left", 0), (scLo, "right", 0) ])
        pm.scriptJob(attributeChange=[self.renderGlobalsNode.environmentType, pm.Callback(self.uiCallback, tab="environment")])
        pm.scriptJob(attributeChange=[self.renderGlobalsNode.skyModel, pm.Callback(self.uiCallback, tab="environment")])
        pm.scriptJob(attributeChange=[self.renderGlobalsNode.physicalSun, pm.Callback(self.uiCallback, tab="environment")])
        self.updateEnvironment()

    def AppleseedEnvironmentUpdateTab(self):
        self.updateEnvironment()

    def AppleseedAOVSelectCommand(self, whichField):
        aovDict = self.rendererTabUiDict['aovs']
        if whichField == "source":
            pm.button(aovDict['aovButton'], edit=True, enable=True, label="Add selected Shaders")
            pm.textScrollList(aovDict['aovDestField'], edit=True, deselectAll=True)
        if whichField == "dest":
            pm.button(aovDict['aovButton'], edit=True, enable=True, label="Remove selected Shaders")
            pm.textScrollList(aovDict['aovSourceField'], edit=True, deselectAll=True)

    def AppleseedAOVUpdateDestList(self):
        aovDict = self.rendererTabUiDict['aovs']
        pm.textScrollList(aovDict['aovDestField'], edit=True, removeAll=True)
        aovList = self.AppleseedGetAOVConnections()
        pm.textScrollList(aovDict['aovDestField'], edit=True, append=aovList)

    def AppleseedAOVButtonCommand(self, args=None):
        log.debug("AppleseedAOVButtonCommand " + str(args))
        aovDict = self.rendererTabUiDict['aovs']
        label = pm.button(aovDict['aovButton'], query=True, label=True)
        if "Add selected Shaders" in label:
            selectedItems = pm.textScrollList(aovDict['aovSourceField'], query=True, selectItem=True)
            for item in selectedItems:
                log.debug("Adding " + item)
                node = pm.createNode(item)
                indices = self.renderGlobalsNode.AOVs.getArrayIndices()
                index = 0
                if len(indices) > 0:
                    index = indices[-1] + 1
                node.message >> self.renderGlobalsNode.AOVs[index]
            self.AppleseedAOVUpdateDestList()

        if "Remove selected Shaders" in label:
            selectedItems = pm.textScrollList(aovDict['aovDestField'], query=True, selectItem=True)
            for item in selectedItems:
                shaderName = item.split(" ")[0]
                log.debug("Removing " + shaderName)
                shader = pm.PyNode(shaderName)
                attribute = shader.message.outputs(p=True)[0]
                pm.delete(shader)
                attribute.remove()
            self.AppleseedAOVUpdateDestList()

    def AppleseedDoubleClickCommand(self):
        aovDict = self.rendererTabUiDict['aovs']
        selectedItems = pm.textScrollList(aovDict['aovDestField'], query=True, selectItem=True)
        pm.select(selectedItems[0].split(" ")[0])

    def AppleseedGetAOVConnections(self):
        aoList = self.renderGlobalsNode.AOVs.inputs()
        aovList = []
        for aov in aoList:
            aovList.append(aov.name() + " (" + str(aov.type()) + ")")
        return aovList

    def AppleseedAOVsCreateTab(self):
        aovDict = {}
        self.rendererTabUiDict['aovs'] = aovDict
        self.createGlobalsNode()
        parentForm = pm.setParent(query=True)
        pm.setUITemplate("attributeEditorTemplate", pushTemplate=True)
        scLo = self.rendererName + "AOScrollLayout"
        with pm.scrollLayout(scLo, horizontalScrollBarThickness=0):
            with pm.columnLayout("ColumnLayout", adjustableColumn=True, width=400):
                with pm.frameLayout(label="AOVs", collapsable=True, collapse=False):
                    with pm.columnLayout():
                        with pm.paneLayout(configuration="vertical2", paneSize=(1, 25, 100)):
                            aovDict['aovSourceField'] = pm.textScrollList("AOVSource", ams=True, append=self.aovShaders, selectCommand=pm.Callback(self.AppleseedAOVSelectCommand, "source"))
                            aovList = self.AppleseedGetAOVConnections()
                            aovDict['aovDestField'] = pm.textScrollList("AOVDest", append=aovList, ams=True, dcc=self.AppleseedDoubleClickCommand, selectCommand=pm.Callback(self.AppleseedAOVSelectCommand, "dest"))
                        aovDict['aovButton'] = pm.button(label="Selection", enable=False, c=self.AppleseedAOVButtonCommand)

        pm.setUITemplate("attributeEditorTemplate", popTemplate=True)
        pm.formLayout(parentForm, edit=True, attachForm=[ (scLo, "top", 0), (scLo, "bottom", 0), (scLo, "left", 0), (scLo, "right", 0) ])


    def AppleseedAOVsUpdateTab(self):
        log.debug("AppleseedAOVsUpdateTab()")

    def AppleseedCommonGlobalsCreateTab(self):
        self.OpenMayaCommonGlobalsCreateTab()

    def AppleseedCommonGlobalsUpdateTab(self):
        self.OpenMayaCommonGlobalsUpdateTab()

    def AppleseedRendererCreateTab(self):
        self.createGlobalsNode()
        parentForm = pm.setParent(query=True)
        pm.setUITemplate("renderGlobalsTemplate", pushTemplate=True)
        pm.setUITemplate("attributeEditorTemplate", pushTemplate=True)
        scLo = self.rendererName + "ScrollLayout"
        if self.rendererTabUiDict.has_key('common'):
            self.rendererTabUiDict.pop('common')
        uiDict = {}
        self.rendererTabUiDict['common'] = uiDict

        with pm.scrollLayout(scLo, horizontalScrollBarThickness=0):
            with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                with pm.frameLayout(label="Pixel Sampler", collapsable=True, collapse=False):
                    with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                        self.addRenderGlobalsUIElement(attName='pixel_renderer', uiType='enum', displayName='Pixel Sampler', default='0', uiDict=uiDict, callback=self.AppleseedRendererUpdateTab)
                        self.addRenderGlobalsUIElement(attName='minSamples', uiType='int', displayName='Min Samples', default=False, uiDict=uiDict)
                        self.addRenderGlobalsUIElement(attName='maxSamples', uiType='int', displayName='Max Samples', default=False, uiDict=uiDict)
                        self.addRenderGlobalsUIElement(attName='maxError', uiType='float', displayName='Max Error', default=False, uiDict=uiDict)
                        self.addRenderGlobalsUIElement(attName='enable_diagnostics', uiType='bool', displayName="Diagnostic AOV's", default=False, uiDict=uiDict)
                        pm.separator()
                        self.addRenderGlobalsUIElement(attName='frameRendererPasses', uiType='int', displayName='Passes', uiDict=uiDict)

                with pm.frameLayout(label="Filtering", collapsable=True, collapse=False):
                    with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                        attr = pm.Attribute(self.renderGlobalsNodeName + ".filtertype")
                        ui = pm.attrEnumOptionMenuGrp(label="Filter Type", at=self.renderGlobalsNodeName + ".filtertype", ei=self.getEnumList(attr))
                        self.addRenderGlobalsUIElement(attName='filtersize', uiType='int', displayName='Filter Size', default=False, uiDict=uiDict)

                with pm.frameLayout(label="Features", collapsable=True, collapse=False):
                    with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                        self.addRenderGlobalsUIElement(attName='doMotionBlur', uiType='bool', displayName='Motion Blur', default=False, uiDict=uiDict)
                        self.addRenderGlobalsUIElement(attName='doDof', uiType='bool', displayName='Depth Of Field', default=False, uiDict=uiDict)

                with pm.frameLayout(label="Output", collapsable=True, collapse=False):
                    with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                        attr = pm.Attribute(self.renderGlobalsNodeName + ".bitdepth")
                        ui = pm.attrEnumOptionMenuGrp(label="Bit Depth", at=self.renderGlobalsNodeName + ".bitdepth", ei=self.getEnumList(attr))
                        attr = pm.Attribute(self.renderGlobalsNodeName + ".colorSpace")
                        ui = pm.attrEnumOptionMenuGrp(label="Color Space", at=self.renderGlobalsNodeName + ".colorSpace", ei=self.getEnumList(attr))
                        ui = pm.checkBoxGrp(label="Clamping:", value1=False)
                        pm.connectControl(ui, self.renderGlobalsNodeName + ".clamping", index=2)

                with pm.frameLayout(label="Lighting Engine", collapsable=True, collapse=False):
                    with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                        self.addRenderGlobalsUIElement(attName='lightingEngine', uiType='enum', displayName='Lighting Engine', default='0', uiDict=uiDict, callback=self.AppleseedRendererUpdateTab)
                        with pm.frameLayout(label="Lighting Engine Settings", collapsable=True, collapse=False) as uiDict['LE_framelayout']:
                            with pm.columnLayout(self.rendererName + "LEColumnLayout", adjustableColumn=True, width=400) as uiDict['LE_layout']:
                                pass

                with pm.frameLayout(label="Renderer", collapsable=True, collapse=True):
                    with pm.columnLayout(self.rendererName + "ColumnLayout", adjustableColumn=True, width=400):
                        self.addRenderGlobalsUIElement(attName='threads', uiType='int', displayName='Threads:', uiDict=uiDict)
                        self.addRenderGlobalsUIElement(attName='rendererVerbosity', uiType='int', displayName='Verbosity:', uiDict=uiDict)
                        self.addRenderGlobalsUIElement(attName='tilesize', uiType='int', displayName='Tile Size:', uiDict=uiDict)
                        self.addRenderGlobalsUIElement(attName='texCacheSize', uiType='int', displayName='Tex Cache Size (kb):', uiDict=uiDict)
                        self.addRenderGlobalsUIElement(attName='assemblySBVH', uiType='bool', displayName='Use SBVH Acc. for mb:', uiDict=uiDict)

        pm.setUITemplate("renderGlobalsTemplate", popTemplate=True)
        pm.setUITemplate("attributeEditorTemplate", popTemplate=True)
        pm.formLayout(parentForm, edit=True, attachForm=[ (scLo, "top", 0), (scLo, "bottom", 0), (scLo, "left", 0), (scLo, "right", 0) ])
        self.AppleseedRendererUpdateTab()

    def AppleseedRendererUpdateTab(self, dummy=None):
        self.createGlobalsNode()
        if not self.rendererTabUiDict.has_key('common'):
            return

        uiDict = self.rendererTabUiDict['common']

        if self.renderGlobalsNode.pixel_renderer.get() == 0:
            uiDict['maxSamples'].setLabel("Max Samples")
            uiDict['maxSamples'].setEnable(True)
            uiDict['minSamples'].setManage(True)
            uiDict['maxError'].setManage(True)

        if self.renderGlobalsNode.pixel_renderer.get() == 1:
            uiDict['minSamples'].setManage(False)
            uiDict['maxError'].setManage(False)
            uiDict['maxSamples'].setLabel("Samples")

        pm.deleteUI(uiDict['LE_layout'])
        with pm.columnLayout(self.rendererName + "LEColumnLayout", adjustableColumn=True, width=400, parent=uiDict['LE_framelayout']) as uiDict['LE_layout']:
            if self.renderGlobalsNode.lightingEngine.get() == 0:  # path tracer
                self.addRenderGlobalsUIElement(attName='enable_ibl', uiType='bool', displayName='Enable IBL', default=False, uiDict=uiDict, callback=self.AppleseedRendererUpdateTab)
                self.addRenderGlobalsUIElement(attName='enable_caustics', uiType='bool', displayName='Enable Caustics', default=False, uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='enable_dl', uiType='bool', displayName='Enable Direct Light', default=False, uiDict=uiDict, callback=self.AppleseedRendererUpdateTab)
                pm.separator()
                self.addRenderGlobalsUIElement(attName='environmentSamples', uiType='int', displayName='Environment Samples', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='directLightSamples', uiType='int', displayName='Direct Light Samples', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='max_ray_intensity', uiType='float', displayName='Max Ray Intensity', anno='Clamp intensity of rays (after the first bounce) to this value to reduce fireflies', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='max_path_length', uiType='float', displayName='Max Bounces', uiDict=uiDict)
                if not self.renderGlobalsNode.enable_ibl.get():
                    uiDict['environmentSamples'].setEnable(False)
                else:
                    uiDict['environmentSamples'].setEnable(True)
                if not self.renderGlobalsNode.enable_dl.get():
                    uiDict['directLightSamples'].setEnable(False)
                else:
                    uiDict['directLightSamples'].setEnable(True)

            if self.renderGlobalsNode.lightingEngine.get() == 1:  # sppm
                self.addRenderGlobalsUIElement(attName='enable_ibl', uiType='bool', displayName='Enable IBL', default=False, uiDict=uiDict, callback=self.AppleseedRendererUpdateTab)
                self.addRenderGlobalsUIElement(attName='enable_caustics', uiType='bool', displayName='Enable Caustics', default=False, uiDict=uiDict)
                pm.separator()
                self.addRenderGlobalsUIElement(attName='dl_mode', uiType='enum', displayName='Direct Lighting Mode', default='0', uiDict=uiDict, callback=self.AppleseedRendererUpdateTab)
                pm.separator()
                self.addRenderGlobalsUIElement(attName='path_tracing_max_path_length', uiType='int', displayName='Max Bounces', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='light_photons_per_pass', uiType='int', displayName='Light Photons PP', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='env_photons_per_pass', uiType='int', displayName='Env Photons PP', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='max_photons_per_estimate', uiType='int', displayName='Max Photons Per Estimate', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='photon_type', uiType='enum', displayName='Photon Type', default='0', uiDict=uiDict, callback=self.AppleseedRendererUpdateTab)
                self.addRenderGlobalsUIElement(attName='photon_tracing_rr_min_path_length', uiType='int', displayName='Min Bounces', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='photon_tracing_max_path_length', uiType='int', displayName='Max Bounces', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='photons_per_pass', uiType='int', displayName='Photons Per Pass', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='initial_radius', uiType='float', displayName='Initial Radius', anno='Initial photon gathering radius in percent of the scene diameter.', uiDict=uiDict)
                self.addRenderGlobalsUIElement(attName='sppmAlpha', uiType='float', displayName='Alpha', anno='Evolution rate of photon gathering radius', uiDict=uiDict)


    def outputFileBrowse(self, args=None):
        startingDir = path.path(self.renderGlobalsNode.exportSceneFileName.get()).dirname()
        filename = pm.fileDialog2(fileMode=0, caption="Output File Name", startingDirectory=startingDir)
        if filename:
            self.rendererTabUiDict['translator']['fileNameField'].setText(filename[0])

    def dirBrowse(self, args=None):
        dirname = pm.fileDialog2(fileMode=3, caption="Select Optimize Files Dir")
        if dirname:
            self.rendererTabUiDict['opti']['optiField'].setText(dirname[0])
        
    def AppleseedTranslatorCreateTab(self):
        self.createGlobalsNode()
        parentForm = pm.setParent(query=True)
        pm.setUITemplate("attributeEditorTemplate", pushTemplate=True)
        scLo = self.rendererName + "TrScrollLayout"
        uiDict = {}
        self.rendererTabUiDict['translator'] = uiDict

        with pm.scrollLayout(scLo, horizontalScrollBarThickness=0):
            with pm.columnLayout(self.rendererName + "TrColumnLayout", adjustableColumn=True, width=400):
                with pm.frameLayout(label="Translator", collapsable=True, collapse=False):
                    with pm.columnLayout(adjustableColumn=True, width=400):
                        self.addRenderGlobalsUIElement(attName='translatorVerbosity', uiType='enum', displayName='Verbosity', default='0', uiDict=uiDict)
                with pm.frameLayout(label="{0} Output".format(self.rendererName), collapsable=True, collapse=False):
                    with pm.columnLayout(adjustableColumn=True, width=400):
                        self.addRenderGlobalsUIElement(attName='exportMode', uiType='enum', displayName='Output Mode', default='0', uiDict=uiDict, callback=self.AppleseedTranslatorUpdateTab)
                    with pm.rowLayout(nc=3) as uiDict['outputFilenameLayout']:
                        pm.text(label="Output Filename:")
                        uiDict['fileNameField'] = pm.textField(text=self.renderGlobalsNode.exportSceneFileName.get())
                        pm.symbolButton(image="navButtonBrowse.png", c=self.outputFileBrowse)
                        pm.connectControl(uiDict['fileNameField'], self.renderGlobalsNodeName + ".exportSceneFileName", index=2)
                with pm.frameLayout(label="Optimize Textures", collapsable=True, collapse=False):
                    with pm.columnLayout(adjustableColumn=True, width=400):
                        optiDict = {}
                        ui = pm.checkBoxGrp(label="Use Optimized Textures:", value1=False)
                        pm.connectControl(ui, self.renderGlobalsNodeName + ".useOptimizedTextures", index=2)
                    with pm.rowLayout(nc=3):
                        self.rendererTabUiDict['opti'] = optiDict
                        pm.text(label="OptimizedTex Dir:")
                        optiDict['optiField'] = pm.textField(text=self.renderGlobalsNode.optimizedTexturePath.get())
                        pm.symbolButton(image="navButtonBrowse.png", c=self.dirBrowse)
                        pm.connectControl(optiDict['optiField'], self.renderGlobalsNodeName + ".optimizedTexturePath", index=2)

                with pm.frameLayout(label="Additional Settings", collapsable=True, collapse=False):
                    ui = pm.floatFieldGrp(label="Scene scale:", value1=1.0, numberOfFields=1)
                    pm.connectControl(ui, self.renderGlobalsNodeName + ".sceneScale", index=2)

        pm.setUITemplate("attributeEditorTemplate", popTemplate=True)
        pm.formLayout(parentForm, edit=True, attachForm=[ (scLo, "top", 0), (scLo, "bottom", 0), (scLo, "left", 0), (scLo, "right", 0) ])
        self.AppleseedTranslatorUpdateTab()

    def AppleseedTranslatorUpdateTab(self):
        uiDict = self.rendererTabUiDict['translator']
        outputMode = self.renderGlobalsNode.exportMode.get()
        if outputMode == 0:  # render only
            uiDict['outputFilenameLayout'].setEnable(False)
        if outputMode == 1:  # output only
            uiDict['outputFilenameLayout'].setEnable(True)
        if outputMode == 2:  # render and output
            uiDict['outputFilenameLayout'].setEnable(True)
        if outputMode > 0:
            textField = uiDict['fileNameField']
            outputFieldtext = pm.textField(textField, query=True, text=True)
            if len(outputFieldtext) == 0:
                exportFileName = pm.workspace.path + "/renderData/" + pm.sceneName().basename().split(".")[0] + ".appleseed"
                self.renderGlobalsNode.exportSceneFileName.set(exportFileName)
                pm.textField(textField, edit=True, text=exportFileName)
                
    def uiCallback(self, **args):
        if args['tab'] == "environment":
            self.updateEnvironment()

    def createImageFormats(self):
        if self.renderGlobalsNode:
            iList = self.renderGlobalsNode.imageFormat.getEnums()
            self.imageFormats = []
            self.imageFormats.extend(iList)

    def registerNodeExtensions(self):
        """Register appleseed specific node extensions. e.g. camera type, diaphram_blades and others
        """
        # we will have a thinlens camera only
        # pm.addExtension(nodeType="camera", longName="mtap_cameraType", attributeType="enum", enumName="Pinhole:Thinlens", defaultValue = 0)
        pm.addExtension(nodeType="camera", longName="mtap_diaphragm_blades", attributeType="long", defaultValue=0)
        pm.addExtension(nodeType="camera", longName="mtap_diaphragm_tilt_angle", attributeType="float", defaultValue=0.0)
        pm.addExtension(nodeType="camera", longName="mtap_latLongLens", attributeType="bool", defaultValue=False)

        # mesh
        pm.addExtension(nodeType="mesh", longName="mtap_mesh_useassembly", attributeType="bool", defaultValue=False)
        pm.addExtension(nodeType="mesh", longName="mtap_ray_bias_method", attributeType="enum", enumName="none:normal:incoming_direction:outgoing_direction", defaultValue=0)
        pm.addExtension(nodeType="mesh", longName="mtap_ray_bias_distance", attributeType="float", defaultValue=0.0)
        pm.addExtension(nodeType="mesh", longName="mtap_standin_path", dataType="string", usedAsFilename=True)
        pm.addExtension(nodeType="mesh", longName="mtap_visibleLights", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="mesh", longName="mtap_visibleProbe", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="mesh", longName="mtap_visibleGlossy", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="mesh", longName="mtap_visibleSpecular", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="mesh", longName="mtap_visibleDiffuse", attributeType="bool", defaultValue=True)

        pm.addExtension(nodeType="spotLight", longName="mtap_cast_indirect_light", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="spotLight", longName="mtap_importance_multiplier", attributeType="float", defaultValue=1.0)
        pm.addExtension(nodeType="directionalLight", longName="mtap_cast_indirect_light", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="directionalLight", longName="mtap_importance_multiplier", attributeType="float", defaultValue=1.0)
        pm.addExtension(nodeType="pointLight", longName="mtap_cast_indirect_light", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="pointLight", longName="mtap_importance_multiplier", attributeType="float", defaultValue=1.0)

        pm.addExtension(nodeType="areaLight", longName="primaryVisibility", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="areaLight", longName="castsShadows", attributeType="bool", defaultValue=False)
        pm.addExtension(nodeType="areaLight", longName="mtap_visibleLights", attributeType="bool", defaultValue=False)
        pm.addExtension(nodeType="areaLight", longName="mtap_visibleProbe", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="areaLight", longName="mtap_visibleGlossy", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="areaLight", longName="mtap_visibleSpecular", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="areaLight", longName="mtap_visibleDiffuse", attributeType="bool", defaultValue=True)
        pm.addExtension(nodeType="areaLight", longName="mtap_visibleTransparency", attributeType="bool", defaultValue=True)

    def renderProcedure(self, width, height, doShadows, doGlow, camera, options):
        self.createGlobalsNode()
        self.preRenderProcedure()
        self.setImageName()
        if pm.about(batch=True):
            pm.appleseedMaya()
        else:
            pm.appleseedMaya(width=width, height=height, camera=camera)

    def startIprRenderProcedure(self, editor, resolutionX, resolutionY, camera):
        self.ipr_isrunning = True
        self.createGlobalsNode()
        self.preRenderProcedure()
        self.setImageName()
        pm.appleseedMaya(width=resolutionX, height=resolutionY, camera=camera, startIpr=True)

    def stopIprRenderProcedure(self):
        pm.appleseedMaya(stopIpr=True)
        self.ipr_isrunning = False

    def updateProgressBar(self, percent):
        if not self.ipr_isrunning:
            progressValue = percent * 100
            pm.progressBar(self.gMainProgressBar, edit=True, progress=progressValue)

    def preRenderProcedure(self):
        self.createGlobalsNode()
        if self.renderGlobalsNode.threads.get() == 0:
            # TODO this is windows only, search for another solution...
            numThreads = int(os.environ['NUMBER_OF_PROCESSORS'])
            # Let's reduce the number of threads to stay as interactive as possible.
            if self.ipr_isrunning:
                if numThreads > 4:
                    numThreads = numThreads - 1
                self.renderGlobalsNode.threads.set(numThreads)

        if self.renderGlobalsNode.useOptimizedTextures.get():
            if not self.renderGlobalsNode.optimizedTexturePath.get() or len(self.renderGlobalsNode.optimizedTexturePath.get()) == 0:
                try:
                    optimizedPath = pm.workspace.path / pm.workspace.fileRules['renderData'] / "optimizedTextures"
                except:
                    optimizedPath = path.path(tempfile.gettempdir()) / "optimizedTextures"
                if not os.path.exists(optimizedPath):
                    optimizedPath.makedirs()
                self.renderGlobalsNode.optimizedTexturePath.set(str(optimizedPath))

            optimizetextures.preRenderOptimizeTextures(optimizedFilePath=self.renderGlobalsNode.optimizedTexturePath.get())

        if not self.ipr_isrunning:
            self.gMainProgressBar = pm.mel.eval('$tmp = $gMainProgressBar');
            pm.progressBar(self.gMainProgressBar,
                                    edit=True,
                                    beginProgress=True,
                                    isInterruptable=True,
                                    status='"Render progress:',
                                    maxValue=100)

    def postRenderProcedure(self):
        optimizetextures.postRenderOptimizeTextures()        
        if not self.ipr_isrunning:
            pm.progressBar(self.gMainProgressBar, edit=True, endProgress=True)

    def afterGlobalsNodeReplacement(self):
        self.rendererTabUiDict = {}

    def aeTemplateCallback(self, nodeName):
        aet.AEappleseedNodeTemplate(nodeName)

    def connectNodeToNodeOverrideCallback(self, srcNode, destNode):
        dn = pm.PyNode(destNode)
        sn = pm.PyNode(srcNode)
        return 1

    def createRenderNode(self, nodeType=None, postCommand=None):
        nodeClass = None
        rendererName = self.rendererName.lower()
        for cl in pm.getClassification(nodeType):
            if (rendererName + "/material") in cl.lower():
                nodeClass = "shader"
            if (rendererName + "/texture") in cl.lower():
                nodeClass = "texture"

        if nodeClass == "shader":
            mat = pm.shadingNode(nodeType, asShader=True)
            shadingGroup = pm.sets(renderable=True, noSurfaceShader=True, empty=True, name="{0}SG".format(mat))
            mat.outColor >> shadingGroup.surfaceShader
        else:
            mat = pm.shadingNode(nodeType, asTexture=True)

        if postCommand is not None:
            postCommand = postCommand.replace("%node", str(mat))
            postCommand = postCommand.replace("%type", '\"\"')
            pm.mel.eval(postCommand)
        return ""


"""
This procedure loads all AETemplates that are loaceted in the AETemplates module.
Normally if you load pymel, it automatically loads the templates but only the ones it finds in the
very first AETemplates directory. If you have several OpenMaya renderers loaded or if you have your own
AETemplates directory, the automatic loading will not work. So I replace it with this procedure.
"""

def loadAETemplates():
    rendererName = "appleseed"
    aeDir = path.path(__file__).dirname() + "/aetemplate/"
    for d in aeDir.listdir("*.py"):
        if d.endswith("template.py"):
            templateName = d.basename().replace(".py", "")
            pythonCommand = "import {1}.aetemplate.{0}".format(templateName, rendererName.lower())
            melCommand = 'python("{0}");'.format(pythonCommand)
            pm.mel.eval(melCommand)

def loadPlugins():
    python_plugins = ["loadshadersplugin"]
    currentPath = path.path(__file__).dirname()
    
    for plugin in python_plugins:
        try:
            pluginPath = "{0}/{1}.py".format(currentPath, plugin) 
            log.debug("Loading additional plugin: {0}".format(pluginPath))
            if not pm.pluginInfo(plugin, query=True, loaded=True):
                pm.loadPlugin(pluginPath)
        except:
            traceback.print_exc(file=sys.__stderr__)
            log.error("Loading of additional plugin: {0} failed.".format(pluginPath))

def theRenderer():
    return AppleseedRenderer.theRenderer()

def initRenderer():
    try:
        theRenderer().registerRenderer()
        if not pm.about(batch=True):
            loadAETemplates()
            theRenderer().createRendererMenu()
        loadPlugins()
    except:
        traceback.print_exc(file=sys.__stderr__)
        log.error("Init renderer appleseed FAILED")

def unregister():
    theRenderer().removeRendererMenu()
    theRenderer().unRegisterRenderer()

def uiCallback(*args):
    theRenderer().uiCallback(args)
