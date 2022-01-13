# Copyright (c) 2016 GENIVI Alliance
# Copyright (c) 2016 Advanced Driver Information Technology
#
# author Jens Lorenz, jlorenz@de.adit-jv.com ADIT 2016
#	     Mattia Guerra, mguerra@de.adit-jv.com ADIT 2016
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

cmake_minimum_required(VERSION 3.0)

INCLUDE(CMakeDependentOption)
FIND_PACKAGE(PkgConfig)

project(PluginRoutingAdapterALSATest)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

FIND_PACKAGE(Threads)

FIND_PACKAGE(LibXml2 REQUIRED)

FIND_LIBRARY(gtest NAMES libgtest.a)

set (INCLUDE_FOLDER include)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${AUDIO_INCLUDE_FOLDER}
    ${CMAKE_SOURCE_DIR}
    ${INCLUDE_FOLDER}
    ${GMOCK_INCLUDE_DIR}
    ${GOOGLE_TEST_INCLUDE_DIR}
)

file(GLOB RoutingAdapterALSA_SRCS_CXX
    "src/*.cpp"
)

ADD_EXECUTABLE(AmPluginRoutingAdapterALSATest ${RoutingAdapterALSA_SRCS_CXX})

TARGET_LINK_LIBRARIES(AmPluginRoutingAdapterALSATest
    gtest
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBXML2_LIBRARIES}
)

INSTALL(TARGETS AmPluginRoutingAdapterALSATest
        DESTINATION ${TEST_EXECUTABLE_INSTALL_PATH}
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
        COMPONENT tests
)
