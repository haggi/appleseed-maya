import math, sys

import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx
import Appleseed.metaShaderNode as meta
import Appleseed.shadingNodes as shadingNodes
        
def initializePlugin(mobject):
    mplugin = OpenMayaMPx.MFnPlugin(mobject)    
    shadingNodes.registerShadingNodes(mplugin)

def uninitializePlugin(mobject):
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    shadingNodes.unregisterShadingNodes(mplugin)
    
