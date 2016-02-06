
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016 Haggi Krey, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "shadingutils.h"

// appleseed-maya headers.
#include "utilities/logging.h"

// Maya headers.
#include <maya/MFnDependencyNode.h>
#include <maya/MFnParticleSystem.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

bool getObjectShadingGroups(const MObject& geoObject, MObject& sGroup, int instId)
{

    MPlugArray  connections;
    MFnDependencyNode dependNode(geoObject);
    MPlug plug(geoObject, dependNode.attribute("instObjGroups"));

    plug.elementByLogicalIndex(instId).connectedTo(connections, false, true);

    if (connections.length() > 0)
    {
        MObject shadingGroup(connections[0].node());
        if (shadingGroup.apiType() == MFn::kShadingEngine)
        {
            sGroup = shadingGroup;
            return true;
        }
    }
    else
    {
        Logging::debug(MString("Object-instObjGroups has no connection to shading group."));
    }
    return false;
}

bool getObjectShadingGroups(const MDagPath& shapeObjectDP, MObject& shadingGroup)
{
    // if obj is a light, simply return the mobject
    if (shapeObjectDP.hasFn(MFn::kLight))
        shadingGroup = shapeObjectDP.node();

    if (shapeObjectDP.hasFn(MFn::kMesh))
    {
        // Find the Shading Engines Connected to the SourceNode
        MFnMesh fnMesh(shapeObjectDP.node());

        // A ShadingGroup will have a MFnSet
        MObjectArray sets, comps;
        fnMesh.getConnectedSetsAndMembers(shapeObjectDP.instanceNumber(), sets, comps, true);

        // Each set is a Shading Group. Loop through them
        for (unsigned int i = 0; i < sets.length(); ++i)
        {
            MFnDependencyNode fnDepSGNode(sets[i]);
            shadingGroup = sets[i];
            return true;
        }
    }

    if (shapeObjectDP.hasFn(MFn::kNurbsSurface)||shapeObjectDP.hasFn(MFn::kParticle)||shapeObjectDP.hasFn(MFn::kNParticle))
    {

        MObject instObjGroupsAttr;
        if (shapeObjectDP.hasFn(MFn::kNurbsSurface))
        {
            MFnNurbsSurface fnNurbs(shapeObjectDP.node());
            instObjGroupsAttr = fnNurbs.attribute("instObjGroups");
        }
        if (shapeObjectDP.hasFn(MFn::kParticle)||shapeObjectDP.hasFn(MFn::kNParticle))
        {
            MFnParticleSystem fnPart(shapeObjectDP.node());
            instObjGroupsAttr = fnPart.attribute("instObjGroups");
        }
        MPlug instPlug(shapeObjectDP.node(), instObjGroupsAttr);

        // Get the instance that our node is referring to;
        // In other words get the Plug for instObjGroups[intanceNumber];
        MPlug instPlugElem = instPlug.elementByLogicalIndex(shapeObjectDP.instanceNumber());

        // Find the ShadingGroup plugs that we are connected to as Source
        MPlugArray SGPlugArray;
        instPlugElem.connectedTo(SGPlugArray, false, true);

        // Loop through each ShadingGroup Plug
        for (unsigned int i=0; i < SGPlugArray.length(); ++i)
        {
            MFnDependencyNode fnDepSGNode(SGPlugArray[i].node());
            shadingGroup = SGPlugArray[i].node();
            return true;
        }
    }
    return false;
}


//
//  This is the most intersting method because it recognizes per face assignments as well.
//  The per face intArray list will contain a shading group index for every face.
//  Input: DagPath of the shape node
//  Output: perFaceInt array
//          all connected shading groups

