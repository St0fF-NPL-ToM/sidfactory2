#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
]]
cmake_minimum_required( VERSION 4.2 )

# include better.cmake
if ( NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/better.cmake )
    file( DOWNLOAD https://gitlab.com/copr-projekte/better-cmake/-/releases/0.2.2/downloads/better.cmake better.cmake STATUS BCM )
    list( POP_FRONT BCM BCM_EC )
    if ( BCM_EC ) # error-code non-zero
        message( FATAL_ERROR "Better CMake could not be downloaded:\n\tCode: ${BCM_EC}\t${BCM}" )
    endif()
endif()
include( "${CMAKE_CURRENT_BINARY_DIR}/better.cmake" )

# Before we start: make a fresh CMake-Cache-installation point to the artifacts folder
set( CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/artifacts" )

# Let's fetz!
Msg( START "Configuring SIDFactoryII on '${CMAKE_HOST_SYSTEM_NAME}'" )

# query build ID, make a version number out of it.
init_Git()
cmd( git show --no-patch --format=%cs OUT_VAR _bd )
cmd( git rev-parse --short HEAD OUT_VAR _id )
calc( _bi "0x${_id}" )                         # CMake version numbering only allows base10-
string( REPLACE "-" "." _bi "${_bd}.${_bi}" )  # integers and colons.  No dashes allowed!

# declare project
project( sidfactory2 VERSION ${_bi} LANGUAGES C CXX )
set( CMAKE_CXX_STANDARD 14 )                  # some libraries by now require more
set( CMAKE_CXX_STANDARD_REQUIRED absolutely ) # LOL coding humor. A string evaluates true.
Msg( "current Version: ${_bd}.${_id} → CMake translation: ${PROJECT_VERSION}" )

# temporary option
option( SF2_MAKEFILE_APPROACH "Use the same approach for building with CMake like the original SF2 Makefile." on )

#[[ Preparation for installation differences:
    -----------------------------------------
    - platform-options / preprocessor definitions
	- library gathering
	- installation directories
]]
set( TPROP "" ) # differs per OS … CMAKE-property sent to add_executable for OS-special bundling
if ( LINUX )
    # Linux only configuration options
    option( SF2_USE_ALSA "use the alsasound midi backend" off )
    option( SF2_USE_JACK "use the jack audio connection kit for midi" on )
    if ( SF2_USE_ALSA EQUAL SF2_USE_JACK )
        Msg( FATAL "Please decide on one midi-interface to use! (CMake cache variables: 'SF2_USE_ALSA', 'SF2_USE_JACK')" )
    elseif( SF2_USE_ALSA )
        add_compile_definitions( __LINUX_ALSA__ )
    else()
        add_compile_definitions( __UNIX_JACK__ )
    endif()
    add_link_options( -flto -lpthread )
    # All major Distros provide PackageConfig. And they provide development packages of
    # SDL2 and Jack, as well as libusb, etc. - so please make sure those are installed.
    find_package( SDL2 QUIET )
    if ( NOT SDL2_FOUND )
        cmake_pkg_config( IMPORT sdl2 )
        cmake_pkg_config( IMPORT sdl2-compat )  # fallback
    endif()
    cmake_pkg_config( IMPORT jack )
    cmake_pkg_config( IMPORT libusb-1.0 )
    get_directory_property( SF2_LIBS IMPORTED_TARGETS )
    list( FILTER SF2_LIBS EXCLUDE REGEX "Git.*" ) # Git was also imported, but not for linking!
    # libexec needs a subdirectory on linux (as we also install folders)
	include( GNUInstallDirs )
    string( APPEND CMAKE_INSTALL_LIBEXECDIR /sidfactory2 )
    set( CACHE{SF2_EXEC_DIR} FORCE VALUE "${CMAKE_INSTALL_LIBEXECDIR}" )
    set( CACHE{SF2_DOC_DIR} FORCE VALUE "${CMAKE_INSTALL_DOCDIR}" )
elseif( APPLE )
    set( TPROP MACOSX_BUNDLE )
    Msg( FATAL "APPLE build chain has not yet been ported to CMake. Sorry!" )
elseif( WIN32 )
    set( TPROP WIN32 )
    include( FetchContent ) # SDL2 on Windows - puh - let's just grab it from the network …
    FetchContent_Declare( SDL2 GIT_REPOSITORY https://github.com/libsdl-org/SDL GIT_TAG SDL2 GIT_SHALLOW True )
    FetchContent_MakeAvailable( SDL2 )
    # see extra CMakeLists.txt in that subdirectory!
    set( SF2_LIBS SDL2::SDL2 SDL2::SDL2main libusb )
    set( CACHE{SF2_EXEC_DIR} FORCE VALUE "." )
    set( CACHE{SF2_DOC_DIR} FORCE VALUE "." )
    add_subdirectory( libs/libusb )
    include_directories( libs/usbsid-windows )
else()
    Msg( FATAL "Seems like your operating system is not yet supported. Sorry!\nYou may as well contribute by extending this CMake-buildchain." )
endif()

# prepare DEFINES: build-number and platform
set( BN "\"${_bd}.${_id}\"" )
string( TOUPPER "${CMAKE_HOST_SYSTEM_NAME}" PLATFORM )
add_compile_definitions( _SF2_${PLATFORM} _BUILD_NR=${BN} )

# Create the executable inside the project base folder
add_subdirectory( SIDFactoryII )

# Installation tasks from here:
install( DIRECTORY dist/documentation    DESTINATION "${SF2_DOC_DIR}" )
install( FILES README.md                 DESTINATION "${SF2_DOC_DIR}" )
if ( LINUX )
    # Linux needs the icon and desktop entries in special XDG-DIRS
    install( FILES macos/icon.svg        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps/${PROJECT_NAME}.svg )
    install( FILES dist/com.github.sidfactory2.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications )
    install( FILES dist/com.github.sidfactory2.appstream.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo/
        RENAME com.github.sidfactory2.metainfo.xml )
elseif( APPLE )
elseif( WIN32 )
endif()

Msg( "→ install destinations:" )
Msg( "BIN:     ${CMAKE_INSTALL_BINDIR}" )
Msg( "LIBEXEC: ${CMAKE_INSTALL_LIBEXECDIR}" )
Msg( "DATA:    ${CMAKE_INSTALL_DATAROOTDIR}" )
Msg( "DOC:     ${CMAKE_INSTALL_DOCDIR}" )
Msg( DONE )
