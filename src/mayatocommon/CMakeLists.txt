include_directories (
    .
)

set (mayarendernodes_sources
    mayarendernodes/rendercmd.cpp
    mayarendernodes/rendercmd.h
    mayarendernodes/renderglobalsnode.cpp
    mayarendernodes/renderglobalsnode.h
    mayarendernodes/standinlocatornode.cpp
    mayarendernodes/standinlocatornode.h
    mayarendernodes/standinmeshnode.cpp
    mayarendernodes/standinmeshnode.h
)
list (APPEND mayatocommon_sources
    ${mayarendernodes_sources}
)
source_group ("mayarendernodes" FILES
    ${mayarendernodes_sources}
)

set (memory_sources
    memory/memoryinfo.cpp
    memory/memoryinfo.h
)
list (APPEND mayatocommon_sources
    ${memory_sources}
)
source_group ("memory" FILES
    ${memory_sources}
)

set (osl_sources
    osl/oslutils.cpp
    osl/oslutils.h
)
list (APPEND mayatocommon_sources
    ${osl_sources}
)
source_group ("osl" FILES
    ${osl_sources}
)

set (rendering_sources
    rendering/mayarenderer.cpp
    rendering/mayarenderer.h
    rendering/renderer.cpp
    rendering/renderer.h
    rendering/rendererfactory.h
)
list (APPEND mayatocommon_sources
    ${rendering_sources}
)
source_group ("rendering" FILES
    ${rendering_sources}
)

set (shadingtools_sources
    shadingtools/material.cpp
    shadingtools/material.h
    shadingtools/shaderdefs.cpp
    shadingtools/shaderdefs.h
    shadingtools/shadingnode.cpp
    shadingtools/shadingnode.h
    shadingtools/shadingutils.cpp
    shadingtools/shadingutils.h
)
list (APPEND mayatocommon_sources
    ${shadingtools_sources}
)
source_group ("shadingtools" FILES
    ${shadingtools_sources}
)

set (swatchesrenderer_sources
    swatchesrenderer/newswatchrenderer.h
    swatchesrenderer/swatchesqueue.h
    swatchesrenderer/swatchgeometry.h
    swatchesrenderer/swatchrenderer.cpp
    swatchesrenderer/swatchrenderer.h
    swatchesrenderer/swatchrendererinterface.h
    swatchesrenderer/swatchrendererinterfacefactory.h
)
list (APPEND mayatocommon_sources
    ${swatchesrenderer_sources}
)
source_group ("swatchesrenderer" FILES
    ${swatchesrenderer_sources}
)

set (threads_sources
    threads/compute.cpp
    threads/compute.h
    threads/queue.cpp
    threads/queue.h
    threads/renderqueueworker.cpp
    threads/renderqueueworker.h
    threads/threads.cpp
    threads/threads.h
)
list (APPEND mayatocommon_sources
    ${threads_sources}
)
source_group ("threads" FILES
    ${threads_sources}
)

set (translators_sources
    translators/meshtranslator.cpp
    translators/meshtranslator.h
)
list (APPEND mayatocommon_sources
    ${translators_sources}
)
source_group ("translators" FILES
    ${translators_sources}
)

set (utilities_sources
    utilities/attrtools.cpp
    utilities/attrtools.h
    utilities/logging.cpp
    utilities/logging.h
    utilities/meshtools.cpp
    utilities/meshtools.h
    utilities/minimap.cpp
    utilities/minimap.h
    utilities/pystring.cpp
    utilities/pystring.h
    utilities/tools.cpp
    utilities/tools.h
)
list (APPEND mayatocommon_sources
    ${utilities_sources}
)
source_group ("utilities" FILES
    ${utilities_sources}
)

set (sources
    definitions.h
    mayaobject.cpp
    mayaobject.h
    mayaobjectfactory.h
    mayascene.cpp
    mayascene.h
    mayascenefactory.cpp
    mayascenefactory.h
    mayasceneparsing.cpp
    mayasceneutils.cpp
    renderglobals.cpp
    renderglobals.h
    renderglobalsfactory.cpp
    renderglobalsfactory.h
    renderprocess.cpp
    renderprocess.h
    world.cpp
    world.h
)
list (APPEND mayatocommon_sources
    ${sources}
)
source_group ("" FILES
    ${sources}
)

add_library (mayatocommon STATIC ${mayatocommon_sources})
