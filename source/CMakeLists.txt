#[[
    CMakeLists Project file for SIDFactoryII
    ----------------------------------------
    This is a try to get one buildsystem working on all 3 major OS.
        by St0fF/NPL^ToM 2026    → source subdirectory

    CMake-Approach:
    - keep the CMakeLists.txt file short enough to stay manageable (like <200 lines)
      - subfolders with only few TUs/headers are added directly from here
      - subfolders that could be a lib themselves are included as a subfolder
    - source files / TUs are added "normally"
    - headers and resources are given their respective file groups
]]
cmake_minimum_required( VERSION 4.2 )

# first up: this is the master include folder
target_include_directories( SIDFactoryII PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" )

# these are libraries, they should be used as such to ease maintenance:
#
add_subdirectory( libraries/miniz )
add_subdirectory( libraries/residfp )
add_subdirectory( libraries/rtmidi )
set( SF2_LIBS ${SF2_LIBS} miniz reSIDfp RtMidi PARENT_SCOPE )

# this folder also is the main include_directory
target_include_directories( SIDFactoryII PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )

# declare all non-library headers
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

    libraries/picopng/picopng.h

    libraries/usbsid/USBSID.h
    libraries/usbsid/USBSID_Manager.h

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
    utils/sidpanning.h
)
# declare the sources
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

    libraries/picopng/picopng.cpp

    libraries/usbsid/USBSID.cpp
    libraries/usbsid/USBSID_Manager.cpp

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
    utils/sidpanning.cpp
)
target_sources( SIDFactoryII PRIVATE ${SOURCES} )
target_sources( SIDFactoryII PRIVATE FILE_SET headers TYPE HEADERS FILES ${HEADERS} )
add_subdirectory( runtime )
