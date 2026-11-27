#[[
    SIDFactoryII - Driver Development Kit Fork
    ------------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
        by St0fF/NPL^ToM 2026

    Basic Ideas an approaches:
    -   SIDFactoryII is a great program, still lacking a few features, but great for "the job"
    -   It's the first Editor I realized that can load different "drivers" without loading a
        different exe or Plugin.
    -   So I had a deep look into the source (took a month, produced some PRs) to find out,
        which places I need to modify to fit my needs.

        Buildsystem:    a Software product is as good as it's maintainability
                        If you have to take completely different paths on different OS' to
                        produce a valid build, then you're missing a point, somehow …

        Editor vs. Driver encapsulation:
        When I found out about loading different drivers, I had not yet realized they all followed
        one simple architecture.  And that this architecture is only partially abstracted into
        desciptive interfaces.
        The Editor-Code knows about explicit byte values, instead of knowing explicit semantics,
        mapped to byte value markers.  This is a single layer in-between to be installed, of which
        a large part already exists and the other part consists of correctly moving around
        respective existing code, or sequeezing out code of existing routines, replacing this code
        with the respective "new driver level functions".

    This Master CMakeLists.txt:
    -   declares the project, its options and variables
    -   tries to find all build requirements, OS-dependently
        -   using modern CMake techniques, where applicable: see FetchContent use in Windows path
    -   then includes the subdirectories to produce the final target
    -   manages installation related tasks
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
Msg( START "Configuring SIDFactoryII-DDK on '${CMAKE_HOST_SYSTEM_NAME}'" )

# query build ID, make a version number out of it.
init_Git()
cmd( git show --no-patch --format=%cs OUT_VAR _bd )
cmd( git rev-parse --short HEAD OUT_VAR _id )
calc( _bi "0x${_id}" )                         # CMake version numbering only allows base10-
string( REPLACE "-" "." _bi "${_bd}.${_bi}" )  # integers and colons.  No dashes allowed!

# declare project
project( sf2_ddk VERSION ${_bi} LANGUAGES C CXX )
set( CMAKE_CXX_STANDARD 23 )                  # some libraries by now require more
set( CMAKE_CXX_STANDARD_REQUIRED absolutely ) # LOL coding humor. A string evaluates true.
Msg( "current Version: ${_bd}.${_id} → CMake translation: ${PROJECT_VERSION}" )

# project settings - none for now ...
include( resources/KickAssembler.cmake )

#[[ Preparation for installation differences:
    -----------------------------------------
    - platform-options / preprocessor definitions
	- library gathering
	- installation directories
]]
set( TPROP "" ) # differs per OS … CMAKE-property sent to add_executable for OS-special bundling
if ( LINUX )
    # Linux only configuration options
    add_compile_definitions( __UNIX_JACK__ )
    add_link_options( -flto -lpthread )
    # All major Distros provide PackageConfig. And they provide development packages of
    # SDL2 and Jack, as well as libusb, etc. - so please make sure those are installed.
    find_package( SDL2 QUIET )
    if ( NOT SDL2_FOUND )
        cmake_pkg_config( IMPORT sdl2 )
        cmake_pkg_config( IMPORT sdl2-compat )  # fallback
    endif()
    cmake_pkg_config( IMPORT jack )
    add_compile_definitions( JACK_HAS_PORT_RENAME ) # this is default nowadays ...
    cmake_pkg_config( IMPORT libusb-1.0 )
    get_directory_property( SF2_LIBS IMPORTED_TARGETS )
    list( FILTER SF2_LIBS EXCLUDE REGEX "Git.*" ) # Git was also imported, but not for linking!
    # libexec needs a subdirectory on linux (as we also install folders)
	include( GNUInstallDirs )
    cmake_path( APPEND CMAKE_INSTALL_LIBEXECDIR ${PROJECT_NAME} )
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
    add_subdirectory( libs/libusb )
    include_directories( libs/usbsid-windows )
    set( SF2_LIBS SDL2::SDL2 SDL2::SDL2main libusb )
    set( CACHE{SF2_EXEC_DIR} FORCE VALUE "." )
    set( CACHE{SF2_DOC_DIR} FORCE VALUE "." )
else()
    Msg( FATAL "Seems like your operating system is not yet supported. Sorry!\nYou may as well contribute by extending this CMake-buildchain." )
endif()

# prepare DEFINES: build-number and platform
set( BN "\"${_bd}.${_id}\"" )
string( TOUPPER "${CMAKE_HOST_SYSTEM_NAME}" PLATFORM )
add_compile_definitions( _SF2_${PLATFORM} _BUILD_NR=${BN} )

# Create the executable inside the project base folder
add_subdirectory( source )

Msg( "Adding link libraries:" LIST ${SF2_LIBS} )
target_link_libraries( SIDFactoryII PUBLIC ${SF2_LIBS} )

# Installation tasks from here:
install( DIRECTORY  resources/color_schemes
                    resources/drivers
                    resources/music
                    resources/overlay           DESTINATION "${SF2_EXEC_DIR}" )
install( FILES      resources/config.ini        DESTINATION "${SF2_EXEC_DIR}/config/" )
install( FILES      resources/COPYING           DESTINATION "${SF2_DOC_DIR}" )

install( DIRECTORY dist/documentation    DESTINATION "${SF2_DOC_DIR}" )
install( FILES resources/README.md       DESTINATION "${SF2_DOC_DIR}" )
if ( LINUX )
    # Linux needs the icon and desktop entries in special XDG-DIRS
    install( FILES old_macos/icon.svg        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps/${PROJECT_NAME}.svg )
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
