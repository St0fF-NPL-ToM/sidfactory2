#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
        by St0fF/NPL^ToM 2026    → ATMDS subdirectory
]]
cmake_minimum_required( VERSION 4.2 )
target_include_directories( SIDFactoryII PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
set( HEADERS
    driver_arch_atmds3.h
    ATMDS3_Constants.h
)
set( SOURCES
    driver_arch_atmds3.cpp
)
set( OTHERS )
target_sources( SIDFactoryII PRIVATE ${SOURCES} )
target_sources( SIDFactoryII PRIVATE FILE_SET headers TYPE HEADERS FILES ${HEADERS} )
target_sources( SIDFactoryII PRIVATE FILE_SET resources TYPE HEADERS FILES ${OTHERS} )
