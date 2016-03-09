
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

import commonglobals as commonGlobals
import path
import uiutils as uiUtils

import pymel.core as pm

import logging
import os
import sys

log = logging.getLogger("renderLogger")

class MayaToRenderer(object):

    def __init__(self, rendererName, moduleName):
        self.rendererName = rendererName
        self.moduleName = moduleName
        self.pluginName = "mayato" + self.rendererName.lower()
        self.renderGlobalsNodeName = self.rendererName.lower() + "Globals"
        self.renderGlobalsNode = None
        # self.baseRenderMelCommand = "import {0} as rcall; reload(rcall); rcall.theRenderer().".format(self.moduleName)
        self.baseRenderMelCommand = "import {0} as rcall; rcall.theRenderer().".format(self.moduleName)
        self.imageFormats = []
        self.ipr_isrunning = False
        self.imageFormatCtrl = None
        self.openMayaCommonGlobals = None
        self.useRendererTabPrefix = True

    def addRenderGlobalsUIElement(self, attName=None, uiType=None, displayName=None, default=None, data=None, uiDict=None, callback=None, anno=None):
        uiUtils.addRenderGlobalsUIElement(self.renderGlobalsNodeName, attName, uiType, displayName, default, data, uiDict, callback, anno)
    # the render callback is called with arguments like this
    # renderCmd 640 480 1 1 perspShape " -layer defaultRenderLayer"
    # if the renderCmd is replaced with a python call it may not get all informations so we
    # can think about an alternative call like this
    # renderMelCmd pythonCmd 640 480 1 1 perspShape " -layer defaultRenderLayer"
    # this can result in a pythonCmd(640, 480, 1, 1, perspShape, " -layer defaultRenderLayer") call
    def renderCallback(self, procedureName):
        cmd = "python(\"{0}{1}()\");\n".format(self.baseRenderMelCommand, procedureName)
        return cmd

    def makeMelPythonCmdStringFromPythonCmd(self, pythonCmdObj, argSet):
        pCmd = "\"{0}{1}(".format(self.baseRenderMelCommand, pythonCmdObj.__name__)
        argList = []
        for dtype, value in argSet:
            if dtype == 'string':
                argList.append(value + "=" + r"""'"+$%s+"'""" % value)
            else:
                argList.append(value + "=\" + $" + value + " + \"")
        pCmd += ",".join(argList)
        pCmd += ")\""
        return pCmd

    def makeMelProcFromPythonCmd(self, pythonCmdObj, argSet):
        cmd = pythonCmdObj.__name__
        melProcName = "RenderProc_" + cmd
        melCmd = "global proc %s (" % melProcName
        argList = []
        for arg in argSet:
            argList.append(" $".join(arg))
        melCmd += ",".join(argList) + ")\n{\n"
        melCmd += "    string $cmdString = " + self.makeMelPythonCmdStringFromPythonCmd(pythonCmdObj, argSet) + ";\n"
        melCmd += "    python($cmdString);\n"
        melCmd += "};\n";
        pm.mel.eval(melCmd)
        return melProcName

    def batchRenderProcedure(self, options):
        self.preRenderProcedure()
        self.renderProcedure(-1, -1, True, True, None, options)

    def renderOptionsProcedure(self):
        self.preRenderProcedure()

    def commandRenderProcedure(self, width, height, doShadows, doGlow, camera, options):
        self.preRenderProcedure()
        self.renderProcedure()

    def renderProcedure(self, width, height, doShadows, doGlowPass, camera, options):
        pass

    def batchRenderOptionsProcedure(self):
        self.preRenderProcedure()

    def batchRenderOptionsStringProcedure(self):
        self.preRenderProcedure()

    def cancelBatchRenderProcedure(self):
        self.preRenderProcedure()

    def showBatchRenderProcedure(self):
        self.preRenderProcedure()

    def showRenderLogProcedure(self):
        self.preRenderProcedure()

    def showBatchRenderLogProcedure(self):
        self.preRenderProcedure()

    def renderRegionProcedure(self):
        self.preRenderProcedure()

    def textureBakingProcedure(self):
        self.preRenderProcedure()

    def renderingEditorsSubMenuProcedure(self, *args):
        log.debug("renderingEditorsSubMenuProcedure")

    def OpenMayaCommonGlobalsCreateTab(self):
        self.createGlobalsNode()
        if self.openMayaCommonGlobals is None:
            self.openMayaCommonGlobals = commonGlobals.OpenMayaCommonGlobals(self.renderGlobalsNode)
        self.openMayaCommonGlobals.renderNode = self.renderGlobalsNode
        self.openMayaCommonGlobals.OpenMayaCommonGlobalsCreateTab()

    def OpenMayaCommonGlobalsUpdateTab(self):
        self.createGlobalsNode()
        if self.openMayaCommonGlobals is None:
            self.openMayaCommonGlobals = commonGlobals.OpenMayaCommonGlobals(self.renderGlobalsNode)
        self.openMayaCommonGlobals.renderNode = self.renderGlobalsNode
        self.openMayaCommonGlobals.OpenMayaCommonGlobalsUpdateTab()

    # render tab creation. Renderer tabs need a mel globals procedure.
    # a mel procedure is called for create and update tab
    # here I create a fake mel procedure to call my own python command

    # per default, the inheriting class will implement the procedures:
    # {Name}RendererCreateTab() and the {Name}RendererUpdateTab()
    # {Name}TranslatorCreateTab() and the {Name}TranslatorUpdateTab()
    def renderTabMelProcedure(self, tabName):
        createTabCmd = "{0}{1}CreateTab".format(self.rendererName, tabName)
        updateTabCmd = "{0}{1}UpdateTab".format(self.rendererName, tabName)
        tabLabel = "{0}".format(tabName)
        createPyCmd = "python(\"{0}{1}()\");".format(self.baseRenderMelCommand, createTabCmd)
        updatePyCmd = "python(\"{0}{1}()\");".format(self.baseRenderMelCommand, updateTabCmd)
        melCreateCmd = "global proc {0}()\n{{\t{1}\n}}\n".format(createTabCmd, createPyCmd)
        melUpdateCmd = "global proc {0}()\n{{\t{1}\n}}\n".format(updateTabCmd, updatePyCmd)
        pm.mel.eval(melCreateCmd)
        pm.mel.eval(melUpdateCmd)
        return (tabLabel, createTabCmd, updateTabCmd)

    def createImageFormats(self):
        pass

    def checkNamingConvention(self):
        #check file naming convention. We use only name.ext and name.nr.ext
        defaultGlobals = pm.SCENE.defaultRenderGlobals
        defaultGlobals.periodInExt.set(1)
        defaultGlobals.putFrameBeforeExt.set(1)

    def preRenderProcedure(self):
        self.checkNamingConvention()
        self.createGlobalsNode()
        drg = pm.PyNode("defaultRenderGlobals")
        if self.renderGlobalsNode.threads.get() == 0:
            # TODO this is windows only, search for another solution...
            numThreads = int(os.environ['NUMBER_OF_PROCESSORS'])
            self.renderGlobalsNode.threads.set(numThreads)

    def postRenderProcedure(self):
        drg = pm.PyNode("defaultRenderGlobals")

    def preFrameProcedure(self):
        pass

    def postFrameProcedure(self):
        pass

    def overwriteUpdateMayaImageFormatControl(self):
        cmd = """
global proc string createMayaImageFormatControl()
{
    return "imageMenuMayaSW";
}
"""
        pm.mel.eval(cmd)
        cmd = """
global proc updateMayaImageFormatControl()
{}
"""
        pm.mel.eval(cmd)
        return

    def unDoOverwriteUpdateMayaImageFormatControl(self):
        pm.mel.eval("source createMayaSoftwareCommonGlobalsTab")

    def createGlobalsNode(self):
        selection = pm.ls(sl=True)
        if not self.renderGlobalsNode:
            # maybe the node has been replaced by replaced by a new loaded node, check this
            if len(pm.ls(self.renderGlobalsNodeName)) > 0:
                self.renderGlobalsNode = pm.ls(self.renderGlobalsNodeName)[0]
                log.debug("Globals node replaced.")
                self.afterGlobalsNodeReplacement()
                self.renderGlobalsNode.setLocked(True)
            else:
                self.renderGlobalsNode = pm.createNode(self.renderGlobalsNodeName)
                self.renderGlobalsNode.rename(self.renderGlobalsNodeName)
                log.debug("Created node " + str(self.renderGlobalsNode))
                optimizedPath = pm.workspace.path+"/optimizedTextures"
                if pm.workspace.fileRules.has_key('renderData'):
                    optimizedPath = pm.workspace.path / pm.workspace.fileRules['renderData'] / "optimizedTextures"
                if not os.path.exists(optimizedPath):
                    optimizedPath.makedirs()
                self.renderGlobalsNode.optimizedTexturePath.set(str(optimizedPath))
                self.afterGlobalsNodeReplacement()
                self.renderGlobalsNode.setLocked(True)
        pm.select(selection)

    def registerNodeExtensions(self):
        pass

    def hyperShadePanelBuildCreateMenuCallback(self):
        pm.menuItem(label=self.rendererName)
        pm.menuItem(divider=True)

    def hyperShadePanelBuildCreateSubMenuCallback(self):
        return "shader/surface"

    def buildRenderNodeTreeListerContentCallback(self, tl, postCommand, filterString):
        melCmd = 'addToRenderNodeTreeLister( "{0}", "{1}", "{2}", "{3}", "{4}", "{5}");'.format(tl, postCommand, self.rendererName + "/Materials", self.rendererName.lower() + "/material", "-asShader", "")
        log.debug("Treelister cmd " + melCmd)
        pm.mel.eval(melCmd)
        melCmd = 'addToRenderNodeTreeLister( "{0}", "{1}", "{2}", "{3}", "{4}", "{5}");'.format(tl, postCommand, self.rendererName + "/Textures", self.rendererName.lower() + "/texture", "-asUtility", "")
        pm.mel.eval(melCmd)

    def aeTemplateCallback(self, nodeName):
        pass

    def registerAETemplateCallbacks(self):
        pm.callbacks(addCallback=self.aeTemplateCallback, hook='AETemplateCustomContent', owner=self.pluginName)
        pm.callbacks(addCallback=self.hyperShadePanelBuildCreateMenuCallback, hook='hyperShadePanelBuildCreateMenu', owner=self.pluginName)
        pm.callbacks(addCallback=self.hyperShadePanelBuildCreateSubMenuCallback, hook='hyperShadePanelBuildCreateSubMenu', owner=self.pluginName)
        pm.callbacks(addCallback=self.buildRenderNodeTreeListerContentCallback, hook='buildRenderNodeTreeListerContent', owner=self.pluginName)
        pm.callbacks(addCallback=self.createRenderNodeCallback, hook='createRenderNodeCommand', owner=self.pluginName)
        pm.callbacks(addCallback=self.connectNodeToNodeOverrideCallback, hook='connectNodeToNodeOverrideCallback', owner=self.pluginName)
        aeTemplateName = "AE{0}NodeTemplate".format(self.rendererName.lower())
        aeTemplateImportName = aeTemplateName

    def createRenderNodeCallback(self, postCommand, nodeType):
        for c in pm.getClassification(nodeType):
            if self.rendererName.lower() in c.lower():
                buildNodeCmd = "import {0} as rcall; rcall.theRenderer().createRenderNode(nodeType=\\\"{1}\\\", postCommand='{2}')".format(self.moduleName, nodeType, postCommand)
                buildNodeCmd = "string $cmd = \"{0}\"; python($cmd);".format(buildNodeCmd)
                return buildNodeCmd

    def connectNodeToNodeOverrideCallback(self, srcNode, destNode):
        return 1

    def registerRenderer(self):
        self.unRegisterRenderer()
        self.registerNodeExtensions()
        self.registerAETemplateCallbacks()
        pm.renderer(self.rendererName, rendererUIName=self.rendererName)
        pm.renderer(self.rendererName, edit=True, renderProcedure=self.makeMelProcFromPythonCmd(self.renderProcedure, [('int', 'width'),
                                                                                                                       ('int', 'height'),
                                                                                                                       ('int', 'doShadows'),
                                                                                                                       ('int', 'doGlow'),
                                                                                                                       ('string', 'camera'),
                                                                                                                       ('string', 'options')]))
        pm.renderer(self.rendererName, edit=True, batchRenderProcedure=self.makeMelProcFromPythonCmd(self.batchRenderProcedure, [('string', 'options')]))
        pm.renderer(self.rendererName, edit=True, commandRenderProcedure=self.makeMelProcFromPythonCmd(self.batchRenderProcedure, [('string', 'options')]))
        pm.renderer(self.rendererName, edit=True, batchRenderOptionsProcedure=self.renderCallback("batchRenderOptionsProcedure"))
        pm.renderer(self.rendererName, edit=True, batchRenderOptionsStringProcedure=self.renderCallback("batchRenderOptionsStringProcedure"))
        pm.renderer(self.rendererName, edit=True, addGlobalsNode="defaultRenderGlobals")
        pm.renderer(self.rendererName, edit=True, addGlobalsNode="defaultResolution")
        pm.renderer(self.rendererName, edit=True, addGlobalsNode=self.renderGlobalsNodeName)
        pm.renderer(self.rendererName, edit=True, startIprRenderProcedure=self.makeMelProcFromPythonCmd(self.startIprRenderProcedure, [('string', 'editor'),
                                                                                                                                       ('int', 'resolutionX'),
                                                                                                                                       ('int', 'resolutionY'),
                                                                                                                                       ('string', 'camera')]))
        pm.renderer(self.rendererName, edit=True, stopIprRenderProcedure=self.makeMelProcFromPythonCmd(self.stopIprRenderProcedure, []))
        pm.renderer(self.rendererName, edit=True, pauseIprRenderProcedure=self.makeMelProcFromPythonCmd(self.pauseIprRenderProcedure, [('string', 'editor'), ('int', 'pause')]))
        pm.renderer(self.rendererName, edit=True, changeIprRegionProcedure=self.renderCallback("changeIprRegionProcedure"))
        pm.renderer(self.rendererName, edit=True, iprRenderProcedure=self.renderCallback("iprRenderProcedure"))
        pm.renderer(self.rendererName, edit=True, isRunningIprProcedure=self.renderCallback("isRunningIprProcedure"))
        pm.renderer(self.rendererName, edit=True, refreshIprRenderProcedure=self.renderCallback("refreshIprRenderProcedure"))
        pm.renderer(self.rendererName, edit=True, logoImageName=self.rendererName + ".png")
        pm.renderer(self.rendererName, edit=True, renderDiagnosticsProcedure=self.renderCallback("renderDiagnosticsProcedure"))
        pm.renderer(self.rendererName, edit=True, renderOptionsProcedure=self.renderCallback("renderOptionsProcedure"))
        pm.renderer(self.rendererName, edit=True, cancelBatchRenderProcedure=self.renderCallback("cancelBatchRenderProcedure"))
        pm.renderer(self.rendererName, edit=True, showBatchRenderProcedure=self.renderCallback("showBatchRenderProcedure"))
        pm.renderer(self.rendererName, edit=True, showRenderLogProcedure=self.renderCallback("showRenderLogProcedure"))
        pm.renderer(self.rendererName, edit=True, showBatchRenderLogProcedure=self.renderCallback("showBatchRenderLogProcedure"))
        pm.renderer(self.rendererName, edit=True, textureBakingProcedure=self.renderCallback("textureBakingProcedure"))
        pm.renderer(self.rendererName, edit=True, renderRegionProcedure="mayaRenderRegion")
        scriptDir = path.path(__file__).dirname().parent
        
        pm.mel.eval('global string $gImageFormatData[]; $gImageFormatData = {};')
        pm.mel.eval('global string $gPLEImageFormatData[]; $gPLEImageFormatData = {};')
        pm.mel.eval('global string $gPLEImageFormatData_Mental[]; $gPLEImageFormatData_Mental = {};')
        pm.mel.source('createMayaSoftwareCommonGlobalsTab')
        pm.mel.source("unifiedRenderGlobalsWindow")
        self.defineCommonMelProcedures()
        pm.evalDeferred(self.addTabs)

    def defineCommonMelProcedures(self):
        melCmd = """global proc updateMayaSoftwareImageFormatControl(){
            python("{0}");
        }""".replace("{0}", self.baseRenderMelCommand + "updateMayaSoftwareImageFormatControl()")
        pm.mel.eval(melCmd)

        melCmd = """global proc updateMultiCameraBufferNamingMenu(){
            python("{0}");
        }""".replace("{0}", self.baseRenderMelCommand + "updateMultiCameraBufferNamingMenu()")
        pm.mel.eval(melCmd)

        melCmd = """global proc addOneTabToGlobalsWindow(string $renderer, string $tabLabel, string $createProc){
            python("{0}");
        }""".replace("{0}", self.baseRenderMelCommand + "addOneTabToGlobalsWindow(\\\"\" + $renderer + \"\\\", \\\"\" + $tabLabel + \"\\\", \\\"\" + $createProc + \"\\\")")
        pm.mel.eval(melCmd)

    def addOneTabToGlobalsWindow(self, renderer, tabLabel, createProc):
        # no windows no need to call
        if not pm.window('unifiedRenderGlobalsWindow', exists=True):
            return

        displayAllTabs = pm.mel.isDisplayingAllRendererTabs()
        if not displayAllTabs and pm.SCENE.defaultRenderGlobals.currentRenderer.get() != renderer:
            return
        # Set the correct tabLayout parent.
        if displayAllTabs:
            tabLayoutName = pm.mel.getRendererTabLayout(pm.melGlobals['gMasterLayerRendererName'])
        else:
            tabLayoutName = pm.mel.getRendererTabLayout(renderer)
    
        # Hide the tabForm while updating.
        tabFormManagedStatus = pm.formLayout('tabForm', q=True, manage=True)
        pm.formLayout('tabForm', edit=True, manage=False)
        pm.setParent('tabForm')

        if displayAllTabs:
            tabLayoutName = pm.mel.getRendererTabLayout(pm.melGlobals['gMasterLayerRendererName'])
        else:
            tabLayoutName = pm.mel.getRendererTabLayout(renderer)
        pm.setParent(tabLayoutName)
        tabName = pm.mel.rendererTabName(renderer, tabLabel)

        commonTabNames = {
            'Common'             : "m_unifiedRenderGlobalsWindow.kCommon",
            'Passes'             : "m_unifiedRenderGlobalsWindow.kPassesTab",
            'Maya Software'      : "m_unifiedRenderGlobalsWindow.kMayaSoftware",
            'Maya Hardware'      : "m_unifiedRenderGlobalsWindow.kMayaHardware",
            'Maya Vector'        : "m_unifiedRenderGlobalsWindow.kMayaVector",
            'Features'           : "m_unifiedRenderGlobalsWindow.kFeatures",
            'Quality'            : "m_unifiedRenderGlobalsWindow.kQuality",
            'Indirect Lighting'  : "m_unifiedRenderGlobalsWindow.kIndirectLighting",
            'Options'            : "m_unifiedRenderGlobalsWindow.kOptions"
        }
        if commonTabNames.has_key(tabLabel):
            tabLabel = pm.mel.uiRes(commonTabNames[tabLabel])

        if not pm.layout(tabName, exists=True):
            pm.setUITemplate('renderGlobalsTemplate', pushTemplate=True)
            pm.setUITemplate('attributeEditorTemplate', pushTemplate=True)
            pm.formLayout(tabName)
            createProcs = ['createMayaSoftwareCommonGlobalsTab', 'createMayaSoftwareGlobalsTab']
            for renderer in ["arnold", "Appleseed"]:
                try:
                    createProcs.extend(pm.renderer(renderer, q=True, globalsTabCreateProcNames=True))
                except:
                    pass
            if createProc in createProcs:
                pm.mel.eval(createProc)

            pm.setParent('..')
            pm.setUITemplate(popTemplate=True)
            pm.setUITemplate(popTemplate=True)
            pm.tabLayout(tabLayoutName, edit=True, tabLabel=(tabName, tabLabel))
        pm.formLayout('tabForm', edit=True, manage=tabFormManagedStatus)

    def updateMultiCameraBufferNamingMenu(self):
        pass

    def updateMayaSoftwareImageFormatControl(self):
        pass

    def addTabs(self):
        pm.renderer(self.rendererName, edit=True, addGlobalsTab=self.renderTabMelProcedure("CommonGlobals"))
        pm.renderer(self.rendererName, edit=True, addGlobalsTab=self.renderTabMelProcedure("Renderer"))
        pm.renderer(self.rendererName, edit=True, addGlobalsTab=self.renderTabMelProcedure("Environment"))        
        pm.renderer(self.rendererName, edit=True, addGlobalsTab=self.renderTabMelProcedure("Translator"))

    def createImageFormatControls(self):
        self.createGlobalsNode()
        self.createImageFormats()
        if pm.optionMenuGrp("openMayaImageFormats", q=True, exists=True):
            pm.deleteUI("openMayaImageFormats")
        self.imageFormatCtrl = pm.optionMenuGrp("openMayaImageFormats", label="Image Formats", cc=pm.Callback(self.imageFormatCallback))

        for pr in self.imageFormats:
            log.debug("adding image format: " + pr)
            pm.menuItem(pr)
        return self.imageFormatCtrl

    def updateImageFormatControls(self):
        self.createGlobalsNode()
        selectedItem = self.renderGlobalsNode.imageFormat.get()
        enums = self.renderGlobalsNode.imageFormat.getEnums()
        selectedValue = "---"
        for key in enums.keys():
            if enums[key] == selectedItem:
                selectedValue = key
        self.imageFormatCtrl.setValue(selectedValue)

    def imageFormatCallback(self):
        selectedItem = self.imageFormatCtrl.getSelect()
        selectedValue = self.imageFormatCtrl.getValue()
        id = self.renderGlobalsNode.imageFormat.getEnums()[selectedValue]
        selectedFormat = self.imageFormats[id]
        self.renderGlobalsNode.imageFormat.set(id)

    def addUserTabs(self):
        pass

    def unRegisterRenderer(self):
        if pm.renderer(self.rendererName, q=True, exists=True):
            pm.renderer(self.rendererName, unregisterRenderer=True)
        pm.mel.source('createMayaSoftwareCommonGlobalsTab')
        pm.mel.source("unifiedRenderGlobalsWindow")

    def globalsTabCreateProcNames(self):
        pass

    def globalsTabLabels(self):
        pass

    def globalsTabUpdateProcNames(self):
        pass

    def changeIprRegionProcedure(self, *args, **kwargs):
        pass
    
    def iprOptionsProcedure(self):
        print "AppleseediprOptionsProcedure"
    
    def iprOptionsMenuLabel(self):
        print "AppleseedIPROptionsMenuLabel"
        return "AppleseedIPROptionsMenuLabel"

    def iprRenderProcedure(self):
        pass

    def iprRenderSubMenuProcedure(self):
        print "iprRenderSubMenuProcedure"

    def isRunningIprProcedure(self):
        return self.ipr_isrunning

    def pauseIprRenderProcedure(self, editor, pause):
        pass

    def refreshIprRenderProcedure(self):
        pass

    def stopIprRenderProcedure(self):
        self.ipr_isrunning = False

    def startIprRenderProcedure(self, editor, resolutionX, resolutionY, camera):
        self.ipr_isrunning = True

    def logoCallbackProcedure(self):
        pass

    def logoImageName(self):
        return self.rendererName + ".png"

    def renderDiagnosticsProcedure(self):
        pass

    def renderGlobalsProcedure(self):
        pass

    def uiCallback(self, *args):
        pass

    def setImageName(self):
        self.renderGlobalsNode.basePath.set(pm.workspace.path)
        try:
            self.renderGlobalsNode.imagePath.set(pm.workspace.path + pm.workspace.fileRules['images'])
        except:
            self.renderGlobalsNode.imagePath.set(pm.workspace.path + 'images')
        sceneName = ".".join(pm.sceneName().basename().split(".")[:-1])

        # get rid of the number extension which is created by maya if we start a batchrendering from the UI
        numberPart = sceneName.split("__")[-1]
        try:
            number = int(numberPart)
            if number > 999:
                sceneName = sceneName.replace("__" + numberPart, "")
        except:
            pass
        imageName = sceneName
        prefix = pm.SCENE.defaultRenderGlobals.imageFilePrefix.get()
        if prefix:
            if len(prefix) > 0:
                settings = pm.api.MCommonRenderSettingsData()
                pm.api.MRenderUtil.getCommonRenderSettings(settings)

                cams = [cam for cam in pm.ls(type='camera') if cam.renderable.get()]
                if len(cams) < 1:
                    log.error("No renderable camera.")
                    prefix = ""
                prefix = prefix.replace("<Camera>", cams[0].name())
                prefix = prefix.replace("<Scene>", sceneName)
                imageName = prefix

        self.renderGlobalsNode.imageName.set(imageName)
