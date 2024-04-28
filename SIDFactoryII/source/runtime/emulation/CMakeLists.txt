################################################################################
#
# SIDFactoryII ... "runtime/emulation" folder
#
################################################################################
SF2_ADD_FILES( ${CMAKE_CURRENT_SOURCE_DIR}
	cpuframecapture.cpp
	cpumemory.cpp
	cpumos6510.cpp
	cpuframecapture.h
	cpumemory.h
	cpumos6510.h
	icpuwritecallback.h
	imemoryrandomreadaccess.h
	sid/sidproxy.cpp
	sid/sidproxy.h
	sid/sidproxydefines.h
)