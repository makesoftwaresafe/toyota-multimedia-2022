#######
# @licence make begin@
# SPDX license identifier: MPL-2.0
#
# Copyright (C) 2016, ADIT GmbH
#
# This file is part of GENIVI Project DLT - Diagnostic Log and Trace.
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License (MPL), v. 2.0.
# If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
#
# For further information see http://www.genivi.org/.
# @licence end@
#######

set(dlt_monitor_SRCS dlt-monitor.c ../shared/dlt_config_file_parser.c)
add_executable(dlt-monitor ${dlt_monitor_SRCS})
target_link_libraries(dlt-monitor dlt dl ${EXPAT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(dlt-monitor PROPERTIES LINKER_LANGUAGE C)

add_subdirectory( cpu )
add_subdirectory( product )
add_subdirectory( filesystem )
add_subdirectory( process )
add_subdirectory( io )
add_subdirectory( network )
add_subdirectory( memory )

install(TARGETS dlt-monitor
	RUNTIME DESTINATION bin
	COMPONENT base)

install(FILES dlt-sysmon.conf
	DESTINATION ${CONFIGURATION_FILES_DIR}
	COMPONENT base)
