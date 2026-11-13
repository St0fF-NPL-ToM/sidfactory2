#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
]]
cmake_minimum_required( VERSION 4.2 )

# include better.cmake
if ( NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/better.cmake )
    file( DOWNLOAD https://gitlab.com/copr-projekte/better-cmake/-/releases/0.2.2/downloads/better.cmake STATUS BCM SHOW_PROGRESS )
    list( POP_FRONT BCM BCM_EC )
    if ( BCM_EC ) # error-code non-zero
        message( FATAL_ERROR "Better CMake could not be downloaded:\n\tCode: ${BCM_EC}\t${BCM}" )
    else()
        file( RENAME ${CMAKE_CURRENT_BINARY_DIR}/better.cmake ${CMAKE_CURRENT_SOURCE_DIR}/tools/better.cmake )
    endif()
endif()
include( "${CMAKE_CURRENT_SOURCE_DIR}/tools/better.cmake" )

string( TOUPPER "${CMAKE_HOST_SYSTEM_NAME}" PLATFORM )
Msg( START "Configuring SIDFactoryII on '${PLATFORM}'" )
set( CACHE{CMAKE_INSTALL_PREFIX} FORCE VALUE "${CMAKE_CURRENT_SOURCE_DIR}/artifacts" )

# query build ID, make a version number out of it.
init_Git()
cmd( git show --no-patch --format=%cs OUT_VAR _bd )
cmd( git rev-parse --short HEAD OUT_VAR _id )
calc( _bi "0x${_id}" )                  # And only decimal numbers.
string( REPLACE "-" "." _bi "${_bd}.${_bi}" )  # Version numbering does not allow dashes

# declare project
project( SIDFactoryII VERSION ${_bi} LANGUAGES C CXX )
set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQUIRED absolutely ) # LOL coding humor. A string evaluates true.
Msg( "current Version: ${_bd}.${_id} → CMake translation: ${PROJECT_VERSION}" )

# import libraries
find_package( SDL2 REQUIRED )   # same on all OS'es
set( SF2_LIBS SDL2::SDL2 )

if ( LINUX )
    cmake_pkg_config( IMPORT jack )
    list( APPEND SF2_LIBS PkgConfig::jack )
    set( TPROP "" )
elseif( APPLE )
    Msg( FATAL "APPLE build chain has not yet been ported to CMake. Sorry!" )
    add_compile_definitions( __MACOS_CORE__ )
    set( TPROP MACOSX_BUNDLE )
elseif( WIN32 )
    Msg( FATAL "WINDOWS build chain has not yet been ported to CMake. Sorry!" )
    set( TPROP WIN32 )
else()
    Msg( FATAL "Seems like your operating system was never tried. Sorry!" )
endif()
set( BN "\"${_bd}.${_id}\"" )
add_compile_definitions( _SF2_${PLATFORM} _BUILD_NR=${BN} )

# prepare for install commands
include( GNUInstallDirs )

# Create the executable inside the project base folder
add_subdirectory( SIDFactoryII )

# Installation tasks from here:
install( DIRECTORY dist/documentation    TYPE DOC )
install( FILES README.md                 TYPE DOC )
if ( LINUX )
    # Linux needs the icon and desktop entries in special XDG-DIRS
    install( FILES macos/icon.svg        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps/${PROJECT_NAME}.svg )
    install( FILES dist/com.github.sidfactory2.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications/ )
    install( FILES dist/com.github.sidfactory2.appstream.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo/com.github.sidfactory2.metainfo.xml )
elseif( APPLE )
elseif( WIN32 )
endif()

Msg( DONE "Puh!" )
