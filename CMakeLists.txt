################################################################################
#
# SIDFactoryII - 1st try to create a CMakeLists to build on win_x64
#
################################################################################
cmake_minimum_required( VERSION 3.15...3.28 )

project( SF2 
	VERSION 2
	DESCRIPTION "SIDFactory2 Cross SID audio workstation"
	LANGUAGES CXX C
)
if( WIN32 )
	add_compile_definitions( _SF2_WINDOWS )
elseif( APPLE )
else()
	add_compile_definitions( _SF2_LINUX )
endif()

# find shipped SDL (whatever Laxity adds, I guess) <= didn't work well...
#file( GLOB_RECURSE SDL_CFG CONFIGURE_DEPENDS "libs/SDL2*cmake" )
#list( SUBLIST SDL_CFG 0 1 SDL_CFG )
#cmake_path( REMOVE_FILENAME SDL_CFG OUTPUT_VARIABLE SDL2_DIR )
find_package( SDL2 REQUIRED PATHS ${SDL2_DIR} )
include_directories( ${SDL2_INCLUDE_DIR} )

# SF2 local includes are relative to "source"
include_directories( "SIDFactoryII/source" )

# prepare target
add_executable( SF2 )

# internal Macro to add subfolders -> use this, first!
# After adding subdirs, you shall add local files.
macro( SF2_add_subdirs )
	if( ${ARGC} )
		foreach( SD ${ARGV} )
			add_subdirectory( ${SD} )
			set( SF2_HDR ${SF2_HDR} PARENT_SCOPE )
			set( SF2_C0DE ${SF2_C0DE} PARENT_SCOPE )
		endforeach()
	endif()
endmacro( SF2_add_subdirs )
# Macro that splits up a given long file list into sources and headers,
# then adds the respective files into our "global cmake vars" with the
# given folder prepended (1st arg), and to the executable.
macro( SF2_add_files dir )
	set( X ${ARGV} )
	list( REMOVE_ITEM X ${dir} )
	#	list( JOIN X "\n\t" Y )
	#	message( NOTICE "MACRO-add with arguments: dir=${dir}\nflist=\t${Y}" )
	set( HDR ${X} )
	list( FILTER HDR INCLUDE REGEX "[.]h(pp)?$" )
	list( TRANSFORM HDR PREPEND "${dir}/" )
	set( SF2_HDR ${SF2_HDR} ${HDR} PARENT_SCOPE )
	set( SRC ${X} )
	list( FILTER SRC INCLUDE REGEX "[.]c(c|pp)?$" )
	list( TRANSFORM SRC PREPEND "${dir}/" )
	set( SF2_C0DE ${SF2_C0DE} ${SRC} PARENT_SCOPE )
	target_sources( SF2 
		PRIVATE ${HDR}
		PRIVATE ${SRC} )
endmacro( SF2_add_files )
set( SF2_HDR ";" )
set( SF2_C0DE ";" )

# collect files
add_subdirectory( "SIDFactoryII" )

## debug the file lists:
#list( JOIN SF2_HDR "\n\t" H )
#message( NOTICE "SF2_Headers:\n\t${H}")
#list( JOIN SF2_C0DE "\n\t" C )
#message( NOTICE "SF2_C0DE:\t${C}" )
source_group( TREE "${CMAKE_CURRENT_SOURCE_DIR}/SIDFactoryII/source/" PREFIX "Headers" FILES ${SF2_HDR} )
source_group( TREE "${CMAKE_CURRENT_SOURCE_DIR}/SIDFactoryII/source/" PREFIX "Sources" FILES ${SF2_C0DE} )
source_group( "Sources" FILES "${CMAKE_CURRENT_SOURCE_DIR}/SIDFactoryII/main.cpp" )

if( WIN32 )
	set_target_properties( SF2 PROPERTIES VS_STARTUP_PROJECT ON VS_DEBUGGER_WORKING_DIRECTORY $(TargetDir) )
	add_custom_command( TARGET SF2 POST_BUILD
		COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/SF2_bundle.cmake" "--"
			${CMAKE_CURRENT_SOURCE_DIR} $(TargetDir) )
	target_link_libraries( SF2 SDL2::SDL2 SDL2::SDL2main )
else()
	target_link_libraries( SF2 SDL2::SDL2 )
endif()
