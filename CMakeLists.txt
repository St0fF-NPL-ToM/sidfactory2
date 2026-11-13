#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
]]
cmake_minimum_required( VERSION 4.2 )

# include better.cmake
if ( NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/better.cmake )
    file( DOWNLOAD https://gitlab.com/copr-projekte/better-cmake/-/releases/0.2.2/downloads/better.cmake better.cmake STATUS BCM SHOW_PROGRESS )
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
set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQUIRED absolutely ) # LOL coding humor. A string evaluates true.
Msg( "current Version: ${_bd}.${_id} → CMake translation: ${PROJECT_VERSION}" )

#[[ Preparation for installation differences:
    -----------------------------------------
	- library gathering
	- preprocessor definitions
	- installation directories
]]
if ( LINUX )
    set( TPROP "" ) # differs per OS …
    # All major Distros provide PackageConfig. And they provide development packages of
    # SDL2 and Jack. Please make sure the dev-packages of your distribution is installed:
    # RedHat (Fedora, Ultramarine, CentOS)              | Debi(li)an (Ubuntu, …)
    #   sdl2-compat-devel,                              |   libsdl2-dev
    #   (jack-audio-connection-kit-devel)               |   libjack-dev
    #       ||                                          |
    #   (pulseaudio-jack-audio-connection-kit-devel)    |
    cmake_pkg_config( IMPORT SDL2 )
    cmake_pkg_config( IMPORT jack )
    set( SF2_LIBS PkgConfig::SDL2 PkgConfig::SDL2Main PkgConfig::jack )
    # libexec needs a subdirectory on linux (as we also install folders)
    set( CMAKE_INSTALL_LIBEXECDIR /libexec/sidfactory2 )
	include( GNUInstallDirs )
	add_compile_definitions( __UNIX_JACK__ ) # RtMidi needs that
elseif( APPLE )
    set( TPROP MACOSX_BUNDLE )
    add_compile_definitions( __MACOS_CORE__ )
    Msg( FATAL "APPLE build chain has not yet been ported to CMake. Sorry!" )
elseif( WIN32 )
    set( TPROP WIN32 )
    include( FetchContent ) # SDL2 on Windows - puh - let's just grab it from the network …
    FetchContent_Declare( SDL2 GIT_REPOSITORY https://github.com/libsdl-org/SDL GIT_TAG SDL2 GIT_SHALLOW True )
    FetchContent_MakeAvailable( SDL2 )
    set( SF2_LIBS SDL2::SDL2 SDL2::SDL2main )
    set( CMAKE_INSTALL_BINDIR "." )  # on Windows: everything goes into $INSTALLROOT
    set( CMAKE_INSTALL_LIBEXECDIR "." )
    set( CMAKE_INSTALL_DATAROOTDIR "." )
    set( CMAKE_INSTALL_DOCDIR "." )
	add_compile_definitions( __WINDOWS_UWP__ ) # RtMidi needs that
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
install( DIRECTORY dist/documentation    TYPE DOC )
install( FILES README.md                 TYPE DOC )
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
