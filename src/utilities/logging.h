
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

#ifndef UTILITIES_LOGGING_H
#define UTILITIES_LOGGING_H

class MString;

class Logging
{
  public:
    enum LogLevel
    {
        LevelInfo = 0,
        LevelError = 1,
        LevelWarning = 2,
        LevelProgress = 3,
        LevelDebug = 4,
        LevelNone = 5
    };

    enum OutputType
    {
        ScriptEditor,
        OutputWindow
    };

    static void setLogLevel(const LogLevel level);
    static void info(const MString& message);
    static void warning(const MString& message);
    static void error(const MString& message);
    static void debug(const MString& message);
    static void progress(const MString& message);
};

static Logging::LogLevel log_level = Logging::LevelInfo;
static Logging::OutputType log_outtype = Logging::ScriptEditor;

#endif  // !UTILITIES_LOGGING_H
