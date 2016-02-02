
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
#include "mayatoappleseed.h"

// appleseed-maya headers.
#include "threads/renderqueueworker.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "mayatoworld.h"

// Maya headers.
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MSyntax.h>

MSyntax MayaToAppleseed::syntaxCreator()
{
    MSyntax syntax;
    syntax.addFlag("-cam", "-camera", MSyntax::kString);
    syntax.addFlag("-s", "-state");
    syntax.addFlag("-wi", "-width", MSyntax::kLong);
    syntax.addFlag("-hi", "-height", MSyntax::kLong);
    syntax.addFlag("-sar", "-startIpr");
    syntax.addFlag("-str", "-stopIpr");
    syntax.addFlag("-par", "-pauseIpr");
    return syntax;
}

void* MayaToAppleseed::creator()
{
    return new MayaToAppleseed();
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

MStatus MayaToAppleseed::doIt(const MArgList& args)
{
    MStatus stat = MStatus::kSuccess;
    MGlobal::displayInfo("Executing MayaToAppleseed...");

    setLogLevel();

    MArgDatabase argData(syntax(), args);

    if (argData.isFlagSet("-state", &stat))
    {
        if (getWorldPtr()->renderState == MayaToWorld::RSTATETRANSLATING)
            setResult("rstatetranslating");
        if (getWorldPtr()->renderState == MayaToWorld::RSTATERENDERING)
            setResult("rstaterendering");
        if (getWorldPtr()->renderState == MayaToWorld::RSTATEDONE)
            setResult("rstatedone");
        if (getWorldPtr()->renderState == MayaToWorld::RSTATENONE)
            setResult("rstatenone");
        if (getWorldPtr()->renderState == MayaToWorld::RSTATESTOPPED)
            setResult("rstatestopped");
        return MS::kSuccess;
    }

    if (argData.isFlagSet("-stopIpr", &stat))
    {
        Event e;
        e.type = Event::IPRSTOP;
        theRenderEventQueue()->push(e);
        return MS::kSuccess;
    }

    if (argData.isFlagSet("-pauseIpr", &stat))
    {
        Event e;
        e.type = Event::IPRPAUSE;
        theRenderEventQueue()->push(e);
        return MS::kSuccess;
    }

    // I have to request useRenderRegion here because as soon the command is finished,
    // what happens immediatly after the command is put into the queue, the value is
    // set back to false.
    std::auto_ptr<CmdArgs> cmdArgs(new CmdArgs);
    MObject drg = objectFromName("defaultRenderGlobals");
    MFnDependencyNode drgfn(drg);
    cmdArgs->useRenderRegion = drgfn.findPlug("useRenderRegion").asBool();

    if (argData.isFlagSet("-startIpr", &stat))
        cmdArgs->renderType = MayaToWorld::IPRRENDER;

    if (argData.isFlagSet("-width", &stat))
        argData.getFlagArgument("-width", 0, cmdArgs->width);

    if (argData.isFlagSet("-height", &stat))
        argData.getFlagArgument("-height", 0, cmdArgs->height);

    if (argData.isFlagSet("-camera", &stat))
    {
        MSelectionList selectionList;
        argData.getFlagArgument("-camera", 0, selectionList);

        MDagPath camera;
        selectionList.getDagPath(0, camera);
        camera.extendToShape();
        cmdArgs->cameraDagPath = camera;
    }

    Event e;
    e.cmdArgsData.reset(cmdArgs.release());
    e.type = Event::INITRENDER;
    theRenderEventQueue()->push(e);

    if (MGlobal::mayaState() == MGlobal::kBatch)
        RenderQueueWorker::startRenderQueueWorker();

    return MStatus::kSuccess;
}
