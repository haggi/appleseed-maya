
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

#ifndef MTAP_NEWSWATCHRENDER_H
#define MTAP_NEWSWATCHRENDER_H

#include "renderer/api/scene.h"
#include "renderer/api/project.h"
#include "renderer/global/globallogger.h"
#include "renderer/api/rendering.h"
#include "definitions.h"

#include <maya/MObject.h>

namespace asf = foundation;
namespace asr = renderer;

class NewSwatchRenderer;

class AppleseedSwatchRenderer
{
  public:
    AppleseedSwatchRenderer();
    ~AppleseedSwatchRenderer();
    void mainLoop();
    void setSize(int size);
    void setShader(MObject shader);
    void renderSwatch();
    void renderSwatch(NewSwatchRenderer *sr);
    void fillSwatch(float *pixels);
    bool terminateLoop = false;
    bool enableSwatchRenderer = true;
    bool loopDone = false;

    asf::auto_release_ptr<asr::Scene> scene;
    asf::auto_release_ptr<asr::Project> project;
    autoPtr<asf::ILogTarget> log_target;
    autoPtr<asr::MasterRenderer> mrenderer;
    asr::DefaultRendererController renderer_controller;

    static void startAppleseedSwatchRender(AppleseedSwatchRenderer *swRend);
    static void terminateAppleseedSwatchRender(AppleseedSwatchRenderer *swRend);

    void defineMaterial(MObject shadingNode);

};


#endif
