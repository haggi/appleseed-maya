
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
import path
import pprint
import pymel.core as pm
import os
import shutil
import subprocess
import sys
import xml.etree.cElementTree as ET
import xml.dom.minidom as minidom

SHADER_DICT = {}

log = logging.getLogger("renderLogger")

def getShaderInfo(shaderPath):
    osoFiles = getOSOFiles(shaderPath)
    return osoFiles

def getOSODirs(renderer = "appleseed"):
    try:
        shaderDir = os.environ['{0}_OSL_SHADERS_LOCATION'.format(renderer.upper())]
    except KeyError:
        shaderDir = path.path(__file__).parent.parent.parent + "/shaders"
    osoDirs = set()
    for root, dirname, files in os.walk(shaderDir):
        for filename in files:
            if filename.endswith(".oso"):
                osoDirs.add(root.replace("\\", "/"))
    return list(osoDirs)

def getOSOFiles(renderer = "appleseed"):
    try:
        shaderDir = os.environ['{0}_OSL_SHADERS_LOCATION'.format(renderer.upper())]
    except KeyError:
        shaderDir = path.path(__file__).parent.parent.parent + "/shaders"
    osoFiles = set()
    for root, dirname, files in os.walk(shaderDir):
        for filename in files:
            if filename.endswith(".oso"):
                osoFiles.add(os.path.join(root, filename).replace("\\", "/"))
    return list(osoFiles)

def getOSLFiles(renderer = "appleseed"):
    try:
        shaderDir = os.environ['{0}_OSL_SHADERS_LOCATION'.format(renderer.upper())]
    except KeyError:
        shaderDir = path.path(__file__).parent.parent.parent + "/shaders"
    osoFiles = set()
    for root, dirname, files in os.walk(shaderDir):
        for filename in files:
            if filename.endswith(".osl"):
                osoFiles.add(os.path.join(root, filename).replace("\\", "/"))
    return list(osoFiles)

def reverseValidate(pname):
    if pname == "inMin":
        return "min"
    if pname == "inMax":
        return "max";
    if pname == "inVector":
        return "vector";
    if pname == "inMatrix":
        return "matrix";
    if pname == "inColor":
        return "color";
    if pname == "outOutput":
        return "output";
    return pname;

def analyzeContent(content):
    d = {}
    currentElement = None
    for line in content:
        if len(line) == 0:
            continue
        if line.startswith("shader") or line.startswith("surface"):
            d['name'] = line.split(" ")[1].replace("\"", "")
            d['mayaClassification'] = ""
            d['mayaId'] = 0
            d['help'] = ""
            d['inputs'] = []
            d['outputs'] = []
            currentElement = d
        else:
            if line.startswith("Default value"):
                currentElement['default'] = line.split(" ")[-1].replace("\"", "")
                if currentElement.has_key("type"):
                    if currentElement["type"] in ["color", "vector", "output vector"]:
                        vector = line.split("[")[-1].split("]")[0]
                        vector = vector.strip()
                        currentElement['default'] = map(float, vector.split(" "))
            if line.startswith("Unknown default value"):
                if currentElement.has_key("type"):
                    if currentElement["type"] in ["color", "vector"]:
                        currentElement['default'] = "[ 0.0, 0.0, 0.0 ]"
            if line.startswith("metadata"):
                if "options = " in line:
                    currentElement['options'] = line.split(" ")[-1].replace("\"", "").split("|")
                if "hint = " in line:
                    currentElement['hint'] = line.split(" ")[-1].replace("\"", "")
                if "min = " in line:
                    currentElement['min'] = line.split(" ")[-1]
                if "max = " in line:
                    currentElement['max'] = line.split(" ")[-1]
                if "help = " in line:
                    currentElement['help'] = " ".join(line.split("=")[1:]).replace("\"", "").strip()
                if "mayaClassification = " in line:
                    currentElement['mayaClassification'] = " ".join(line.split("=")[1:]).replace("\"", "").strip()
                if "mayaId = " in line:
                    currentElement['mayaId'] = int(line.split("=")[-1])
            if line.startswith("\""): # found a parameter
                currentElement = {}
                elementName = line.split(" ")[0].replace("\"", "")
                currentElement['name'] = reverseValidate(elementName)
                currentElement['type'] = " ".join(line.split(" ")[1:]).replace("\"", "")
                
                if "output" in line:
                    d['outputs'].append(currentElement)
                    currentElement = d['outputs'][-1]
                else:                    
                    d['inputs'].append(currentElement)
                    currentElement = d['inputs'][-1]
    return d

