include(FindPkgConfig)
pkg_check_modules(LINUX_HEADERS linux-uapi-headers)

if(NOT LINUX_HEADERS_FOUND)
	find_path(LINUX_HEADERS_INCLUDE_DIR
		NAMES linux/usb/functionfs.h
		HINTS $ENV{PKG_CONFIG_SYSROOT_DIR}/usr/uapi/include/
		HINTS ${CMAKE_SYSROOT}/usr/uapi/include/
		PATHS $ENV{PKG_CONFIG_SYSROOT_DIR}/usr/uapi/include/
		PATHS ${CMAKE_SYSROOT}/usr/uapi/include/)

	# Use some standard module to handle the QUIETLY and REQUIRED arguments, and
	# set LINUX_HEADERS_FOUND to TRUE if these two variables are set.
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(LINUX_HEADERS REQUIRED_VARS LINUX_HEADERS_INCLUDE_DIR)

	if(LINUX_HEADERS_FOUND)
		set(LINUX_HEADERS_INCLUDEDIR ${LINUX_HEADERS_INCLUDE_DIR})
	endif()
endif()

mark_as_advanced(LINUX_HEADERS_INCLUDE_DIR)
