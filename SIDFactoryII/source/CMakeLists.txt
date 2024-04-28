################################################################################
#
# SIDFactoryII ... "source" folder
#
################################################################################
SF2_ADD_SUBDIRS(
	foundation
	libraries
	runtime/editor
	runtime/emulation
	runtime/execution
	utils
)
SF2_ADD_FILES( ${CMAKE_CURRENT_SOURCE_DIR}
	resources/data_char.h
	resources/data_logo.h
	runtime/environmentdefines.h
)
