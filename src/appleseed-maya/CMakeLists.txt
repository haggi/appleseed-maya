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
    ../mayatocommon/mayaobject.cpp
    ../mayatocommon/mayaobject.h
    ../mayatocommon/mayaobjectfactory.h
    ../mayatocommon/mayascene.cpp
    ../mayatocommon/mayascene.h
    ../mayatocommon/mayascenefactory.cpp
    ../mayatocommon/mayascenefactory.h
    ../mayatocommon/mayarendernodes/rendercmd.cpp
    ../mayatocommon/mayarendernodes/renderglobalsnode.cpp
    ../mayatocommon/mayarendernodes/standinlocatornode.cpp
    ../mayatocommon/mayasceneparsing.cpp
    ../mayatocommon/mayasceneutils.cpp
    ../mayatocommon/memory/memoryinfo.cpp
    ../mayatocommon/renderglobals.cpp
    ../mayatocommon/renderglobalsfactory.cpp
    ../mayatocommon/rendering/mayarenderer.cpp
    ../mayatocommon/rendering/renderer.cpp
    ../mayatocommon/renderprocess.cpp
    ../mayatocommon/shadingtools/material.cpp
    ../mayatocommon/shadingtools/shaderdefs.cpp
    ../mayatocommon/shadingtools/shadingnode.cpp
    ../mayatocommon/shadingtools/shadingutils.cpp
    ../mayatocommon/swatchesrenderer/swatchrenderer.cpp
    ../mayatocommon/threads/compute.cpp
    ../mayatocommon/threads/queue.cpp
    ../mayatocommon/threads/renderqueueworker.cpp
    ../mayatocommon/threads/threads.cpp
    ../mayatocommon/utilities/attrtools.cpp
    ../mayatocommon/utilities/logging.cpp
    ../mayatocommon/utilities/meshtools.cpp
    ../mayatocommon/utilities/minimap.cpp
    ../mayatocommon/utilities/pystring.cpp
    ../mayatocommon/utilities/tools.cpp
    ../mayatocommon/world.cpp
    ../mayatocommon/osl/oslutils.cpp
    ${rendererplugin_sources}
)

target_link_libraries (appleseed-maya
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
