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

PROJECT(PluginRoutingInterfaceCAPI VERSION 7.4.0)

find_package(AudioManager REQUIRED > 7.3.0)  
find_package(AudioManagerUtilities REQUIRED > 7.3.0)

if (AudioManager_FOUND)
	message(STATUS "Audiomanager version: ${AudioManager_VERSION}")
endif (AudioManager_FOUND)
      
IF(NOT WITH_CAPI_WRAPPER)
	MESSAGE(ERROR "Audiomanager was not build with CAPI Wrapper!")
ENDIF(NOT WITH_CAPI_WRAPPER)

OPTION (WITH_ROUTING_INTERFACE_GENERIC_LIB
		"build routing iterface common-api glue code" OFF )

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
file(GLOB PLUGINCAPI_SRCS_CXX "src/*.cpp" )


SET(ROUTINGCONTROL_CAPI RoutingControlGenericCommonAPI )
SET(ROUTINGCONTROL_CAPI_LIB lib${ROUTINGCONTROL_CAPI}.so )                
SET(ROUTINGCONTROL_CAPI_LIB ${ROUTINGCONTROL_CAPI_LIB} PARENT_SCOPE )

INCLUDE (cmake/CommonAPI.cmake)
LOAD_COMMONAPI_LIB()

IF(NOT WITH_ROUTING_INTERFACE_GENERIC_LIB ) 
	pkg_check_modules(RC org.genivi.am.routinginterfaceProxy)
	pkg_check_modules(RC1 org.genivi.am.routinginterfaceStub)
	IF(NOT RC_FOUND)
		SET(WITH_ROUTING_INTERFACE_GENERIC_LIB ON)
	ELSE(NOT RC_FOUND)
	   SET(ROUTING_GEN_INCLUDE_DIR ${RC_INCLUDEDIR}) 
	   INCLUDE_DIRECTORIES(${INCLUDE_DIRECTORIES} ${COMMONAPI_INCLUDE_DIRS} ${RC_INCLUDE_DIRS} ${RC1_INCLUDE_DIRS})  
	   LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${RC_LIBRARY_DIRS} ${RC1_LIBRARY_DIRS}) 
	set (ROUTINGCONTROL_CAPI
		${RC_LIBRARIES}
		${RC1_LIBRARIES}
		)           
	ENDIF(NOT RC_FOUND)
ENDIF()

IF(WITH_ROUTING_INTERFACE_GENERIC_LIB)        
  
	SET(SRC_GEN src-gen/v${COMMONAPI_VERSION_NUMBER}/Generic)
	# Find the common-api sources. 
	# The sources can be placed in the build directory (DESTINATION) or in the source tree (ALT_DESTINATION). 
	# The ALT_DESTINATION is used as fall back if the DESTINATION directory dosen't contain the sources or you just don't want to use the generator.
	COMMON_API_GENERATE_SOURCES(
								# Prefix.
								TARGET ROUTING 
								# A list with fidls for the generic generator. Leave this parameter empty if you won't generic code.
								FIDLS_GENERIC fidl/RoutingInterface.fidl 
								# A relative path to the build directory or an absolute path.
								DESTINATION ${SRC_GEN} 
								# An alternative relative/absolute path with common-api sources, usually in the source tree.
								ALT_DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/src-gen 
								)
	
	INCLUDE_DIRECTORIES(${INCLUDE_DIRECTORIES}
			${ROUTING_GEN_INCLUDE_DIR}
			${COMMONAPI_INCLUDE_DIRS}
			)
				 
	add_library (${ROUTINGCONTROL_CAPI} SHARED ${ROUTING_GEN_COMMON_SOURCES} ) 	
	target_link_libraries(${ROUTINGCONTROL_CAPI} ${CommonAPI_LIBRARY} ${AUDIOMANAGERUTILITIES_LIBRARIES})
	set_target_properties(${ROUTINGCONTROL_CAPI} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})	    	
	                                    
	
  	
								
  INSTALL (TARGETS ${ROUTINGCONTROL_CAPI}
		  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
		  LIBRARY DESTINATION lib
		  COMPONENT plugin)                   
   set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)       
#       link_directories(${CMAKE_INSTALL_PREFIX}/lib/${LIB_INSTALL_SUFFIX})  
   link_directories(${CMAKE_CURRENT_BINARY_DIR})   

  INSTALL( DIRECTORY ${ROUTING_GEN_INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_INSTALL_SUFFIX} COMPONENT plugin)
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cmake/plugin.pc.in ${CMAKE_BINARY_DIR}/${ROUTINGCONTROL_CAPI}.pc @ONLY )  
  install(FILES ${CMAKE_BINARY_DIR}/${ROUTINGCONTROL_CAPI}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT plugin)
					
ENDIF()  

add_library(PluginRoutingInterfaceCAPI MODULE ${PLUGINCAPI_SRCS_CXX})

TARGET_LINK_LIBRARIES(PluginRoutingInterfaceCAPI 
	${ROUTINGCONTROL_CAPI}
)
	
INSTALL(TARGETS PluginRoutingInterfaceCAPI 
		DESTINATION ${DEFAULT_PLUGIN_ROUTING_DIR}
		PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
		COMPONENT plugin
)

IF(WITH_TESTS)
	add_subdirectory (test)
ENDIF(WITH_TESTS)

