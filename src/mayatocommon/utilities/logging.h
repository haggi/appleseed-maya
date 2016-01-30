
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

#ifndef MTM_LOGGING_H
#define MTM_LOGGING_H

#include <maya/MString.h>
#include <maya/MTimerMessage.h>
#include <maya/MStreamUtils.h>

#define COUT(msg) MStreamUtils::stdOutStream() << msg << "\n"

class Logging
{
public:
    enum LogLevel{
        LevelInfo = 0,
        LevelError = 1,
        LevelWarning = 2,
        LevelProgress = 3,
        LevelDebug = 4,
        LevelNone = 5
    };
    enum OutputType{
        ScriptEditor,
        OutputWindow
    };

    static void setLogLevel( Logging::LogLevel level);
    static void info(MString logString);
    static void warning(MString logString);
    static void error(MString logString);
    static void debug(MString logString);
    static void debugs(MString logString, int level);
    static void progress(MString logString);
    static void detail(MString logString);
};

MString makeSpace(int level);
static  Logging::LogLevel log_level = Logging::LevelInfo;
static  Logging::OutputType log_outtype = Logging::ScriptEditor;

#endif
