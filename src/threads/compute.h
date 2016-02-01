
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

/*
This module is a replacement for mayas MComputation. The problem with MComputation is it bocks the UI.
I hope this one works better, or at least as expected.
*/

#ifndef COMPUTE_H
#define COMPUTE_H

#ifdef _WIN32
#include <windows.h>
#endif
#include <iostream>
#include <maya/MComputation.h>
#include <maya/MRenderView.h>
#include <maya/MGlobal.h>

#include "foundation/platform/thread.h"

#include "boost/chrono.hpp"

static MString setWaitCursorCmd = "import pymel.core as pm;pm.waitCursor(state=True);";
static MString releaseWaitCursorCmd = "import pymel.core as pm;pm.waitCursor(state=False);pm.refresh()";

class Compute
{
  private:
    static bool escPressed;
    static bool autoexit;
    static bool checkDone;
    static boost::thread checkThread;
    static bool usewaitcursor;
#ifdef _WIN32
    static HWND windowHandle;
#endif
    static void checkInterrupt()
    {
        bool done = false;
        while (!done && !Compute::checkDone)
        {
#ifdef _WIN32
            if (GetAsyncKeyState(VK_ESCAPE))
            {
                Compute::escPressed = true;
                if (Compute::autoexit)
                    done = true;
            }
#endif
            if (!done && !Compute::checkDone)
                foundation::sleep(100);
        }
    }

  public:
    Compute()
    {
        Compute::escPressed = false;
        Compute::autoexit = false;
        Compute::checkDone = false;
        Compute::usewaitcursor = false;
    }

    void beginComputation(bool autoExit = true, bool useWaitCursor = true)
    {
        Compute::escPressed = false;
        Compute::checkDone = false;
        Compute::usewaitcursor = useWaitCursor;
        Compute::autoexit = autoExit;
        Compute::checkThread = boost::thread(checkInterrupt);

#ifdef _WIN32
        Compute::windowHandle = GetForegroundWindow();
        int count = 0;
        while (GetAsyncKeyState(VK_ESCAPE) && (count++ < 10))
        {
            GetAsyncKeyState(VK_ESCAPE);
        }
#endif
        if (useWaitCursor)
        {
            if (MRenderView::doesRenderEditorExist())
                MGlobal::executePythonCommand(setWaitCursorCmd);
        }
    }

    void endComputation()
    {
        Compute::checkDone = true;
        if (Compute::checkThread.joinable())
        {
            Compute::checkThread.join();
            if (Compute::usewaitcursor && MRenderView::doesRenderEditorExist())
                MGlobal::executePythonCommand(releaseWaitCursorCmd);
        }
        Compute::escPressed = false;
    }

    bool isInterruptRequested()
    {
        return Compute::escPressed;
    }
};

bool Compute::escPressed = false;
bool Compute::autoexit = false;
bool Compute::checkDone = false;
bool Compute::usewaitcursor = false;
boost::thread Compute::checkThread;

#ifdef _WIN32
HWND Compute::windowHandle = 0;
#endif

#endif
