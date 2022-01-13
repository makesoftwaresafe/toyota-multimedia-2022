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

PROJECT(PluginControlInterface VERSION 7.4.0)

include(CMakeDependentOption)
find_package(PkgConfig)
find_package(AudioManager REQUIRED > 7.3.0)  
find_package(AudioManagerUtilities REQUIRED > 7.3.0)   

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR} 
	${CMAKE_CURRENT_BINARY_DIR}
	${AudioManager_INCLUDE_DIRS}
	${AudioManagerUtilities_INCLUDE_DIRS}
)

LINK_DIRECTORIES(${LINK_DIRECTORIES} ${AudioManagerUtilities_LIB_DIRECTORIES})

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wno-variadic-macro")

# all source files go here
file(GLOB PLUGIN_CONTROL_SRCS_CXX "src/*.cpp")

add_library(PluginControlInterface MODULE ${PLUGIN_CONTROL_SRCS_CXX})


TARGET_LINK_LIBRARIES(PluginControlInterface)

IF(WITH_TESTS)
	#add_subdirectory (test)
ENDIF(WITH_TESTS)


INSTALL(TARGETS PluginControlInterface   
        DESTINATION ${CONTROLLER_PLUGIN_DIR}
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
        COMPONENT plugin
)


