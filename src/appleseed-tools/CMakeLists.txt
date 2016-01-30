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
