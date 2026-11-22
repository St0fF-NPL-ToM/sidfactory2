#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
    - miniz subdirectory:

	I rewrote the whole library CML to fit SIDFactoryII's needs.  On all occations this library is
	correctly included guarded by '#define MINIZ_HEADER_FILE_ONLY', keeping the include directory
	is OK. This just makes it very simple.
]]
cmake_minimum_required( VERSION 4.2 )

PROJECT( miniz )

add_library( miniz STATIC miniz.c )
target_include_directories( miniz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
