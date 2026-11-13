#[[
    CMakeLists Project file for RtMidi
    ----------------------------------
    This is a try to get one buildsystem working on all 3 major OS for SIDFactoryII.
    … Sometimes code just falls out … St0fF/NPL^ToM
]]
cmake_minimum_required( VERSION 4.2 )

# declare project
project( RtMidi LANGUAGES CXX )
 
add_library( RtMidi STATIC RtMidi.h RtMidi.cpp )
if ( WIN32 )
	target_compile_definitions( RtMidi PUBLIC __WINDOWS_UWP__ )
    set( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON )
    find_library( RTI NAMES RuntimeObject REQUIRED )
    target_link_libraries( RtMidi PRIVATE winmm ${RTI} )
elseif ( LINUX )
	target_compile_definitions( RtMidi PUBLIC __UNIX_JACK__ )
elseif ( APPLE )
	target_compile_definitions( RtMidi PUBLIC __MACOS_CORE__ )
endif()
target_include_directories( RtMidi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
