################################################################################
#
# SIDFactoryII ... "runtime/editor" folder
#
################################################################################
set( SUBS
	auxilarydata
	components
	converters
	datacopy
	datasources
	debug
	dialog
	driver
	instrument
	keys
	optimize
	overlays
	packer
	screens
	undo
	utilities
	visualizer_components
)
### CMake-Helper for creating initial CMakeLists.txt files (the editor has many ...)
###
#foreach( SUB ${SUBS} )
#	set( CML "${CMAKE_CURRENT_SOURCE_DIR}/${SUB}/CMakeLists.txt" )
#	if ( NOT EXISTS ${CML} )
#		file( GLOB F RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/${SUB}" "${SUB}/*.h" "${SUB}/*.c" "${SUB}/*.cc" "${SUB}/*.hpp" "${SUB}/*.cpp" )
#		list( JOIN F "\n\t" FT )
#		message( CHECK_START "editor-subfolder: ${SUB}\nfiles:\t${FT}" )
#		file( CONFIGURE 
#			OUTPUT ${CML}
#			CONTENT "SF2_ADD_FILES( \${CMAKE_CURRENT_SOURCE_DIR}\n\t@FT@\n)"
#			@ONLY )
#	endif()
#endforeach()
SF2_ADD_SUBDIRS( ${SUBS} )
SF2_ADD_FILES( ${CMAKE_CURRENT_SOURCE_DIR}
	components_manager.h	components_manager.cpp
	cursor_control.h		cursor_control.cpp
	display_state.h			display_state.cpp
	edit_state.h			edit_state.cpp
	editor_facility.h		editor_facility.cpp
	editor_types.h
	overlay_control.h		overlay_control.cpp
)