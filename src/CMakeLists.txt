
#
# This source file is part of appleseed.
# Visit http://appleseedhq.net/ for additional information and resources.
#
# This software is released under the MIT license.
#
# Copyright (c) 2016 Haggi Krey, The appleseedhq Organization
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

include_directories (
    .
)

set (shaders_sources
    shaders/asdisneymaterial.cpp
    shaders/asdisneymaterial.h
    shaders/asdisneymaterialoverride.cpp
    shaders/asdisneymaterialoverride.h
    shaders/aslayeredshader.cpp
    shaders/aslayeredshader.h
)
list (APPEND appleseedmaya_sources
    ${shaders_sources}
)
source_group ("shaders" FILES
    ${shaders_sources}
)

set (shadingtools_sources
    shadingtools/material.cpp
    shadingtools/material.h
    shadingtools/shaderdefinitions.cpp
    shadingtools/shaderdefinitions.h
    shadingtools/shadingnode.cpp
    shadingtools/shadingnode.h
    shadingtools/shadingutils.cpp
    shadingtools/shadingutils.h
)
list (APPEND appleseedmaya_sources
    ${shadingtools_sources}
)
source_group ("shadingtools" FILES
    ${shadingtools_sources}
)

set (utilities_sources
    utilities/attrtools.cpp
    utilities/attrtools.h
    utilities/concurrentqueue.h
    utilities/logging.cpp
    utilities/logging.h
    utilities/meshtools.cpp
    utilities/meshtools.h
    utilities/oslutils.cpp
    utilities/oslutils.h
    utilities/pystring.cpp
    utilities/pystring.h
    utilities/tools.cpp
    utilities/tools.h
)
list (APPEND appleseedmaya_sources
    ${utilities_sources}
)
source_group ("utilities" FILES
    ${utilities_sources}
)

set (sources
    appleseedmaya.cpp
    appleseedmaya.h
    appleseedrenderer.cpp
    appleseedrenderer.h
    appleseedswatchrenderer.cpp
    appleseedswatchrenderer.h
    appleseedutils.cpp
    appleseedutils.h
    binmeshreadercmd.cpp
    binmeshreadercmd.h
    binmeshtranslator.cpp
    binmeshtranslator.h
    binmeshwritercmd.cpp
    binmeshwritercmd.h
    event.h
    globalsnode.cpp
    globalsnode.h
    hypershaderenderer.cpp
    hypershaderenderer.h
    mayaobject.cpp
    mayaobject.h
    mayascene.cpp
    mayascene.h
    meshwalker.cpp
    meshwalker.h
    nodecallbacks.cpp
    nodecallbacks.h
    pluginmain.cpp
    proxymesh.cpp
    proxymesh.h
    renderercontroller.cpp
    renderercontroller.h
    renderglobals.cpp
    renderglobals.h
    renderqueue.cpp
    renderqueue.h
    swatchrenderer.cpp
    swatchrenderer.h
    tilecallback.cpp
    tilecallback.h
    version.cpp
    version.h
    world.cpp
    world.h
)
list (APPEND appleseedmaya_sources
    ${sources}
)
source_group ("" FILES
    ${sources}
)

add_library (appleseedmaya SHARED
    ${appleseedmaya_sources}
)

target_link_libraries (appleseedmaya
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMaya_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
    ${MAYA_OpenMayaFX_LIBRARY}
    ${MAYA_OpenMayaRender_LIBRARY}
    ${MAYA_OpenMayaUI_LIBRARY}
    ${APPLESEED_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
)

if (USE_EXTERNAL_EXR)
    target_link_libraries (appleseedmaya
        ${IMATH_LIBRARIES}
        ${OPENEXR_LIBRARIES}
    )
else ()
    target_link_libraries (appleseedmaya
        debug       ${APPLESEED_DEPS_STAGE_DIR}/ilmbase-debug/lib/Half.lib
        debug       ${APPLESEED_DEPS_STAGE_DIR}/ilmbase-debug/lib/Iex-2_2.lib
        debug       ${APPLESEED_DEPS_STAGE_DIR}/ilmbase-debug/lib/IlmThread-2_2.lib
        debug       ${APPLESEED_DEPS_STAGE_DIR}/ilmbase-debug/lib/Imath-2_2.lib
        debug       ${APPLESEED_DEPS_STAGE_DIR}/openexr-debug/lib/IlmImf-2_2.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/ilmbase-release/lib/Half.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/ilmbase-release/lib/Iex-2_2.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/ilmbase-release/lib/IlmThread-2_2.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/ilmbase-release/lib/Imath-2_2.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/openexr-release/lib/IlmImf-2_2.lib
    )
endif ()

if (USE_EXTERNAL_OIIO)
    target_link_libraries (appleseedmaya
        ${OPENIMAGEIO_LIBRARIES}
        ${LIBJPEG_LIBRARY}
        ${LIBTIFF_LIBRARY}
    )
else ()
    target_link_libraries (appleseedmaya
        debug       ${APPLESEED_DEPS_STAGE_DIR}/oiio-debug/lib/OpenImageIO.lib
        debug       ${APPLESEED_DEPS_STAGE_DIR}/libjpeg-turbo-debug/lib/jpeg-static.lib
        debug       ${APPLESEED_DEPS_STAGE_DIR}/libtiff-debug/lib/libtiff.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/oiio-release/lib/OpenImageIO.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/libjpeg-turbo-release/lib/jpeg-static.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/libtiff-release/lib/libtiff.lib
    )
endif ()

set_target_properties (appleseedmaya PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set_target_properties (appleseedmaya PROPERTIES SUFFIX ".mll")
endif ()
