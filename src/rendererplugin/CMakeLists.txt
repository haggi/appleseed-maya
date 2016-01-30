
include_directories (
    ../mayatocommon/cpp
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

add_library (mayaToAppleseed SHARED
    ../mayatocommon/cpp/mayaobject.cpp
    ../mayatocommon/cpp/mayaobject.h
    ../mayatocommon/cpp/mayaobjectfactory.h
    ../mayatocommon/cpp/mayascene.cpp
    ../mayatocommon/cpp/mayascene.h
    ../mayatocommon/cpp/mayascenefactory.cpp
    ../mayatocommon/cpp/mayascenefactory.h
    ../mayatocommon/cpp/mayarendernodes/rendercmd.cpp
    ../mayatocommon/cpp/mayarendernodes/renderglobalsnode.cpp
    ../mayatocommon/cpp/mayarendernodes/standinlocatornode.cpp
    ../mayatocommon/cpp/mayasceneparsing.cpp
    ../mayatocommon/cpp/mayasceneutils.cpp
    ../mayatocommon/cpp/memory/memoryinfo.cpp
    ../mayatocommon/cpp/renderglobals.cpp
    ../mayatocommon/cpp/renderglobalsfactory.cpp
    ../mayatocommon/cpp/rendering/mayarenderer.cpp
    ../mayatocommon/cpp/rendering/renderer.cpp
    ../mayatocommon/cpp/renderprocess.cpp
    ../mayatocommon/cpp/shadingtools/material.cpp
    ../mayatocommon/cpp/shadingtools/shaderdefs.cpp
    ../mayatocommon/cpp/shadingtools/shadingnode.cpp
    ../mayatocommon/cpp/shadingtools/shadingutils.cpp
    ../mayatocommon/cpp/swatchesrenderer/swatchrenderer.cpp
    ../mayatocommon/cpp/threads/compute.cpp
    ../mayatocommon/cpp/threads/queue.cpp
    ../mayatocommon/cpp/threads/renderqueueworker.cpp
    ../mayatocommon/cpp/threads/threads.cpp
    ../mayatocommon/cpp/utilities/attrtools.cpp
    ../mayatocommon/cpp/utilities/logging.cpp
    ../mayatocommon/cpp/utilities/meshtools.cpp
    ../mayatocommon/cpp/utilities/minimap.cpp
    ../mayatocommon/cpp/utilities/pystring.cpp
    ../mayatocommon/cpp/utilities/tools.cpp
    ../mayatocommon/cpp/world.cpp
    ../mayatocommon/cpp/osl/oslutils.cpp
    ${rendererplugin_sources}
)

target_link_libraries (mayaToAppleseed
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

set_target_properties (mayaToAppleseed PROPERTIES PREFIX "")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set_target_properties (mayaToAppleseed PROPERTIES SUFFIX ".mll")
endif ()
