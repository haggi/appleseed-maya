
include_directories (
    ../mayatocommon/cpp
    ../appleseed
    ..
)

add_library (appleseedTools SHARED
    ../mayatocommon/cpp/memory/memoryinfo.cpp
    ../mayatocommon/cpp/shadingtools/shadingutils.cpp
    ../mayatocommon/cpp/utilities/attrtools.cpp
    ../mayatocommon/cpp/utilities/logging.cpp
    ../mayatocommon/cpp/utilities/pystring.cpp
    ../mayatocommon/cpp/utilities/tools.cpp
    ../appleseed/appleseedmeshwalker.cpp
    binmeshtools/binmeshreadercmd.cpp
    binmeshtools/binmeshtranslator.cpp
    binmeshtools/binmeshwritercmd.cpp
    binmeshtools/proxymesh.cpp
    toolspluginmain.cpp
)

target_link_libraries (appleseedTools
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMaya_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
    ${MAYA_OpenMayaFX_LIBRARY}
    ${MAYA_OpenMayaRender_LIBRARY}
    ${APPLESEED_LIBRARIES}
    ${Boost_LIBRARIES}
)

set_target_properties (appleseedTools PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set_target_properties (appleseedTools PROPERTIES SUFFIX ".mll")
endif ()
