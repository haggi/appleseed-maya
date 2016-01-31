import math, sys

import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx
import appleseed.metashadernode as meta
import appleseed.shadingnodes as shadingNodes

def initializePlugin(mobject):
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    shadingNodes.registerShadingNodes(mplugin)

def uninitializePlugin(mobject):
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    shadingNodes.unregisterShadingNodes(mplugin)
