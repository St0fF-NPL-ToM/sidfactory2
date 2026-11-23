#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
        by St0fF/NPL^ToM 2026    → new runtime subdirectory
]]
cmake_minimum_required( VERSION 4.2 )

set( HEADERS
    environmentdefines.h

    emulation/asid/asid.h
    emulation/sid/sidproxy.h
    emulation/sid/sidproxydefines.h
    emulation/usbsid/usbsidpico.h

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
    emulation/usbsid/usbsidpico.cpp

    emulation/cpuframecapture.cpp
    emulation/cpumemory.cpp
    emulation/cpumos6510.cpp

    execution/executionhandler.cpp
    execution/flightrecorder.cpp
)
set( OTHERS )
target_sources( SIDFactoryII PRIVATE ${SOURCES} )
target_sources( SIDFactoryII PRIVATE FILE_SET headers TYPE HEADERS FILES ${HEADERS} )
target_sources( SIDFactoryII PRIVATE FILE_SET resources TYPE HEADERS FILES ${OTHERS} )
