
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
#include "appleseedmaya.h"

// appleseed-maya headers.
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "event.h"
#include "renderqueue.h"
#include "world.h"

// Maya headers.
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MSyntax.h>

MSyntax AppleseedMaya::syntaxCreator()
{
    MSyntax syntax;
    syntax.addFlag("-cam", "-camera", MSyntax::kString);
    syntax.addFlag("-s", "-state");
    syntax.addFlag("-wi", "-width", MSyntax::kLong);
    syntax.addFlag("-hi", "-height", MSyntax::kLong);
    syntax.addFlag("-sar", "-startIpr");
    syntax.addFlag("-str", "-stopIpr");
    syntax.addFlag("-par", "-pauseIpr");
    syntax.addFlag("-uir", "-updateIprRegion");
    return syntax;
}

void* AppleseedMaya::creator()
{
    return new AppleseedMaya();
}

namespace
{
    void setLogLevel()
    {
        const MObject globalsObj = getRenderGlobalsNode();

        if (globalsObj == MObject::kNullObj)
            Logging::setLogLevel(Logging::LevelDebug);
        else
        {
            const MFnDependencyNode globalsNode(globalsObj);
            const int logLevel = getIntAttr("translatorVerbosity", globalsNode, 0);
            Logging::setLogLevel(static_cast<Logging::LogLevel>(logLevel));
        }
    }
}

MStatus AppleseedMaya::doIt(const MArgList& args)
{
    MStatus stat = MStatus::kSuccess;
    MGlobal::displayInfo("Executing appleseed-maya...");

    setLogLevel();

    MArgDatabase argData(syntax(), args);

    if (argData.isFlagSet("-state", &stat))
    {
        if (getWorldPtr()->getRenderState() == World::RSTATETRANSLATING)
            setResult("rstatetranslating");
        if (getWorldPtr()->getRenderState() == World::RSTATERENDERING)
            setResult("rstaterendering");
        if (getWorldPtr()->getRenderState() == World::RSTATEDONE)
            setResult("rstatedone");
        if (getWorldPtr()->getRenderState() == World::RSTATENONE)
            setResult("rstatenone");
        if (getWorldPtr()->getRenderState() == World::RSTATESTOPPED)
            setResult("rstatestopped");
        return MS::kSuccess;
    }

    if (argData.isFlagSet("-updateIprRegion", &stat))
    {
        iprUpdateRenderRegion();
        return MS::kSuccess;
    }

    if (argData.isFlagSet("-stopIpr", &stat))
    {
        getWorldPtr()->mRenderer->abortRendering();
        waitUntilRenderFinishes();
        return MS::kSuccess;
    }

    if (argData.isFlagSet("-pauseIpr", &stat))
    {
        getWorldPtr()->mRenderer->abortRendering();
        return MS::kSuccess;
    }

    World::RenderType renderType = World::UIRENDER;

    MFnDependencyNode defaultGlobals(objectFromName("defaultResolution"));
    int width = defaultGlobals.findPlug("width").asInt();
    int height = defaultGlobals.findPlug("height").asInt();

    // I have to request useRenderRegion here because as soon the command is finished,
    // what happens immediately after the command is put into the queue, the value is
    // set back to false.
    MFnDependencyNode drgfn(objectFromName("defaultRenderGlobals"));
    bool doRenderRegion = drgfn.findPlug("useRenderRegion").asBool();

    if (argData.isFlagSet("-startIpr", &stat))
        renderType = World::IPRRENDER;

    if (argData.isFlagSet("-width", &stat))
        argData.getFlagArgument("-width", 0, width);

    if (argData.isFlagSet("-height", &stat))
        argData.getFlagArgument("-height", 0, height);

    MDagPath cameraDagPath;
    if (argData.isFlagSet("-camera", &stat))
    {
        MSelectionList selectionList;
        argData.getFlagArgument("-camera", 0, selectionList);
        selectionList.getDagPath(0, cameraDagPath);
        cameraDagPath.extendToShape();
    }

    initRender(renderType, width, height, cameraDagPath, doRenderRegion);

    return MStatus::kSuccess;
}
