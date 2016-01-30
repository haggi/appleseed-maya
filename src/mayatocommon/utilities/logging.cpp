
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

#include "threads/queue.h"
#include "utilities/logging.h"
#include "memory/memoryinfo.h"
#include <maya/MGlobal.h>
#include <stdio.h>

void Logging::setLogLevel(Logging::LogLevel level)
{
    if (level == Logging::LevelDebug)
    {
        MGlobal::displayInfo("Set logging level to DEBUG");
    }
    if (level == Logging::LevelInfo)
    {
        MGlobal::displayInfo("Set logging level to INFO");
    }
    if (level == Logging::LevelWarning)
    {
        MGlobal::displayInfo("Set logging level to WARNING");
    }
    if (level == Logging::LevelError)
    {
        MGlobal::displayInfo("Set logging level to ERROR");
    }
    if (level == Logging::LevelProgress)
    {
        MGlobal::displayInfo("Set logging level to PROGRESS");
    }
    log_level = level;
}

void Logging::info(MString logString)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    if (log_level >= Logging::LevelInfo)
        COUT(outString);
}

void Logging::warning(MString logString)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    if (log_level >= Logging::LevelWarning)
        COUT(outString);
}

void Logging::error(MString logString)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    if (log_level >= Logging::LevelError)
    {
        COUT(outString);
    }
}

void Logging::debug(MString logString)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    if (log_level >= Logging::LevelDebug)
    {
        COUT(outString);
    }
}

void Logging::debugs(MString logString, int level)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " + makeSpace(level) + logString;
    if (log_level >= Logging::LevelDebug)
    {
        COUT(outString);
    }
}

void Logging::progress(MString logString)
{
    if (log_level == 5)
        return;
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    if (log_level >= Logging::LevelProgress)
    {
        COUT(outString);
    }
}

void Logging::detail(MString logString)
{
    MString outString = MString("Mem: ") + getCurrentUsage() + "MB " +  logString;
    COUT(outString);
}

MString makeSpace(int level)
{
    MString space;
    for (int i = 0; i < level; i++)
        space += "\t";
    return space;
}
