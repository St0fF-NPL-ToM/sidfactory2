#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
    - sub-sources subdirectory
]]
cmake_minimum_required( VERSION 4.2 )

add_subdirectory( libraries/miniz )
set( SF2_LIBS ${SF2_LIBS} SF2::MZ PARENT_SCOPE )

target_include_directories( SIDFactoryII PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    foundation
    foundation/base
    foundation/graphics
    foundation/input
    foundation/platform
    foundation/platform/sdl
    foundation/sound
    libraries/ghc
    # libraries/miniz
    libraries/picopng
    libraries/residfp
    libraries/residfp/resample
    libraries/rtmidi
    resources
    utils/config
    utils
)
set( HEADERS
    foundation/base/assert.h
    foundation/base/toptional.h
    foundation/base/types.h
    foundation/graphics/color.h
    foundation/graphics/drawfield.h
    foundation/graphics/image.h
    foundation/graphics/imanaged.h
    foundation/graphics/textfield.h
    foundation/graphics/viewport.h
    foundation/graphics/wrapped_string.h
    foundation/input/keyboard_utils.h
    foundation/input/keyboard.h
    foundation/input/mouse.h
    foundation/platform/imutex.h
    foundation/platform/iplatform.h
    foundation/platform/platform_factory.h
    foundation/platform/sdl/mutex_sdl.h
    foundation/platform/sdl/platform_sdl_linux.h
    foundation/platform/sdl/platform_sdl_macos.h
    foundation/platform/sdl/platform_sdl_windows.h
    foundation/platform/sdl/platform_sdl.h
    foundation/sound/audiostream.h

    libraries/ghc/filesystem.h
    libraries/ghc/fs_fwd.h
    libraries/ghc/fs_impl.h
    libraries/ghc/fs_std_fwd.h
    libraries/ghc/fs_std_impl.h
    libraries/ghc/fs_std.h
    # libraries/miniz/timer.h
    libraries/picopng/picopng.h
    libraries/residfp/resample/Resampler.h
    libraries/residfp/resample/SincResampler.h
    libraries/residfp/resample/TwoPassSincResampler.h
    libraries/residfp/resample/ZeroOrderResampler.h
    libraries/residfp/array.h
    libraries/residfp/Dac.h
    libraries/residfp/EnvelopeGenerator.h
    libraries/residfp/ExternalFilter.h
    libraries/residfp/Filter.h
    libraries/residfp/Filter6581.h
    libraries/residfp/Filter8580.h
    libraries/residfp/FilterModelConfig.h
    libraries/residfp/FilterModelConfig8580.h
    libraries/residfp/Integrator.h
    libraries/residfp/Integrator8580.h
    libraries/residfp/OpAmp.h
    libraries/residfp/Potentiometer.h
    libraries/residfp/SID.h
    libraries/residfp/siddefs-fp.h
    libraries/residfp/Spline.h
    libraries/residfp/Voice.h
    libraries/residfp/WaveformCalculator.h
    libraries/residfp/WaveformGenerator.h
    libraries/rtmidi/RtMidi.h

    resources/data_char.h
    resources/data_logo.h

    utils/config/configcolors.h
    utils/config/configtypes.h
    utils/config/configutils.h
    utils/bit_array.h
    utils/c64file.h
    utils/configfile.h
    utils/delegate.h
    utils/event.h
    utils/global.h
    utils/keyhook.h
    utils/keyhookstore.h
    utils/logging.h
    utils/psidfile.h
    utils/rtmidi_utils.h
    utils/usercolors.h
    utils/utilities.h
)
set( SOURCES
    foundation/graphics/color.cpp
    foundation/graphics/drawfield.cpp
    foundation/graphics/image.cpp
    foundation/graphics/textfield.cpp
    foundation/graphics/viewport.cpp
    foundation/graphics/wrapped_string.cpp
    foundation/input/keyboard_utils.cpp
    foundation/input/keyboard.cpp
    foundation/input/mouse.cpp
    foundation/platform/platform_factory.cpp
    foundation/platform/sdl/mutex_sdl.cpp
    foundation/platform/sdl/platform_sdl_linux.cpp
    foundation/platform/sdl/platform_sdl_macos.cpp
    foundation/platform/sdl/platform_sdl_windows.cpp
    foundation/platform/sdl/platform_sdl.cpp
    foundation/sound/audiostream.cpp

    # libraries/miniz/miniz.c
    # libraries/miniz/timer.cpp
    # libraries/miniz/tinfl.c
    libraries/picopng/picopng.cpp
    libraries/residfp/resample/SincResampler.cpp
    libraries/residfp/Dac.cpp
    libraries/residfp/EnvelopeGenerator.cpp
    libraries/residfp/ExternalFilter.cpp
    libraries/residfp/Filter.cpp
    libraries/residfp/Filter6581.cpp
    libraries/residfp/Filter8580.cpp
    libraries/residfp/FilterModelConfig.cpp
    libraries/residfp/FilterModelConfig8580.cpp
    libraries/residfp/Integrator.cpp
    libraries/residfp/Integrator8580.cpp
    libraries/residfp/OpAmp.cpp
    libraries/residfp/SID.cpp
    libraries/residfp/Spline.cpp
    libraries/residfp/version.cc
    libraries/residfp/WaveformCalculator.cpp
    libraries/residfp/WaveformGenerator.cpp
    libraries/rtmidi/RtMidi.cpp

    utils/config/configcolors.cpp
    utils/config/configtypes.cpp
    utils/config/configutils.cpp
    utils/bit_array.cpp
    utils/c64file.cpp
    utils/configfile.cpp
    utils/global.cpp
    utils/keyhook.cpp
    utils/keyhookstore.cpp
    utils/logging.cpp
    utils/psidfile.cpp
    utils/rtmidi_utils.cpp
    utils/usercolors.cpp
    utils/utilities.cpp
)
set( OTHERS
    libraries/residfp/AUTHORS
    libraries/residfp/COPYING
    libraries/residfp/README
)
target_sources( SIDFactoryII PUBLIC ${HEADERS} ${SOURCES} ${OTHERS} )
add_subdirectory( runtime )
