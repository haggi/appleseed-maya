
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
#include "logging.h"

// appleseed.foundation headers.
#include "foundation/platform/system.h"
#include "foundation/platform/types.h"

// Maya headers.
#include <maya/MGlobal.h>
#include <maya/MStreamUtils.h>
#include <maya/MString.h>

namespace
{
    MString get_level_name(const Logging::LogLevel level)
    {
        switch (level)
        {
          case Logging::LevelInfo: return "INFO";
          case Logging::LevelError: return "ERROR";
          case Logging::LevelWarning: return "WARNING";
          case Logging::LevelProgress: return "PROGRESS";
          case Logging::LevelDebug: return "DEBUG";
          case Logging::LevelNone: return "NONE";
          default: return "UNKNOWN";
        }
    }

    void print(const MString& message)
    {
        const foundation::uint64 memory_size = foundation::System::get_process_virtual_memory_size() / (1024 * 1024);

        MStreamUtils::stdOutStream()
            << "Mem: " << memory_size << " MB "
            << message
            << "\n";
    }
}

void Logging::setLogLevel(const LogLevel level)
{
    MGlobal::displayInfo("Setting logging level to " + get_level_name(level));
    log_level = level;
}

void Logging::info(const MString& message)
{
    if (log_level == Logging::LevelNone)
        return;

    if (log_level >= Logging::LevelInfo)
        print(message);
}

void Logging::warning(const MString& message)
{
    if (log_level == Logging::LevelNone)
        return;

    if (log_level >= Logging::LevelWarning)
        print(message);
}

void Logging::error(const MString& message)
{
    if (log_level == Logging::LevelNone)
        return;

    if (log_level >= Logging::LevelError)
        print(message);
}

void Logging::debug(const MString& message)
{
    if (log_level == Logging::LevelNone)
        return;

    if (log_level >= Logging::LevelDebug)
        print(message);
}

void Logging::progress(const MString& message)
{
    if (log_level == Logging::LevelNone)
        return;

    if (log_level >= Logging::LevelProgress)
        print(message);
}