bool getObjectShadingGroups(const MDagPath& shapeObjectDP, MIntArray& perFaceAssignments, MObjectArray& shadingGroups, bool needsPerFaceInfo=true)
{
    // if obj is a light, simply return the mobject
    if (shapeObjectDP.node().hasFn(MFn::kLight))
    {
        perFaceAssignments.clear();
        shadingGroups.clear();
        shadingGroups.append(shapeObjectDP.node());
        return true;
    }

    if (shapeObjectDP.node().hasFn(MFn::kMesh))
    {
        // Find the Shading Engines Connected to the SourceNode
        MFnMesh meshFn(shapeObjectDP.node());

        perFaceAssignments.clear();
        shadingGroups.clear();
        MObjectArray comps;

        // this one seems to be extremly much faster if we need no per face informations.
        // e.g. for a sphere with 90000 faces, the non per face method needs 0.05 sec. whereas the
        // method with per face info needs about 20 sec.
        if (!needsPerFaceInfo)
        {
            meshFn.getConnectedSetsAndMembers(shapeObjectDP.instanceNumber(), shadingGroups, comps, true);
            return true;
        }

        meshFn.getConnectedShaders(shapeObjectDP.instanceNumber(), shadingGroups, perFaceAssignments);

        if (!meshFn.findPlug("displaySmoothMesh").asBool())
            return true;

        MIntArray indices;
        indices = perFaceAssignments;

        int subdivs = 0;
        int multiplier = 0;

        if (meshFn.findPlug("displaySmoothMesh").asBool())
        {
            MMeshSmoothOptions options;
            MStatus status = meshFn.getSmoothMeshDisplayOptions(options);
            if (status)
            {
                if (!meshFn.findPlug("useSmoothPreviewForRender", false, &status).asBool())
                {
                    int smoothLevel = meshFn.findPlug("renderSmoothLevel", false, &status).asInt();
                    options.setDivisions(smoothLevel);
                }

                subdivs = options.divisions();
                if (subdivs > 0)
                    multiplier = static_cast<int> (pow(4.0f, (subdivs - 1)));
            }
        }

        if (multiplier > 0)
            perFaceAssignments.clear();

        for (unsigned int i = 0; i < indices.length(); i++)
        {
            int subdivisions = multiplier * meshFn.polygonVertexCount(i);
            int index = 0 > indices[i] ? 0 : indices[i]; // non assigned has -1, but we want 0
            perFaceAssignments.append(index);

            // simply replicate the index for all subdiv faces
            for (int k = 0; k < subdivisions - 1; k++)
            {
                perFaceAssignments.append(index);
            }
        }
        return true;
    }

    if (shapeObjectDP.node().hasFn(MFn::kNurbsSurface)||shapeObjectDP.hasFn(MFn::kParticle)||shapeObjectDP.hasFn(MFn::kNParticle))
    {

        MObject instObjGroupsAttr;
        if (shapeObjectDP.hasFn(MFn::kNurbsSurface))
        {
            MFnNurbsSurface fnNurbs(shapeObjectDP.node());
            instObjGroupsAttr = fnNurbs.attribute("instObjGroups");
        }
        if (shapeObjectDP.hasFn(MFn::kParticle)||shapeObjectDP.hasFn(MFn::kNParticle))
        {
            MFnParticleSystem fnPart(shapeObjectDP.node());
            instObjGroupsAttr = fnPart.attribute("instObjGroups");
        }
        MPlug instPlug(shapeObjectDP.node(), instObjGroupsAttr);

        // Get the instance that our node is referring to;
        // In other words get the Plug for instObjGroups[intanceNumber];
        MPlug instPlugElem = instPlug.elementByLogicalIndex(shapeObjectDP.instanceNumber());

        // Find the ShadingGroup plugs that we are connected to as Source
        MPlugArray SGPlugArray;
        instPlugElem.connectedTo(SGPlugArray, false, true);

        perFaceAssignments.clear();
        shadingGroups.clear();

        // Loop through each ShadingGroup Plug
        for (unsigned int i=0; i < SGPlugArray.length(); ++i)
        {
            MFnDependencyNode fnDepSGNode(SGPlugArray[i].node());
            shadingGroups.append(SGPlugArray[i].node());
            return true;
        }
    }
    return false;
}
