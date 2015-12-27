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
        print "UberShader AE buildBody"
        self.beginLayout("Matte", collapse=True)
        self.endLayout()
                
        #self.addAOVLayout(aovReorder = ['direct_diffuse', 'indirect_diffuse', 'direct_specular', 'indirect_specular', 'direct_specular2', 'indirect_specular2', 'reflection', 'refraction', 'emission', 'sss'])
        
        self.update(nodeName)