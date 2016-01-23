
######################################################
# plug-in

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/mayatocommon/cpp
                    ${PROJECT_SOURCE_DIR}/src/appleseed)

ADD_LIBRARY(appleseedTools SHARED
    #${PROJECT_SOURCE_DIR}/extern/TinyThread++-1.1/source/fast_mutex.h
    #${PROJECT_SOURCE_DIR}/extern/TinyThread++-1.1/source/tinythread.h
    #${PROJECT_SOURCE_DIR}/extern/TinyThread++-1.1/source/tinythread.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/memory/memoryinfo.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/shadingtools/shadingutils.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/utilities/attrtools.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/utilities/logging.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/utilities/pystring.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/utilities/tools.cpp
    ${PROJECT_SOURCE_DIR}/src/appleseedtools/binmeshtools/binmeshreadercmd.cpp
    ${PROJECT_SOURCE_DIR}/src/appleseedtools/binmeshtools/binmeshtranslator.cpp
    ${PROJECT_SOURCE_DIR}/src/appleseedtools/binmeshtools/binmeshwritercmd.cpp
    ${PROJECT_SOURCE_DIR}/src/appleseedtools/binmeshtools/proxymesh.cpp
    ${PROJECT_SOURCE_DIR}/src/appleseed/appleseedmeshwalker.cpp
    toolspluginmain.cpp)

TARGET_LINK_LIBRARIES(appleseedTools ${MAYA_Foundation_LIBRARY}
                                      ${MAYA_OpenMaya_LIBRARY}
                                      ${MAYA_OpenMayaAnim_LIBRARY}
                                      )

SET_TARGET_PROPERTIES(appleseedTools PROPERTIES PREFIX "")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET_TARGET_PROPERTIES(appleseedTools PROPERTIES SUFFIX ".mll")
ENDIF()
