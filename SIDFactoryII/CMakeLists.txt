################################################################################
#
# SIDFactoryII ... actual project folder
#
################################################################################
cmake_minimum_required( VERSION 3.15...3.28 )

add_executable( SF2 "main.cpp" )

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
		PRIVATE "${HDR}"
		PRIVATE "${SRC}" )
endmacro( SF2_add_files )
set( SF2_HDR ";" )
set( SF2_C0DE ";" )

# SF2 local includes are relative to "source"
include_directories( "source" )

# collect files
add_subdirectory( "source" )

# debug the file lists:
#list( JOIN SF2_HDR "\n\t" H )
#message( NOTICE "SF2_Headers:\n\t${H}")
#list( JOIN SF2_C0DE "\n\t" C )
#message( NOTICE "SF2_C0DE:\t${C}" )
source_group( TREE "${CMAKE_CURRENT_SOURCE_DIR}/source/" PREFIX "Headers" FILES ${SF2_HDR} )
source_group( TREE "${CMAKE_CURRENT_SOURCE_DIR}/source/" PREFIX "Source" FILES ${SF2_C0DE} )
source_group( "Source" FILES main.cpp )
