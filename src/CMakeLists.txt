
######################################################
# plug-in

INCLUDE_DIRECTORIES(
    .
    mayatocommon/cpp
    appleseed
)

SET(SOURCES
    mayatocommon/cpp/mayaobject.cpp
    mayatocommon/cpp/mayaobject.h
    mayatocommon/cpp/mayaobjectfactory.h
    mayatocommon/cpp/mayascene.cpp
    mayatocommon/cpp/mayascene.h
    mayatocommon/cpp/mayascenefactory.cpp
    mayatocommon/cpp/mayascenefactory.h
    mayatocommon/cpp/mayarendernodes/rendercmd.cpp
    mayatocommon/cpp/mayarendernodes/renderglobalsnode.cpp
    mayatocommon/cpp/mayarendernodes/standinlocatornode.cpp
    mayatocommon/cpp/mayasceneparsing.cpp
    mayatocommon/cpp/mayasceneutils.cpp
    mayatocommon/cpp/memory/memoryinfo.cpp
    mayatocommon/cpp/renderglobals.cpp
    mayatocommon/cpp/renderglobalsfactory.cpp
    mayatocommon/cpp/rendering/mayarenderer.cpp
    mayatocommon/cpp/rendering/renderer.cpp
    mayatocommon/cpp/renderprocess.cpp
    mayatocommon/cpp/shadingtools/material.cpp
    mayatocommon/cpp/shadingtools/shaderdefs.cpp
    mayatocommon/cpp/shadingtools/shadingnode.cpp
    mayatocommon/cpp/shadingtools/shadingutils.cpp
    mayatocommon/cpp/swatchesrenderer/swatchrenderer.cpp
    mayatocommon/cpp/threads/compute.cpp
    mayatocommon/cpp/threads/queue.cpp
    mayatocommon/cpp/threads/renderqueueworker.cpp
    mayatocommon/cpp/threads/threads.cpp
    mayatocommon/cpp/utilities/attrtools.cpp
    mayatocommon/cpp/utilities/logging.cpp
    mayatocommon/cpp/utilities/meshtools.cpp
    mayatocommon/cpp/utilities/minimap.cpp
    mayatocommon/cpp/utilities/pystring.cpp
    mayatocommon/cpp/utilities/tools.cpp
    mayatocommon/cpp/world.cpp
    appleseed/appleseed.cpp
    appleseed/appleseedconfig.cpp
    appleseed/appleseedcreateoslshader.cpp
    appleseed/appleseedframeoutput.cpp
    appleseed/appleseedgeometry.cpp
    appleseed/appleseedmaterial.cpp
    appleseed/swatchesrenderer/appleseedswatchrenderer.cpp
    appleseed/swatchesrenderer/newswatchrenderer.cpp
    appleseed/swatchesrenderer/swatchesevent.cpp
    appleseed/testrender.cpp
    mayatoappleseed.cpp
    mtap_common/mtap_material.cpp
    mtap_common/mtap_mayaobject.cpp
    mtap_common/mtap_mayaobjectfactory.cpp
    mtap_common/mtap_mayarenderer.cpp
    mtap_common/mtap_renderglobals.cpp
    mtap_common/mtap_renderglobalsnode.cpp
    mtap_common/mtap_swatchrendererinterfacefactory.cpp
    mtap_common/mtap_world.cpp
    pluginmain.cpp
    shaders/asdisneymaterial.cpp
    shaders/asdisneymaterialoverride.cpp
    shaders/aslayeredshader.cpp
    appleseed/appleseedrendererfactory.cpp
    appleseed/mtap_renderercontroller.cpp
    appleseed/appleseedcamera.cpp
    appleseed/appleseedenvironment.cpp
    appleseed/appleseedgeo.cpp
    appleseed/appleseedlights.cpp
    appleseed/appleseedupdateelements.cpp
    appleseed/appleseedinteractiveupdate.cpp
    appleseed/mtap_tilecallback.cpp
    appleseed/appleseedutils.cpp
    mayatocommon/cpp/osl/oslutils.cpp
    mtap_common/mtap_swatchrenderer.cpp
)

ADD_LIBRARY(mayaToAppleseed SHARED ${SOURCES})

TARGET_LINK_LIBRARIES(mayaToAppleseed
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
    target_link_libraries (mayaToAppleseed
        ${IMATH_LIBRARIES}
        ${OPENEXR_LIBRARIES}
    )
else ()
    target_link_libraries (mayaToAppleseed
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
    target_link_libraries (mayaToAppleseed
        ${OPENIMAGEIO_LIBRARIES}
        ${LIBJPEG_LIBRARY}
        ${LIBTIFF_LIBRARY}
    )
else ()
    target_link_libraries (mayaToAppleseed
        debug       ${APPLESEED_DEPS_STAGE_DIR}/oiio-debug/lib/OpenImageIO.lib
        debug       ${APPLESEED_DEPS_STAGE_DIR}/libjpeg-turbo-debug/lib/jpeg-static.lib
        debug       ${APPLESEED_DEPS_STAGE_DIR}/libtiff-debug/lib/libtiff.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/oiio-release/lib/OpenImageIO.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/libjpeg-turbo-release/lib/jpeg-static.lib
        optimized   ${APPLESEED_DEPS_STAGE_DIR}/libtiff-release/lib/libtiff.lib
    )
endif ()

SET_TARGET_PROPERTIES(mayaToAppleseed PROPERTIES PREFIX "")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET_TARGET_PROPERTIES(mayaToAppleseed PROPERTIES SUFFIX ".mll")
ENDIF()
