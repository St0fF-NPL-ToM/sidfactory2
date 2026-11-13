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

# Before we start: make a fresh CMake-Cache-installation point to the artifacts folder …
set( CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/artifacts" )
string( TOUPPER "${CMAKE_HOST_SYSTEM_NAME}" PLATFORM )
Msg( START "Configuring SIDFactoryII on '${PLATFORM}'" )

# query build ID, make a version number out of it.
init_Git()
cmd( git show --no-patch --format=%cs OUT_VAR _bd )
cmd( git rev-parse --short HEAD OUT_VAR _id )
calc( _bi "0x${_id}" )                  # And only decimal numbers.
string( REPLACE "-" "." _bi "${_bd}.${_bi}" )  # Version numbering does not allow dashes

# declare project
project( sidfactory2 VERSION ${_bi} LANGUAGES C CXX )
set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQUIRED absolutely ) # LOL coding humor. A string evaluates true.
Msg( "current Version: ${_bd}.${_id} → CMake translation: ${PROJECT_VERSION}" )
include( GNUInstallDirs )

# import libraries
find_package( SDL2 REQUIRED )   # same on all OS'es
set( SF2_LIBS SDL2::SDL2 )

if ( LINUX )
    cmake_pkg_config( IMPORT jack )
    list( APPEND SF2_LIBS PkgConfig::jack )
    set( TPROP "" ) # TPROP is the target-property given to add_executable()
    # prepare for install commands
    string( APPEND CMAKE_INSTALL_LIBEXECDIR /sidfactory2 )
elseif( APPLE )
    Msg( FATAL "APPLE build chain has not yet been ported to CMake. Sorry!" )
    add_compile_definitions( __MACOS_CORE__ )
    set( TPROP MACOSX_BUNDLE )
elseif( WIN32 )
    Msg( FATAL "WINDOWS build chain has not yet been ported to CMake. Sorry!" )
    set( TPROP WIN32 )
else()
    Msg( FATAL "Seems like your operating system is not yet supported. Sorry!\nYou may as well contribute by extending this CMake-buildchain." )
endif()
set( BN "\"${_bd}.${_id}\"" )
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
