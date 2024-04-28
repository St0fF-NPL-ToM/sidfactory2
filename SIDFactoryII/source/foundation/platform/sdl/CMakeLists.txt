################################################################################
#
# SIDFactoryII ... "foundation/platform/sdl" folder
#
################################################################################
if( WIN32 )
	set( F windows )
elseif( APPLE )
	set( F macos )
else()
	set( F linux )
endif()
SF2_ADD_FILES( ${CMAKE_CURRENT_SOURCE_DIR}
	mutex_sdl.h mutex_sdl.cpp
	platform_sdl.h platform_sdl.cpp
	platform_sdl_${F}.h platform_sdl_${F}.cpp
)