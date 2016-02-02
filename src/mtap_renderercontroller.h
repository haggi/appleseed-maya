
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

#ifndef MTAP_RENDERER_CONTROLLER_H
#define MTAP_RENDERER_CONTROLLER_H

#include "renderer/api/rendering.h"

class mtap_IRendererController : public renderer::IRendererController
{
  public:
      mtap_IRendererController()
      {
          m_entityUpdateProc = 0;
          m_status =  renderer::IRendererController::ContinueRendering;
      }

    // Destructor.
    ~mtap_IRendererController() {}

    // This method is called before rendering begins.
    void on_rendering_begin();

    // This method is called after rendering has succeeded.
    void on_rendering_success();

    // This method is called after rendering was aborted.
    void on_rendering_abort();

    // This method is called before rendering a single frame.
    void on_frame_begin();

    // This method is called after rendering a single frame.
    void on_frame_end();

    // This method is called continuously during rendering.
    void on_progress();

    void release(){}

    Status get_status() const
    {
        return m_status;
    }

    void set_status(const Status new_status)
    {
        m_status = new_status;
    }

private:

    volatile Status m_status;
    void (*m_entityUpdateProc)();
};

#endif
