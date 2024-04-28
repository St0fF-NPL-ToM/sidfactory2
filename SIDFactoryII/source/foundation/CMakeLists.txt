################################################################################
#
# SIDFactoryII ... "foundation" folder
#
################################################################################
SF2_ADD_SUBDIRS( platform/sdl )
SF2_ADD_FILES( ${CMAKE_CURRENT_SOURCE_DIR}
	base/assert.h
	base/types.h
	graphics/color.h				graphics/color.cpp
	graphics/drawfield.h			graphics/drawfield.cpp
	graphics/image.h				graphics/image.cpp
	graphics/imanaged.h
	graphics/textfield.h			graphics/textfield.cpp
	graphics/viewport.h				graphics/viewport.cpp
	graphics/wrapped_string.h		graphics/wrapped_string.cpp
	input/keyboard.h				input/keyboard.cpp
	input/keyboard_utils.h			input/keyboard_utils.cpp
	input/mouse.h					input/mouse.cpp
	platform/imutex.h
	platform/iplatform.h
	platform/platform_factory.h		platform/platform_factory.cpp
	sound/audiostream.h				sound/audiostream.cpp
)
