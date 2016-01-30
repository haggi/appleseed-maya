include_directories (
    ../mayatocommon
    ../rendererplugin
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

add_library (appleseedtools SHARED
    ../rendererplugin/appleseedmeshwalker.cpp
    ${appleseedtools_sources}
)

target_link_libraries (appleseedtools
    mayatocommon
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMaya_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
    ${MAYA_OpenMayaFX_LIBRARY}
    ${MAYA_OpenMayaRender_LIBRARY}
    ${APPLESEED_LIBRARIES}
    ${Boost_LIBRARIES}
)

set_target_properties (appleseedtools PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set_target_properties (appleseedtools PROPERTIES SUFFIX ".mll")
endif ()
