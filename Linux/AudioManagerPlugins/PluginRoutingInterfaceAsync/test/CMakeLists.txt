# Copyright (c) 2012 GENIVI Alliance
# Copyright (c) 2012 BMW
#
# author Christian Linke, christian.linke@bmw.de BMW 2011,2012
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

PROJECT(AmRoutingReceiverAsync VERSION 7.4.0)

set(EXECUTABLE_OUTPUT_PATH ${TEST_EXECUTABLE_OUTPUT_PATH})

FIND_PACKAGE(Threads)
FIND_PACKAGE(PkgConfig)
find_package(AudioManager REQUIRED > 7.0.0)  
find_package(AudioManagerUtilities REQUIRED > 7.0.0)
find_package(AudioManagerCore REQUIRED > 7.0.0)
pkg_check_modules (GTEST REQUIRED "gtest >= 1.6.0")
pkg_check_modules (GMOCK REQUIRED "gmock >= 1.6.0")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs -DUNIT_TEST=1 -DDLT_CONTEXT=AudioManager")

INCLUDE_DIRECTORIES(   
    ${AudioManagerUtilities_INCLUDE_DIRS} 
    ${AudioManagerCore_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR} 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${COMMON_API_INCLUDE_DIRS}
    ${COMMON_API_GEN_INCLUDE_DIR}
)

 
file(GLOB ASYNC_PLUGIN_INTERFACE_SRCS_CXX 
     "../src/*.cpp"  
     "CAmRoutingReceiverAsync.cpp" 
)


ADD_EXECUTABLE(AmRoutingReceiveAsyncTest ${ASYNC_PLUGIN_INTERFACE_SRCS_CXX})

TARGET_LINK_LIBRARIES(AmRoutingReceiveAsyncTest 
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${CommonAPI_LIBRARY}
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES}
    ${AudioManagerUtilities_LIBRARIES}
)

INSTALL(TARGETS AmRoutingReceiveAsyncTest 
        DESTINATION ${TEST_EXECUTABLE_INSTALL_PATH}
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
        COMPONENT tests
)