def readShadersXMLDescription():
    xmlFile = path.path(__file__).parent.parent.parent / "resources/shaderdefinitions.xml"
    if not xmlFile.exists():
        return
    try:
        tree = ET.parse(xmlFile)
    except:
        return
    shaders = tree.getroot()
    shaderDict = {}
    for shader in shaders:
        shDict = {}
        shDict['name'] = shader.find('name').text
        shDict['mayaClassification'] = ""
        element = shader.find('mayaClassification')
        if element is not None:
            shDict['mayaClassification'] = element.text
        shDict['mayaId'] = 0
        element = shader.find('mayaId')
        if element is not None:
            shDict['mayaId'] = int(element.text)
        shDict['help'] = ""
        element = shader.find('help')
        if element is not None:
            shDict['help'] = element.text
        shDict['inputs'] = []
        shDict['outputs'] = []
        for inp in shader.find('inputs'):
            inpp = {}
            inpp['name'] = inp.find('name').text
            inpp['type'] = inp.find('type').text
            inpp['help'] = ""
            inpp['hint'] = ""
            inpp['min'] = 0
            inpp['max'] = 1
            inpp['default'] = 0
            findElement = inp.find('help')
            if findElement is not None:
                inpp['help'] = findElement.text
            findElement = inp.find('hint')
            if findElement is not None:
                inpp['hint'] = inp.find('hint').text
            findElement = inp.find('min')
            if findElement is not None:
                inpp['min'] = inp.find('min').text
            findElement = inp.find('max')
            if findElement is not None:
                inpp['max'] = inp.find('max').text
            findElement = inp.find('default')
            if findElement is not None:
                inpp['default'] = inp.find('default').text
            findElement = inp.find('options')
            if findElement is not None:
                inpp['options'] = findElement.text
            shDict['inputs'].append(inpp)

        for inp in shader.find('outputs'):
            inpp = {}
            inpp['name'] = inp.find('name').text
            inpp['type'] = inp.find('type').text
            inpp['help'] = ""
            shDict['outputs'].append(inpp)
        shaderDict[shDict['name']] = shDict

    global SHADER_DICT
    SHADER_DICT = shaderDict
    return shaderDict

def addSubElementList(listEntry, parentElement, subName = "input"):
    for element in listEntry:
        inElement = ET.SubElement(parentElement,subName)
        for ikey, ivalue in element.iteritems():
            subElement = ET.SubElement(inElement,ikey)
            subElement.text = str(ivalue)

def writeXMLShaderDescription(shaderDict=None):
    global SHADER_DICT
    if shaderDict is None:
        shaderDict = SHADER_DICT
    xmlFile = path.path(__file__).parent.parent.parent / "resources/shaderdefinitions.xml"
    root = ET.Element('shaders')
    for shaderKey in shaderDict.keys():
        shader = shaderDict[shaderKey]
        sh = ET.SubElement(root,"shader")
        for key, value in shader.iteritems():
            if key == "inputs":
                ins = ET.SubElement(sh,"inputs")
                addSubElementList(value, ins, subName="input")
            elif key == "outputs":
                ins = ET.SubElement(sh,"outputs")
                addSubElementList(value, ins, subName="output")
            else:
                subElement = ET.SubElement(sh,key)
                subElement.text = str(value)
    tree = ET.ElementTree(root)
    tree.write(xmlFile)
    log.debug("Writing shader info file: {0}".format(xmlFile))
    # just make it nice to read
    xml = minidom.parse(xmlFile)
    pretty_xml_as_string = xml.toprettyxml()
    root = ET.fromstring(pretty_xml_as_string)
    tree = ET.ElementTree(root)
    tree.write(xmlFile)

