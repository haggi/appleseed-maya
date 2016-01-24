
######################################################
# plug-in

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/mayatocommon/cpp
${PROJECT_SOURCE_DIR}/src/appleseed
)

ADD_LIBRARY(mayaToAppleseed SHARED
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayaobject.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayaobject.h
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayaobjectfactory.h
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayascene.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayascene.h
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayascenefactory.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayascenefactory.h

    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayarendernodes/rendercmd.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayarendernodes/renderglobalsnode.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayarendernodes/standinlocatornode.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayasceneparsing.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/mayasceneutils.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/memory/memoryinfo.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/osl/oslutils.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/renderglobals.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/renderglobalsfactory.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/rendering/mayarenderer.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/rendering/renderer.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/renderprocess.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/shadingtools/material.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/shadingtools/shaderdefs.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/shadingtools/shadingnode.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/shadingtools/shadingutils.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/swatchesrenderer/swatchrenderer.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/threads/compute.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/threads/queue.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/threads/renderqueueworker.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/threads/threads.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/utilities/attrtools.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/utilities/logging.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/utilities/meshtools.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/utilities/minimap.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/utilities/pystring.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/utilities/tools.cpp
    ${PROJECT_SOURCE_DIR}/mayatocommon/cpp/world.cpp
    appleseed/appleseed.cpp
    appleseed/appleseedcamera.cpp
    appleseed/appleseedconfig.cpp
    appleseed/appleseedcreateoslshader.cpp
    appleseed/appleseedenvironment.cpp
    appleseed/appleseedframeoutput.cpp
    appleseed/appleseedgeo.cpp
    appleseed/appleseedgeometry.cpp
    appleseed/appleseedinteractiveupdate.cpp
    appleseed/appleseedlights.cpp
    appleseed/appleseedmaterial.cpp
    appleseed/appleseedmeshwalker.cpp
    appleseed/appleseedrendererfactory.cpp
    appleseed/appleseedshaders.cpp
    appleseed/appleseedshadingnodes.cpp
    appleseed/appleseedupdateelements.cpp
    appleseed/appleseedutils.cpp
    appleseed/mtap_renderercontroller.cpp
    appleseed/mtap_tilecallback.cpp
    appleseed/swatchesrenderer/appleseedswatchrenderer.cpp
    appleseed/swatchesrenderer/newswatchrenderer.cpp
    appleseed/swatchesrenderer/swatchesevent.cpp
    appleseed/testrender.cpp
    mayatoappleseed.cpp
    mtap_common/mtap_material.cpp
    mtap_common/mtap_mayaobject.cpp
    mtap_common/mtap_mayaobjectfactory.cpp
    mtap_common/mtap_mayarenderer.cpp
    mtap_common/mtap_mayascene.cpp
    mtap_common/mtap_mayascenefactory.cpp
    mtap_common/mtap_renderglobals.cpp
    mtap_common/mtap_renderglobalsnode.cpp
    mtap_common/mtap_standinlocator.cpp
    mtap_common/mtap_standinmeshnode.cpp
    mtap_common/mtap_swatchrenderer.cpp
    mtap_common/mtap_swatchrendererinterfacefactory.cpp
    mtap_common/mtap_world.cpp
    pluginmain.cpp
    shaders/asdisneymaterial.cpp
    shaders/asdisneymaterialoverride.cpp
    shaders/aslayeredshader.cpp
)

TARGET_LINK_LIBRARIES(mayaToAppleseed ${MAYA_Foundation_LIBRARY}
                                      ${MAYA_OpenMaya_LIBRARY}
                                      ${MAYA_OpenMayaAnim_LIBRARY}
                                      ${APPLESEED_LIBRARIES}
                                      ${Boost_LIBRARIES})

SET_TARGET_PROPERTIES(mayaToAppleseed PROPERTIES PREFIX "")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET_TARGET_PROPERTIES(mayaToAppleseed PROPERTIES SUFFIX ".mll")
ENDIF()
