# Copyright (C) 2012, BMW AG
#
# This file is part of GENIVI Project AudioManager.
# 
# Contributions are licensed to the GENIVI Alliance under one or more
# Contribution License Agreements.
# 
# copyright
# This Source Code Form is subject to the terms of the
# Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed with
# this file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
# author Christian Linke, christian.linke@bmw.de BMW 2011,2012
#
# For further information see http://www.genivi.org/.
#

cmake_minimum_required(VERSION 3.0)

include(CMakeDependentOption)
find_package(PkgConfig)

IF ("${CONFIG_PREFIX}" STREQUAL "")
    SET(CONFIG_PREFIX ${CMAKE_INSTALL_PREFIX}/etc)
ENDIF ("${CONFIG_PREFIX}" STREQUAL "")
SET(PLUGINS_CONFIG_PATH ${CONFIG_PREFIX}/audiomanager)

OPTION (WITH_COMMAND_INTERFACE_CAPI
        "build with commandinterface common-api plugin" OFF)
OPTION (WITH_COMMAND_INTERFACE_DBUS
        "build with commandinterface dbus plugin" OFF)
OPTION (WITH_ROUTING_INTERFACE_ASYNC
        "build with routing interface async plugin" OFF)
OPTION (WITH_ROUTING_INTERFACE_CAPI
        "build with routing interface common-api plugin" OFF)
OPTION (WITH_ROUTING_INTERFACE_DBUS
        "build with routing interface dbus plugin" OFF)
OPTION (WITH_TEST_CONTROLLER
        "Build with the test controller" OFF)
OPTION (WITH_GENERIC_CONTROLLER
        "Build with the generic controller" OFF)
OPTION (WITH_ROUTING_ADAPTER_ALSA
        "Build with the routing adapter ALSA" OFF)

if (NOT WITH_TEST_CONTROLLER AND NOT WITH_ROUTING_INTERFACE_DBUS AND NOT WITH_GENERIC_CONTROLLER AND NOT WITH_ROUTING_INTERFACE_CAPI AND NOT WITH_ROUTING_INTERFACE_ASYNC AND NOT WITH_COMMAND_INTERFACE_DBUS AND NOT WITH_COMMAND_INTERFACE_CAPI AND NOT WITH_ROUTING_ADAPTER_ALSA)
	message (FATAL_ERROR "No option selected. please choose the plugins you want")
endif (NOT WITH_TEST_CONTROLLER AND NOT WITH_ROUTING_INTERFACE_DBUS AND NOT WITH_GENERIC_CONTROLLER AND NOT WITH_ROUTING_INTERFACE_CAPI AND NOT WITH_ROUTING_INTERFACE_ASYNC AND NOT WITH_COMMAND_INTERFACE_DBUS AND NOT WITH_COMMAND_INTERFACE_CAPI AND NOT WITH_ROUTING_ADAPTER_ALSA)

IF (WITH_GENERIC_CONTROLLER)
	add_subdirectory(PluginControlInterfaceGeneric)
ENDIF(WITH_GENERIC_CONTROLLER)

IF (WITH_TEST_CONTROLLER)
    add_subdirectory(PluginControlInterface)
ENDIF (WITH_TEST_CONTROLLER) 

IF (WITH_COMMAND_INTERFACE_CAPI)
    add_subdirectory(PluginCommandInterfaceCAPI)
ENDIF(WITH_COMMAND_INTERFACE_CAPI)
 
IF (WITH_COMMAND_INTERFACE_DBUS)
    add_subdirectory(PluginCommandInterfaceDbus)
ENDIF (WITH_COMMAND_INTERFACE_DBUS)

IF (WITH_ROUTING_INTERFACE_ASYNC)
    add_subdirectory(PluginRoutingInterfaceAsync)
ENDIF (WITH_ROUTING_INTERFACE_ASYNC)

IF (WITH_ROUTING_INTERFACE_CAPI)
    add_subdirectory(PluginRoutingInterfaceCAPI)
ENDIF(WITH_ROUTING_INTERFACE_CAPI)

IF (WITH_ROUTING_INTERFACE_DBUS)
    add_subdirectory(PluginRoutingInterfaceDbus)
ENDIF(WITH_ROUTING_INTERFACE_DBUS)

IF (WITH_ROUTING_ADAPTER_ALSA)
    add_subdirectory(PluginRoutingAdapterALSA)
ENDIF(WITH_ROUTING_ADAPTER_ALSA)

add_custom_target(plugins-install
  COMMAND
     "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=plugin
     -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
)

add_custom_target(plugin-test-install
  COMMAND
     "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=plugin-tests
     -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
)

