import pymel.core as pm
import logging
import Renderer.OSLTools

log = logging.getLogger("ui")

class BaseTemplate(pm.ui.AETemplate):

    def addControl(self, control, label=None, **kwargs):
        pm.ui.AETemplate.addControl(self, control, label=label, **kwargs)
    
    def beginLayout(self, name, collapse=True):
        pm.ui.AETemplate.beginLayout(self, name, collapse=collapse)

class AEuberShaderTemplate(BaseTemplate):
    def __init__(self, nodeName):
        Renderer.OSLTools.readShadersXMLDescription()
        self.shaderDesc = {}
        if Renderer.OSLTools.SHADER_DICT.has_key('uberShader'):
            inDesc = Renderer.OSLTools.SHADER_DICT['uberShader']['inputs']
            print "AEuberShaderTemplate osl shder dict inputs:", inDesc
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
        #self.addControl("SSS1Weight", label="Color Multiplier", changeCommand=self.update)
        #self.addControl("SSS1Radius", label="Radius per Color", changeCommand=self.update)
        #self.addControl("SSS1RadiusMultiplier", label="Radius Multiplier", changeCommand=self.update)
        self.addControl("SSS1RadiusMultiplier", label="Radius", changeCommand=self.update)
        self.endLayout()
        
        self.update(nodeName)