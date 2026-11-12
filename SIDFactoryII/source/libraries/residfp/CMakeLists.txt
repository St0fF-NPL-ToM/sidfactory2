#[[
    CMakeLists Project file for reSIDfp
    -----------------------------------
    This is a try to get one buildsystem working on all 3 major OS for SIDFactoryII.
    … Sometimes code just falls out … St0fF/NPL^ToM

    This is the most simple variation of a CML for reSIDfp as a library,
    as that is how SIDFactoryII uses it.
    NOT making this a library on its own simply fucks up include folders.
    BUT include folders are shared for a target.  So it is not possible to fix a library including
    <assert.h> from system, while that file itself is overwritten in 'foundation'.  It just needs
    to be a library to have its own include tree.
]]
cmake_minimum_required( VERSION 4.2 )

# include better.cmake
file( DOWNLOAD https://gitlab.com/copr-projekte/better-cmake/-/releases/0.2.2/downloads/better.cmake ${CMAKE_BINARY_DIR}/better.cmake )
include( ${CMAKE_BINARY_DIR}/better.cmake )

# declare project
project( reSIDfp )

set( HEADERS
    resample/Resampler.h
    resample/SincResampler.h
    resample/TwoPassSincResampler.h
    resample/ZeroOrderResampler.h
    array.h
    Dac.h
    EnvelopeGenerator.h
    ExternalFilter.h
    Filter.h
    Filter6581.h
    Filter8580.h
    FilterModelConfig.h
    FilterModelConfig8580.h
    Integrator.h
    Integrator8580.h
    OpAmp.h
    SID.h
    siddefs-fp.h
    Spline.h
    Voice.h
    WaveformCalculator.h
    WaveformGenerator.h
)
set( SOURCES
    resample/SincResampler.cpp
    Dac.cpp
    EnvelopeGenerator.cpp
    ExternalFilter.cpp
    Filter.cpp
    Filter6581.cpp
    Filter8580.cpp
    FilterModelConfig.cpp
    FilterModelConfig8580.cpp
    Integrator.cpp
    Integrator8580.cpp
    OpAmp.cpp
    Potentiometer.h
    SID.cpp
    Spline.cpp
    WaveformCalculator.cpp
    WaveformGenerator.cpp
)
set( OTHER
    version.cc
    CMakeLists.txt
    AUTHORS
    COPYING
    README
)
add_library( reSIDfp STATIC ${HEADERS} ${SOURCES} ${OTHER} )
target_include_directories( reSIDfp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} resample )
