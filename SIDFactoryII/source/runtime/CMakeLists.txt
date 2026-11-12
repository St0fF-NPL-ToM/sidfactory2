#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
    - runtime subdirectory
]]
cmake_minimum_required( VERSION 4.2 )

target_include_directories( SIDFactoryII PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    emulation
    emulation/asid
    emulation/sid
    execution
)
set( HEADERS
    environmentdefines.h

    emulation/asid/asid.h
    emulation/sid/sidproxy.h
    emulation/sid/sidproxydefines.h
    emulation/cpuframecapture.h
    emulation/cpumemory.h
    emulation/cpumos6510.h
    emulation/icpuwritecallback.h
    emulation/imemoryrandomreadaccess.h

    execution/executionhandler.h
    execution/flightrecorder.h
)
set( SOURCES

    emulation/asid/asid.cpp
    emulation/sid/sidproxy.cpp
    emulation/cpuframecapture.cpp
    emulation/cpumemory.cpp
    emulation/cpumos6510.cpp

    execution/executionhandler.cpp
    execution/flightrecorder.cpp
)
set( OTHERS )
target_sources( SIDFactoryII PUBLIC ${HEADERS} ${SOURCES} ${OTHERS} )
add_subdirectory( editor )
