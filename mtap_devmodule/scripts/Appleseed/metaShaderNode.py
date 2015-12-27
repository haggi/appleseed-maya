import maya.OpenMaya as om
import maya.OpenMayaMPx as OpenMayaMPx

nodeId = om.MTypeId(0x87000)

class MetaClass(OpenMayaMPx.MPxNode):
    @classmethod
    def nodeCreator(cls):
        return OpenMayaMPx.asMPxPtr( cls() )
    @classmethod
    def nodeInitializer(cls):
        nAttr = om.MFnNumericAttribute()
        for output in cls.data['outputs']:
            print "register OutputAttribute", output
            if output['type'] == 'output pointer': #this is a OSL closure name 
                print "Adding output", output['name']
                cls.output = nAttr.createColor(output['name'],output['name'])
                output['attr'] = cls.output
                cls.addAttribute( cls.output )
            if output['type'] == 'color':
                print "Adding output", output['name']
                cls.output = nAttr.createColor(output['name'],output['name'])
                output['attr'] = cls.output
                cls.addAttribute( cls.output )
            if output['type'] == 'float':
                cls.output = nAttr.create(output['name'],output['name'], om.MFnNumericData.kFloat)
                output['attr'] = cls.output
                cls.addAttribute( cls.output )
        for inputElement in cls.data['inputs']:
            print "register InputAttribute", inputElement
            if inputElement['type'] == 'color':
                cls.input = nAttr.createColor(inputElement['name'],inputElement['name'])
                if inputElement.has_key('default'):
                    nAttr.setDefault(0.0, 0.0, 0.0)
                inputElement['attr'] = cls.input
                cls.addAttribute( cls.input )
                cls.attributeAffects(cls.input, cls.output)
            if inputElement['type'] == 'float':
                cls.input = nAttr.create(inputElement['name'],inputElement['name'], om.MFnNumericData.kFloat)
                if inputElement.has_key('default'):
                    nAttr.setDefault(float(inputElement['default']))
                inputElement['attr'] = cls.input
                cls.addAttribute( cls.input )
                cls.attributeAffects(cls.input, cls.output)
            if inputElement['type'] == 'int':
                cls.input = nAttr.create(inputElement['name'],inputElement['name'], om.MFnNumericData.kInt)
                if inputElement.has_key('default'):
                    nAttr.setDefault(int(inputElement['default']))
                inputElement['attr'] = cls.input
                cls.addAttribute( cls.input )
                cls.attributeAffects(cls.input, cls.output)
    
    def compute(self, plug, dataBlock):
        
        found = False
        for output in type(self).data['outputs']:
            if plug == output['attr']:
                print "Compute output attribut found", output['name']
                if output['type'] == 'color':
                    resultColor = om.MFloatVector(0.0,0.0,0.0)
                    outColorHandle = dataBlock.outputValue( output['attr'] )
                    outColorHandle.setMFloatVector(resultColor)
                    outColorHandle.setClean()
                    found = True
                if output['type'] == 'float':
                    result = 0.0
                    outFloatHandle = dataBlock.outputValue( output['attr'] )
                    outFloatHandle.setFloat(result)
                    outFloatHandle.setClean()
                    found = True
        if not found:
            return om.kUnknownParameter
    
    def __init__(self):
        OpenMayaMPx.MPxNode.__init__(self)

class TestClass(object):
    @classmethod
    def create(cls):
        for key in cls.data.keys():
            #print "Element:",key,"data",cls.data[key]
            if key == "inParams":
                print "Creating input parameters"
                paramList = cls.data["inParams"]
            for p in paramList:
                print "\tName", p["name"]
                print "\tType", p["type"]
            if key == "outParams":
                print "Creating out parameter"
                paramList = cls.data["outParams"]
            for p in paramList:
                print "\tName", p["name"]
                print "\tType", p["type"]



def createDynamicNodes():
    plugin = OpenMayaMPx.MFnPlugin.findPlugin("testPlugin")
    print "Plugin:", plugin

#nodes = []
#for key in nodeDict.keys():
#    print "CreateNode", key
#    X = type(key, (TestClass,), dict(data=nodeDict[key]))
#    nodes.append(X)

#for n in nodes:
#    n.create()