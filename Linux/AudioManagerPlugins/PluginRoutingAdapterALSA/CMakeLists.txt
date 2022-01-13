# Copyright (c) 2016 GENIVI Alliance
# Copyright (c) 2016 Advanced Driver Information Technology
#
# author Mattia Guerra, mguerra@de.adit-jv.com ADIT 2016
#
# copyright
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
# THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# For further information see http://www.genivi.org/.
#
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

INCLUDE(CMakeDependentOption)
FIND_PACKAGE(PkgConfig)

PROJECT(PluginRoutingAdapterALSA)

FIND_PACKAGE(AudioManager REQUIRED > 7.3.0)  
FIND_PACKAGE(AudioManagerUtilities REQUIRED > 7.3.0) 

IF (WITH_DLT)
    pkg_check_modules(DLT_REQUIRED automotive-dlt>=2.2.0)
    add_definitions(${DLT_CFLAGS_OTHER})
    include_directories(${DLT_INCLUDE_DIRS})
    link_directories(${DLT_LIBRARY_DIRS})
ENDIF(WITH_DLT)

OPTION (WITH_DOCUMENTATION
        "build plugins with documentation" OFF )
OPTION (WITH_SHARED_PROXY_INTERFACE
        "build Interface Proxy as a Shared Library" ON )
OPTION (WITH_TEST_ROUTING_ADAPTER_ALSA
        "Build with the routing adapter ALSA test" OFF )
OPTION (WITH_DEVICE_DETECTOR
        "Build with the USB Device Detector using udev" OFF )

SET(PLUGIN_LIB_NAME PluginRoutingAdapterALSA)

SET(AM_LINK_LIBS ${CMAKE_DL_LIBS})

SET(INCLUDE_FOLDER "include")

SET(CONF_ROOT "${PLUGINS_CONFIG_PATH}/routing")

FIND_PACKAGE(LibXml2 REQUIRED)

FIND_PACKAGE(ALSA REQUIRED)

IF(WITH_DEVICE_DETECTOR)
set(UDEV_NAMES ${UDEV_NAMES} udev)
find_library(UDEV_LIBRARY NAMES ${UDEV_NAMES})
add_definitions(-DWITH_DEVICE_DETECTOR)
ENDIF (WITH_DEVICE_DETECTOR)

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${AUDIO_INCLUDE_FOLDER}
    ${AudioManager_INCLUDE_DIRS}
    ${AudioManagerUtilities_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    ${INCLUDE_FOLDER}
)

IF(WITH_DLT)
    INCLUDE_DIRECTORIES(${INCLUDE_DIRECTORIES}
        ${DLT_INCLUDE_DIRS})
    SET(AM_LINK_LIBS
        ${AM_LINK_LIBS}
        ${DLT_LIBRARIES})
ENDIF(WITH_DLT)

add_definitions(-DROUTING_ADAPTER_ALSA_DEFAULT_CONF_ROOT="${CONF_ROOT}")

##build flags set(CPACK_RPM_COMPONENT_INSTALL ON)
set(ROUTING_ADAPTER_ALSA_CMAKE_CXX_FLAGS "-std=c++11 -pedantic -rdynamic -Wno-variadic-macros")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROUTING_ADAPTER_ALSA_CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")

# all source files go here
file(GLOB PLUGINALSA_SRCS_CXX "src/*.cpp")

add_library(PluginRoutingAdapterALSA MODULE ${PLUGINALSA_SRCS_CXX})

TARGET_LINK_LIBRARIES(PluginRoutingAdapterALSA
    ${AM_LINK_LIBS}
    ${LIBXML2_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${UDEV_LIBRARY}
)

INSTALL(TARGETS PluginRoutingAdapterALSA
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${LIB_INSTALL_SUFFIX}/routing
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
        COMPONENT plugin)

INSTALL(DIRECTORY "${INCLUDE_FOLDER}/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/audiomanager/
    COMPONENT dev)

INSTALL(DIRECTORY conf/
    DESTINATION ${CONF_ROOT}
    COMPONENT conf
)

set(DOXY_FILE ${CMAKE_CURRENT_BINARY_DIR}/PluginRoutingAdapterALSA.doxyfile)
set(DOC_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/doc)

if(WITH_DOCUMENTATION)
    find_package(Doxygen)
    configure_file(${CMAKE_SOURCE_DIR}/PluginRoutingAdapterALSA/doc/PluginRoutingAdapterALSA.doxyfile ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile)
    add_custom_target (PluginRoutingAdapterALSADoku ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOC_OUTPUT_PATH}/Doxyfile WORKING_DIRECTORY ${DOC_OUTPUT_PATH}
        SOURCES ${CMAKE_SOURCE_DIR}/PluginRoutingAdapterALSA ${DOC_OUTPUT_PATH}/Doxyfile
    )
    file(COPY ${CMAKE_SOURCE_DIR}/PluginRoutingAdapterALSA/doc/images/ DESTINATION ${DOC_OUTPUT_PATH}/images/)
endif(WITH_DOCUMENTATION)

if(WITH_TEST_ROUTING_ADAPTER_ALSA)
    add_subdirectory(test)
endif(WITH_TEST_ROUTING_ADAPTER_ALSA)
