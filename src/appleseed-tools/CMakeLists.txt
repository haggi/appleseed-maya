
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
    ../appleseed-maya
)

set (sources
    binmeshreadercmd.cpp
    binmeshreadercmd.h
    binmeshtranslator.cpp
    binmeshtranslator.h
    binmeshwritercmd.cpp
    binmeshwritercmd.h
    proxymesh.cpp
    proxymesh.h
    toolspluginmain.cpp
)
list (APPEND appleseedtools_sources
    ${sources}
)
source_group ("" FILES
    ${sources}
)

add_library (appleseed-tools SHARED
    ../appleseed-maya/appleseedmeshwalker.cpp
    ${appleseedtools_sources}
)

target_link_libraries (appleseed-tools
    mayatocommon
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMaya_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
    ${MAYA_OpenMayaFX_LIBRARY}
    ${MAYA_OpenMayaRender_LIBRARY}
    ${APPLESEED_LIBRARIES}
    ${Boost_LIBRARIES}
)

set_target_properties (appleseed-tools PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set_target_properties (appleseed-tools PROPERTIES SUFFIX ".mll")
endif ()
