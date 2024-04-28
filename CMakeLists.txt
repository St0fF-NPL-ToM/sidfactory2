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

# add the actual content ...
add_subdirectory( "SIDFactoryII" )
target_link_libraries( SF2 SDL2::SDL2 )
if( WIN32 )
	target_link_libraries( SF2 SDL2::SDL2main )
	set_target_properties( SF2 PROPERTIES
		VS_STARTUP_PROJECT ON
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/artifacts" )
endif()
