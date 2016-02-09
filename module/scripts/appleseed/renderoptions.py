
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

import sys
import os

sys.path.append("C:/daten/install/3d/appleseed/appleseed/bin")
sys.path.append("C:/daten/install/3d/appleseed/appleseed/bin/appleseed")

os.environ["PYTHONPATH"] = os.environ["PYTHONPATH"] + ";C:/daten/install/3d/appleseed/appleseed/bin/appleseed"
os.environ["PATH"] = os.environ["PYTHONPATH"] + ";C:/daten/install/3d/appleseed/appleseed/bin"

import appleseed

md = appleseed.Configuration.get_metadata()
fh=open("c:/daten/params.txt", "w")

attributeList = []
def analyzeAttr(attr, indent="\t", name=""):
    attribute = {'name':"", 'type':"int", 'default': 0, 'min': 0, 'max':1, 'options':[], 'help':""}
    attribute['name'] = name
    if "type" in attr.keys():
        if attr.has_key("type"):
            attribute['type'] = attr['type']
            #print indent,"Type", attr['type']
            if attr['type'] == "enum":
                if attr.has_key("options"):
                    #print indent,
                    for option in attr['options'].keys():
                        #print option+",",
                        attribute['options'].append(option)
                    #print ""

        if attr.has_key("default"):
            #print indent,"Default", attr['default']
            attribute['default'] = attr['default']
        if attr.has_key("help"):
            attribute['help'] = attr['help']

        attributeList.append(attribute)
    else:
        for newKey, newAttr in attr.iteritems():
            #print indent,"Attr",newKey
            analyzeAttr(newAttr, indent+"\t", newKey)

for k, m in md.iteritems():
    print "Segment: ", k, m
    #print m
    fh.write(k + " " + str(m) + "\n")

    analyzeAttr(m, "\t", k)

anames = []
for attr in attributeList:
    print attr['name'], attr['default']
    anames.append(attr['name'])

anames.sort()
for attr in anames:
    print attr

print "-----------------"
fh = open(r"H:\UserDatenHaggi\Documents\coding\OpenMaya\src\mayaToAppleseed\src\mtap_common\mtap_renderGlobalsNode.cpp", "r")
content = fh.readlines()
fh.close()

cppnames = []
for line in content:
    line = line.strip()
    if line.startswith("MObject MayaToAppleseedGlobals::"):
        cppnames.append(line.split("::")[-1][:-1])
cppnames.sort()
for attr in cppnames:
    print attr

    #for pkey, param in m.iteritems():
    #    print "\t", pkey, param
        #if isinstance(param, type({})):
        #    if param.has_key("type"):
        #        print "ParamType", param['type']
        #else:
        #    print "Simple param", param
#         parameter = param
#         if not isinstance(param, type({})):
#             parameter = m
#
#         try:
#             print "Type", parameter['type']
#         except:
#             print "Error", parameter

fh.close()
