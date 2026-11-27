#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
        by St0fF/NPL^ToM 2026    → ATMDS subdirectory

    Oh, and by the way ... this part includes are Driver Assembly!
    Using the freshly created "KickAssembler.cmake", see resources folder.
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
set( DRV AcidPlayer-3.2.26.asm )
set( DRV_HDRS
    ATMDS3_data.asm
    ATMDS3_datahelfer.asm
    ATPLR-3.2-SF2.asm
    SF2_driver.asm
)
set( OTHERS ${DRV} ${DVR_HDRS} )
target_sources( SIDFactoryII PRIVATE ${SOURCES} )
target_sources( SIDFactoryII PRIVATE FILE_SET headers TYPE HEADERS FILES ${HEADERS} )
target_sources( SIDFactoryII PRIVATE FILE_SET resources TYPE HEADERS FILES ${OTHERS} )

if ( Java_JAVA_EXECUTABLE AND KICKASSEMBLER_JAR )
# Additional targets here: the Driver itself!
    add_kick_target( ATMDS3_Driver MAIN_SOURCE ${DRV} OUTPUT_NAME "AcidPlayer-3.2.26.prg"
        INCLUDES ${DRV_HDRS} ATMDS3_Constants.h
        DEST_DIR "${CMAKE_SOURCE_DIR}/resources/drivers"
        DEPENDENT_TARGETS SIDFactoryII
    )
else()
    Msg( WARN "\nATMDS-3.2-Driver is not being built!\n\n" )
endif()
