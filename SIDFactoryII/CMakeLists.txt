#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
    - main sources subdirectory
]]
cmake_minimum_required( VERSION 4.2 )
# declare sources in this folder, then add the rest
target_sources( SIDFactoryII PUBLIC main.cpp )
add_subdirectory( source )

# Artifacts can be copied with a special target on one hand, but CMake does indeed know that thing
# about installation on the other, making the "dist"-target obsolete.
# - MSVC offers options to debug the installed executable, installed to a local buildroot
install( DIRECTORY color_schemes    TYPE LIBEXEC )
install( DIRECTORY drivers          TYPE LIBEXEC )
install( DIRECTORY music            TYPE LIBEXEC )
install( DIRECTORY overlay          TYPE LIBEXEC )
