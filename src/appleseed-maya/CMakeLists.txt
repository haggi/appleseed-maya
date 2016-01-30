
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
    ../mayatocommon
)

set (shaders_sources
    shaders/asdisneymaterial.cpp
    shaders/asdisneymaterial.h
    shaders/asdisneymaterialoverride.cpp
    shaders/asdisneymaterialoverride.h
    shaders/aslayeredshader.cpp
    shaders/aslayeredshader.h
)
list (APPEND rendererplugin_sources
    ${shaders_sources}
)
source_group ("shaders" FILES
    ${shaders_sources}
)

set (swatchesrenderer_sources
    swatchesrenderer/appleseedswatchrenderer.cpp
    swatchesrenderer/appleseedswatchrenderer.h
    swatchesrenderer/newswatchrenderer.cpp
    swatchesrenderer/swatchesevent.cpp
    swatchesrenderer/swatchesevent.h
)
list (APPEND rendererplugin_sources
    ${swatchesrenderer_sources}
)
source_group ("swatchesrenderer" FILES
    ${swatchesrenderer_sources}
)

set (sources
    appleseed.cpp
    appleseed.h
    appleseedcamera.cpp
    appleseedconfig.cpp
    appleseedcreateoslshader.cpp
    appleseedenvironment.cpp
    appleseedframeoutput.cpp
    appleseedgeo.cpp
    appleseedgeometry.cpp
    appleseedgeometry.h
    appleseedinteractiveupdate.cpp
    appleseedlights.cpp
    appleseedmaterial.cpp
    appleseedmaterial.h
    appleseedmeshwalker.cpp
    appleseedmeshwalker.h
    appleseedrendererfactory.cpp
    appleseedupdateelements.cpp
    appleseedutils.cpp
    appleseedutils.h
    mayatoappleseed.cpp
    mayatoappleseed.h
    mtap_material.cpp
    mtap_material.h
    mtap_mayaobject.cpp
    mtap_mayaobject.h
    mtap_mayaobjectfactory.cpp
    mtap_mayarenderer.cpp
    mtap_mayarenderer.h
    mtap_renderercontroller.cpp
    mtap_renderercontroller.h
    mtap_renderglobals.cpp
    mtap_renderglobalsnode.cpp
    mtap_renderglobalsnode.h
    mtap_shadingnode.cpp
    mtap_shadingnode.h
    mtap_standinlocator.cpp
    mtap_standinlocator.h
    mtap_standinmeshnode.cpp
    mtap_standinmeshnode.h
    mtap_swatchrenderer.cpp
    mtap_swatchrenderer.h
    mtap_swatchrendererinterfacefactory.cpp
    mtap_tilecallback.cpp
    mtap_tilecallback.h
    mtap_world.cpp
    pluginmain.cpp
    version.h
)
list (APPEND rendererplugin_sources
    ${sources}
)
source_group ("" FILES
    ${sources}
)

add_library (appleseed-maya SHARED
    ${rendererplugin_sources}
)

target_link_libraries (appleseed-maya
    mayatocommon
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
    target_link_libraries (appleseed-maya
        ${IMATH_LIBRARIES}
        ${OPENEXR_LIBRARIES}
    )
else ()
    target_link_libraries (appleseed-maya
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
    target_link_libraries (appleseed-maya
        ${OPENIMAGEIO_LIBRARIES}
        ${LIBJPEG_LIBRARY}
        ${LIBTIFF_LIBRARY}
    )
else ()
    target_link_libraries (appleseed-maya
        debug       ${APPLESEED_DEPS_STAGE_DIR}/oiio-debug/lib/OpenImageIO.lib
        debug       ${APPLESEED_DEPS_STAGE_DIR}/libjpeg-turbo-debug/lib/jpeg-static.lib
        debug       ${APPLESEED_DEPS_STAGE_DIR}/libtiff-debug/lib/libtiff.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/oiio-release/lib/OpenImageIO.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/libjpeg-turbo-release/lib/jpeg-static.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/libtiff-release/lib/libtiff.lib
    )
endif ()

set_target_properties (appleseed-maya PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set_target_properties (appleseed-maya PROPERTIES SUFFIX ".mll")
endif ()
