include(FindPkgConfig)
pkg_check_modules(SYSTEMD libsystemd)

if(NOT SYSTEMD_FOUND)
	find_path(SYSTEMD_INCLUDE_DIRS NAMES systemd/sd-daemon.h DOC "The Systemd include directory")

	find_library(SYSTEMD_LIBRARIES NAMES systemd DOC "The Systemd library")

	# Use some standard module to handle the QUIETLY and REQUIRED arguments, and
	# set SYSTEMD_FOUND to TRUE if these two variables are set.
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(SYSTEMD REQUIRED_VARS SYSTEMD_LIBRARIES SYSTEMD_INCLUDE_DIRS)

	if(SYSTEMD_FOUND)
		set(SYSTEMD_LIBRARY ${SYSTEMD_LIBRARIES})
		set(SYSTEMD_INCLUDEDIR ${SYSTEMD_INCLUDE_DIRS})
	endif()
endif()

mark_as_advanced(SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES)