def updateOSLShaderInfo(force=False, osoFiles=[]):
    pp = pprint.PrettyPrinter(indent=4)
    IDLE_PRIORITY_CLASS = 64
    cmd = "oslinfo -v"
    infoDict = {}
    # if we have updates we need to update the xml file as well.
    # first read the xml file
    readShadersXMLDescription()
    global SHADER_DICT
    for osoFile in osoFiles:
        infoCmd = cmd + " " + osoFile
        shaderName = path.path(osoFile).basename().replace(".oso", "")
        process = subprocess.Popen(infoCmd, bufsize=1, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, creationflags=IDLE_PRIORITY_CLASS)
        content = []
        while 1:
            line = process.stdout.readline()
            line = line.strip()
            content.append(line)
            if not line: break
        infoDict[shaderName] = analyzeContent(content)
        SHADER_DICT[shaderName] = infoDict[shaderName]
        #pp.pprint(infoDict)
    writeXMLShaderDescription()
    return infoDict


def compileAllShaders(renderer = "appleseed"):
    try:
        shaderDir = os.environ['{0}_OSL_SHADERS_LOCATION'.format(renderer.upper())]
    except KeyError:
        # we expect this file in module/scripts so we can try to find the shaders in ../shaders
        log.error("Could not get path from APPLESEED_OSL_SHADERS_LOCATION. Trying to find the shaders dir relative to current file: {0}".format(__file__))
        shaderDir = path.path(__file__).parent.parent.parent / "shaders"
        if shaderDir.exists():
            log.info("Using found shaders directory {0}".format(shaderDir))

    include_dir = os.path.join(shaderDir, "src/include")
    oslc_cmd = "oslc"
    failureDict = {}
    osoInfoShaders = []
    for root, dirname, files in os.walk(shaderDir):
        for filename in files:
            if filename.endswith(".osl"):
                oslPath =  os.path.join(root, filename)
                dest_dir = root.replace("\\", "/").replace("shaders/src", "shaders") + "/"
                if not os.path.exists(dest_dir):
                    os.makedirs(dest_dir)
                osoOutputPath = dest_dir + filename.replace(".osl", ".oso")
                osoOutputFile = path.path(osoOutputPath)
                oslInputFile = path.path(oslPath)

                if osoOutputFile.exists():
                    if osoOutputFile.mtime > oslInputFile.mtime:
                        continue
                    else:
                        osoOutputFile.remove()
                        
                saved_wd = os.getcwd()
                os.chdir(root)
                compileCmd = oslc_cmd + " -v -I" + include_dir + ' -o '+ osoOutputPath + ' ' + oslInputFile
                IDLE_PRIORITY_CLASS = 64
                process = subprocess.Popen(compileCmd, bufsize=1, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, creationflags=IDLE_PRIORITY_CLASS)
                progress = []
                fail = False
                while 1:
                    line = process.stdout.readline()
                    if not line: break
                    log.debug(line)
                    line.strip()
                    progress.append(line)
                    pm.mel.trace(line.strip())
                    if "error" in line:
                        fail = True
                if fail:
                    failureDict[osoOutputFile.basename()] = progress
                else:
                    osoInfoShaders.append(osoOutputPath)
                os.chdir(saved_wd)

    if len(failureDict.keys()) > 0:
        log.info("\n\nShader compilation failed for:")
        for key, content in failureDict.iteritems():
            log.info("Shader {0}\n{1}\n\n".format(key, "\n".join(content)))
    else:
        if len(osoInfoShaders) > 0:
            updateOSLShaderInfo(force=False, osoFiles=osoInfoShaders)
