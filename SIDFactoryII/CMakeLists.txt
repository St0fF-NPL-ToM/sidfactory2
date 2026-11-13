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
# about installation on the other, making the "dist"-target obsolete.
# - MSVC offers options to debug the installed executable, installed to a local buildroot
install( DIRECTORY color_schemes            TYPE LIBEXEC )
install( DIRECTORY drivers                  TYPE LIBEXEC )
install( DIRECTORY music                    TYPE LIBEXEC )
install( DIRECTORY overlay                  TYPE LIBEXEC )
install( FILES     config.ini               DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/config/ )
# Targets are to be installed differently (unless libexec and bindir are the same)
install( TARGETS SIDFactoryII EXPORT SIDFactoryII
    DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

if( LINUX )
    # Let's try a CMake-Link?
    install( FILES COPYING                   TYPE DOC )
    install( CODE "
        file( MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR} )
        file( CREATE_LINK
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/SIDFactoryII
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/sidfactory2 SYMBOLIC RESULT _r )
        if ( _r )
            message( WARNING \"Link-Update failed: ${_r}\" )
        else()
            message( STATUS \"Updated:    ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/sidfactory2\" )
        endif()
        unset( _r )
    " )
endif()
