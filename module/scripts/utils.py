
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

import path
import xml.etree.ElementTree as ET

def getElements(elementList, child):
    elementList[child.attrib['model']] = {}
    for parameter in child:
        #print "\t", parameter.tag, parameter.attrib['name']
        elementList[child.attrib['model']][parameter.attrib['name']] = {}
        for paramElements in parameter:
            if paramElements.attrib['name'] == 'label':
                elementList[child.attrib['model']][parameter.attrib['name']]['label'] =  paramElements.attrib['value']
            #print "\t\t->", paramElements.tag, paramElements.attrib
            if paramElements.attrib['name'] == 'default':
                #print "\t\t\t", paramElements.attrib['name'], paramElements.attrib['value']
                elementList[child.attrib['model']][parameter.attrib['name']]['default'] =  paramElements.attrib['value']
            if paramElements.attrib['name'] == 'type':
                if paramElements.attrib['value'] == 'text':
                    elementList[child.attrib['model']][parameter.attrib['name']]['type'] =  'float'
                if paramElements.attrib['value'] == 'file':
                    elementList[child.attrib['model']][parameter.attrib['name']]['type'] =  'string'
                if paramElements.attrib['value'] == 'entity':
                    #print "\t\t\t", "Uniform value"
                    elementList[child.attrib['model']][parameter.attrib['name']]['type'] =  'message'
                    elementList[child.attrib['model']][parameter.attrib['name']]['default'] =  ""
                if paramElements.attrib['value'] == 'colormap':
                    #print "\t\t\t", "Varying value"
                    elementList[child.attrib['model']][parameter.attrib['name']]['type'] =  'varying'
                if paramElements.attrib['value'] == 'enumeration':
                    elementList[child.attrib['model']][parameter.attrib['name']]['type'] =  'enum'
                if paramElements.attrib['value'] == 'numeric':
                    elementList[child.attrib['model']][parameter.attrib['name']]['type'] =  'uniform_scalar'
                if paramElements.attrib['value'] == 'boolean':
                    elementList[child.attrib['model']][parameter.attrib['name']]['type'] =  'boolean'
                    #print "\t\t\t", "Enum type"
                #bsdfs[child.attrib['model']][parameter.attrib['name']]['default'] =  paramElements.attrib['value']
                #print "\t\t\t--", paramElements.attrib['name'], paramElements.attrib['value']

            if paramElements.attrib['name'] == 'items':
                # print "Items found"
                items = []
                for et in paramElements:
                    #print "Enum", et.attrib['name'], et.attrib['value']
                    items.append(et.attrib['value'])
                elementList[child.attrib['model']][parameter.attrib['name']]['data'] = items

            else:
                for et in paramElements:
                    if "color" in et.attrib['name']:
                        elementList[child.attrib['model']][parameter.attrib['name']]['type'] +=  '_color'
                        #print "\t\t\t", "Type color"
                        break
                    if "texture_instance" in et.attrib['name']:
                        #print "\t\t\t", "Type scalar"
                        elementList[child.attrib['model']][parameter.attrib['name']]['type'] +=  '_scalar'
            #bsdfs[child.attrib['model']][parameter.attrib['name']]


def getShadersFromXML():
    p = "D:/install/3d/appleseed/inputMeta.xml"
    #p = "D:/install/3d/appleseed/inputMetaTst.xml"
    tree = ET.parse(p)
    root = tree.getroot()

    bsdfs = {}
    surfaceShaders = {}
    textures = {}
    lights = {}
    edfs = {}
    for child in root:
        if "bsdf" in child.attrib['type']:
            getElements(bsdfs, child)
        if "surface_shader" in child.attrib['type']:
            getElements(surfaceShaders, child)
        if "light" in child.attrib['type']:
            getElements(lights, child)
        if "edf" in child.attrib['type']:
            getElements(edfs, child)
        if "texture" in child.attrib['type']:
            getElements(textures, child)

    fh = open(r"C:\Users\haggi\coding\OpenMaya\src\mayaToAppleseed\vs2010\sourceCodeDocs\Surfaces.txt", "w")
    for key, value in bsdfs.iteritems():
        fh.write("\n#{0}\n".format(key))
        for paramKey, paramValue in bsdfs[key].iteritems():
            #print paramValue
            print ",".join([paramKey, paramValue['type'], paramValue['label'], paramValue['default']]),
            fh.write(",".join([paramKey, paramValue['type'], paramValue['label'], paramValue['default']]))
            if paramValue.has_key('data'):
                print "," + ":".join(paramValue['data'])
                fh.write("," + ":".join(paramValue['data']) + "\n")
            else:
                fh.write("\n")
                print
    for key, value in surfaceShaders.iteritems():
        fh.write("\n#{0}\n".format(key))
        for paramKey, paramValue in surfaceShaders[key].iteritems():
            #print paramValue
            print ",".join([paramKey, paramValue['type'], paramValue['label'], paramValue['default']]),
            fh.write(",".join([paramKey, paramValue['type'], paramValue['label'], paramValue['default']]))
            if paramValue.has_key('data'):
                print "," + ":".join(paramValue['data'])
                fh.write("," + ":".join(paramValue['data']) + "\n")
            else:
                fh.write("\n")
                print
    for key, value in edfs.iteritems():
        fh.write("\n#{0}\n".format(key))
        for paramKey, paramValue in edfs[key].iteritems():
            print paramValue
            print ",".join([paramKey, paramValue['type'], paramValue['label'], paramValue['default']]),
            fh.write(",".join([paramKey, paramValue['type'], paramValue['label'], paramValue['default']]))
            if paramValue.has_key('data'):
                print "," + ":".join(paramValue['data'])
                fh.write("," + ":".join(paramValue['data']) + "\n")
            else:
                fh.write("\n")
                print
    for key, value in textures.iteritems():
        fh.write("\n#{0}\n".format(key))
        for paramKey, paramValue in textures[key].iteritems():
            print paramValue
            print ",".join([paramKey, paramValue['type'], paramValue['label'], paramValue['default']]),
            fh.write(",".join([paramKey, paramValue['type'], paramValue['label'], paramValue['default']]))
            if paramValue.has_key('data'):
                print "," + ":".join(paramValue['data'])
                fh.write("," + ":".join(paramValue['data']) + "\n")
            else:
                fh.write("\n")
                print
    fh.close()
    #print bsdfs

if __name__ == "__main__":
    getShadersFromXML()
