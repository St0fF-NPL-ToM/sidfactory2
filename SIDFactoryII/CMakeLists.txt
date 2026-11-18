#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
    - main sources subdirectory
]]
cmake_minimum_required( VERSION 4.2 )

# declare sources in this folder, then add the rest
if ( SF2_MAKEFILE_APPROACH ) # handle building like the makefile
    Msg( "Enumerating all source files of SF2 …" )
    # This is not good CMake practice! See the second path on how CMake likes it.
    # Second impact of this modus operandum: IDEs may not list header files.
    file( GLOB_RECURSE SRCS LIST_DIRECTORIES off RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "*.cpp" )
    # replace the tester-sourcefile with the actual lib source file in minizip
    list( TRANSFORM SRCS REPLACE "miniz_tester[.]cpp" "miniz.c" )
    list( LENGTH SRCS _sn )
    Msg( "… found ${_sn} source files, creating target." )
    add_executable( SIDFactoryII ${TPROP} ${SRCS} )
    target_include_directories( SIDFactoryII PUBLIC "source" )
else()
    Msg( "Creating basic target from main.cpp" )
    add_executable( SIDFactoryII ${TPROP} main.cpp )

    # add the rest in succession
    add_subdirectory( source )
endif()

Msg( "Adding link libraries:" LIST ${SF2_LIBS} )
target_link_libraries( SIDFactoryII PUBLIC ${SF2_LIBS} )

# Artifacts can be copied with a special target on one hand, but CMake does indeed know that thing
# about installation on the other, making the "dist"-target obsolete.  We'll have to simply "bend"
# the GNUInstallDirs. (Which is done in the toplevel CML)
install(    DIRECTORY   color_schemes drivers music overlay
            DESTINATION "${SF2_EXEC_DIR}" )
install(    FILES       config.ini
            DESTINATION "${SF2_EXEC_DIR}/config/" )
install(    FILES       COPYING
            DESTINATION "${SF2_DOC_DIR}" )
install(    TARGETS     SIDFactoryII
            DESTINATION "${SF2_EXEC_DIR}" )

if( LINUX )
    # Let's try a CMake-Link?
    install( CODE "
        file( MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR} )
        # symlink from bin-dir to libexec-dir
        file( CREATE_LINK
            ${CMAKE_INSTALL_PREFIX}/${SF2_EXEC_DIR}/SIDFactoryII
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/sidfactory2
            SYMBOLIC RESULT _r )
        if ( _r )
            message( WARNING \"Link-Update failed: ${_r}\" )
        else()
            message( STATUS \"Updated:    ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/sidfactory2\" )
        endif()
        unset( _r )
    " )
elseif( WIN32 )
endif()
