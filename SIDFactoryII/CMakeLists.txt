#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
    - main sources subdirectory
]]
cmake_minimum_required( VERSION 4.2 )
# declare sources in this folder, then add the rest
Msg( "Creating basic target from main.cpp" )
add_executable( SIDFactoryII ${TPROP} main.cpp )
add_subdirectory( source )

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
        # move 'badly named' executable into libexec
        file( RENAME ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/SIDFactoryII
                     ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/SIDFactoryII )
        # symlink from bin-dir to libexec-dir
        file( CREATE_LINK SYMBOLIC RESULT _r
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/SIDFactoryII
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/sidfactory2 )
        if ( _r )
            message( WARNING \"Link-Update failed: ${_r}\" )
        else()
            message( STATUS \"Updated:    ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/sidfactory2\" )
        endif()
        unset( _r )
    " )
elseif( WIN32 )
endif()
