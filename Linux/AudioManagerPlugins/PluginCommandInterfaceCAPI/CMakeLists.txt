# Copyright (c) 2012 GENIVI Alliance
# Copyright (c) 2012 BMW
#
# author Aleksandar Donchev, aleksander.donchev@partner.bmw.de BMW 2013
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

include(CMakeDependentOption)
find_package(PkgConfig)

project(PluginCommandInterfaceCAPI VERSION 7.4.0)

find_package(AudioManager REQUIRED > 7.3.0)  
find_package(AudioManagerUtilities REQUIRED > 7.3.0) 

if (AudioManager_FOUND)
	message(STATUS "Audiomanager version: ${AudioManager_VERSION}")
endif (AudioManager_FOUND)
      
IF(NOT WITH_CAPI_WRAPPER)
	MESSAGE(ERROR "Audiomanager was not build with CAPI Wrapper!")
ENDIF(NOT WITH_CAPI_WRAPPER)
        
OPTION (WITH_COMMAND_INTERFACE_GENERIC_LIB
    "build command interface common-api glue code" OFF )


INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR} 
	${CMAKE_CURRENT_BINARY_DIR}
	${AudioManager_INCLUDE_DIRS}
	${AudioManagerUtilities_INCLUDE_DIRS}
)

LINK_DIRECTORIES(${LINK_DIRECTORIES} ${AudioManagerUtilities_LIB_DIRECTORIES})

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wno-variadic-macro")

INCLUDE (cmake/CommonAPI.cmake)
LOAD_COMMONAPI_LIB()  
    
# all source files go here
file(GLOB PLUGINCAPI_SRCS_CXX "src/*.cpp" )
SET(PLUGINCAPI_SRCS_CXX ${PLUGINCAPI_SRCS_CXX})

pkg_check_modules(CC org.genivi.am.commandinterfaceStub)

IF(NOT CC_FOUND)
SET(WITH_COMMAND_INTERFACE_GENERIC_LIB ON)
ELSE(NOT CC_FOUND)
	SET(WITH_COMMAND_INTERFACE_GENERIC_LIB OFF)
	INCLUDE_DIRECTORIES(
		${INCLUDE_DIRECTORIES} 
		${CC_INCLUDE_DIRS})
	set (AM_LINK_LIBS
		${CC_LIBRARIES})
	LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${CC_LIBRARY_DIRS})
ENDIF(NOT CC_FOUND)
    

IF(WITH_COMMAND_INTERFACE_GENERIC_LIB )    

	SET(COMMANDCONTROL_CAPI CommandInterfaceProviderSomeIpStub )
	SET(COMMANDCONTROL_CAPI_LIB lib${COMMANDCONTROL_CAPI}.so )
	SET(COMMANDCONTROL_CAPI_LIB ${COMMANDCONTROL_CAPI_LIB} PARENT_SCOPE)



	SET(SRC_GEN src-gen/v${COMMONAPI_VERSION_NUMBER}/Generic)

	# Find the common-api sources. 
	# The sources can be placed in the build directory (DESTINATION) or in the source tree (ALT_DESTINATION). 
	# The ALT_DESTINATION is used as fall back if the DESTINATION directory dosen't contain the sources or you just don't want to use the generator.
	COMMON_API_GENERATE_SOURCES(
								# Prefix.
								TARGET COMMAND_CTR 
								# A list with fidls for the generic generator. Leave this parameter empty if you won't generic code.
								FIDLS_GENERIC fidl/CommandInterface.fidl
								# A list with fidls for the binding generator. Leave this parameter empty if you won't binding code.
								FIDLS_BINDING #Empty
								# A relative path to the build directory or an absolute path.
								DESTINATION ${SRC_GEN} 
								# An alternative relative/absolute path with common-api sources, usually in the source tree.
								ALT_DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/src-gen
								)

	INCLUDE_DIRECTORIES(	${INCLUDE_DIRECTORIES}
				${COMMAND_CTR_GEN_INCLUDE_DIR}
				${COMMONAPI_INCLUDE_DIRS}
				)


	add_library (${COMMANDCONTROL_CAPI} SHARED ${COMMAND_CTR_GEN_COMMON_SOURCES} )        
	target_link_libraries(${COMMANDCONTROL_CAPI} ${CommonAPI_LIBRARY}  )

	set_target_properties(${COMMANDCONTROL_CAPI} PROPERTIES 
								LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
										  
	INSTALL (TARGETS ${COMMANDCONTROL_CAPI}
			  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
			  LIBRARY DESTINATION lib
			  COMPONENT plugin)    
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	link_directories(${CMAKE_CURRENT_BINARY_DIR}) 

	INSTALL( DIRECTORY ${COMMAND_CTR_GEN_INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_INSTALL_SUFFIX} COMPONENT plugin)

	configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cmake/plugin.pc.in ${CMAKE_BINARY_DIR}/${COMMANDCONTROL_CAPI}.pc @ONLY )  
	install(FILES ${CMAKE_BINARY_DIR}/${COMMANDCONTROL_CAPI}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT plugin)
ENDIF()

  
add_library(PluginCommandInterfaceCAPI MODULE ${PLUGINCAPI_SRCS_CXX})

TARGET_LINK_LIBRARIES(PluginCommandInterfaceCAPI ${AM_LINK_LIBS} ${AudioManagerUtilities_LIBRARIES})
         
install(TARGETS PluginCommandInterfaceCAPI          
    DESTINATION ${DEFAULT_PLUGIN_COMMAND_DIR}
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
    COMPONENT lib
)

IF(WITH_TESTS)
	add_subdirectory (test)
ENDIF(WITH_TESTS)   


