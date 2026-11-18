#[[
    CMakeLists Project file for SIDFactoryII / libusb
    -------------------------------------------------
    This file is NOT read in case of a Linux compile.  Linux delivers all includes and libraries
    thru PackageConfig.  And libusb most likely already is part of your system - maybe you need
    the development-libraries, though …
]]
cmake_minimum_required( VERSION 4.2 )

project( libusb )
add_library( libusb STATIC
    libusb/libusb.h
    libusb/libusbi.h
    libusb/version.h
    libusb/version_nano.h
    libusb/io.c
    libusb/core.c
    libusb/descriptor.c
    libusb/hotplug.c
    libusb/sync.c
    libusb/strerror.c
)
target_include_directories( libusb INTERFACE libusb 
                                   PRIVATE libusb/os )
if ( WIN32 )
    target_sources( libusb PUBLIC
        libusb/os/windows_winusb.h
        libusb/os/windows_winusb.c
        libusb/os/windows_common.h
        libusb/os/windows_common.c
        libusb/os/events_windows.h
        libusb/os/events_windows.c
        libusb/os/threads_windows.c
        libusb/os/threads_windows.h
        libusb/os/windows_usbdk.h
        libusb/os/windows_usbdk.c
    )
    target_include_directories( libusb PRIVATE msvc )
elseif( APPLE )
    target_sources( libusb PUBLIC
        libusb/os/darwin_usb.c
        libusb/os/darwin_usb.h
    )
    target_include_directories( libusb PRIVATE Xcode )
endif()
