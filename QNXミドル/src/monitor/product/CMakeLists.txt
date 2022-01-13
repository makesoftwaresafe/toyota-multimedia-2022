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

set(dlt_LIB_SRCS dlt-monitor-adit-product.c )

add_library(data-collector-adit-product-info ${dlt_LIB_SRCS})
target_link_libraries(data-collector-adit-product-info rt ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(data-collector-adit-product-info PROPERTIES VERSION ${DLT_VERSION} SOVERSION ${DLT_MAJOR_VERSION})

install(TARGETS data-collector-adit-product-info
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
	COMPONENT base)
