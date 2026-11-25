#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
        by St0fF/NPL^ToM 2026    → new editor/interfaces subdirectory
]]
cmake_minimum_required( VERSION 4.2 )

set( HEADERS
    idriver_architecture.h
    ievent_definition.h
    imemory_streaming.h
    ioptimizer.h
)
set( SOURCES
)
set( OTHERS )
target_sources( SIDFactoryII PRIVATE ${SOURCES} )
target_sources( SIDFactoryII PRIVATE FILE_SET headers TYPE HEADERS FILES ${HEADERS} )
target_sources( SIDFactoryII PRIVATE FILE_SET resources TYPE HEADERS FILES ${OTHERS} )
