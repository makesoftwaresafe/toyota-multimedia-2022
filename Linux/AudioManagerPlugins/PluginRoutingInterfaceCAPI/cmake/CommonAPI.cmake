#
# Find common-api sources
#
# This module defines these variables:
#
#  ${PARAMS_TARGET}_GEN_HEADERS
#      A list with generated headers
#  ${PARAMS_TARGET}_GEN_SOURCES
#      A list with generated sources
#  ${PARAMS_TARGET}_GEN_INCLUDE_DIR
#     A list with include directories
#
#  ${PARAMS_TARGET}_GEN_BINDING_STUB_HEADERS
#     A list with generated stub headers
#  ${PARAMS_TARGET}_GEN_BINDING_STUB_SOURCES
#     A list with generated stub sources
#  ${PARAMS_TARGET}_GEN_BINDING_PROXY_HEADERS
#     A list with generated proxy headers
#  ${PARAMS_TARGET}_GEN_BINDING_PROXY_SOURCES
#     A list with generated proxy sources
#  ${PARAMS_TARGET}_GEN_BINDING_DEPLOYMENT_HEADERS
#     A list with generated deplyment headers
#  ${PARAMS_TARGET}_GEN_BINDING_DEPLOYMENT_SOURCES
#     A list with generated deplyment sources
#  ${PARAMS_TARGET}_GEN_COMMON_HEADERS
#     A list with generated common headerss
#  ${PARAMS_TARGET}_GEN_COMMON_SOURCES
#     A list with generated common sources



include(CMakeParseArguments)

IF(NOT WITH_CAPI_GENERATOR_IF_AVAILABLE)
    SET(WITH_CAPI_GENERATOR_IF_AVAILABLE ON CACHE INTERNAL "hide this!" FORCE)
ENDIF(NOT WITH_CAPI_GENERATOR_IF_AVAILABLE)

IF(NOT SELECTED_CAPI_BINDING)
    SET(SELECTED_CAPI_BINDING 0 CACHE INTERNAL "hide this!" FORCE)
ENDIF(NOT SELECTED_CAPI_BINDING)    

# find the proper libs ...
MACRO(LOAD_COMMONAPI_LIB)
	FIND_PACKAGE(CommonAPI REQUIRED)
	FIND_LIBRARY(CommonAPI_LIBRARY 
	             REQUIRED
	             NAMES CommonAPI
	             PATHS
	             "/usr/local/lib"
	             "/usr/lib"
	            )  
	message(STATUS "CommonAPI Version: ${CommonAPI_VERSION}")
	string(REPLACE "." "" COMMONAPI_VERSION_NUMBER ${CommonAPI_VERSION})
	SET(COMMONAPI_VERSION_NUMBER ${COMMONAPI_VERSION_NUMBER} CACHE INTERNAL "hide this!" FORCE)
	SET(CommonAPI_VERSION ${CommonAPI_VERSION} PARENT_SCOPE)   
ENDMACRO()

MACRO(LOAD_COMMONAPI_DBUS_LIB)
    pkg_check_modules (DBUS "dbus-1 >= 1.4" REQUIRED)
    FIND_PACKAGE(CommonAPI-DBus REQUIRED)
    FIND_LIBRARY(CommonAPI-DBus_LIBRARY 
                 REQUIRED
                 NAMES CommonAPI-DBus
                 PATHS
                 "/usr/local/lib"
                 "/usr/lib"
                ) 
    message(STATUS "CommonAPI-DBus Version: ${CommonAPI-DBus_VERSION}")
    string(REPLACE "." "" COMMONAPI_DBUS_VERSION_NUMBER ${CommonAPI-DBus_VERSION})
    SET(COMMONAPI_DBUS_VERSION_NUMBER ${COMMONAPI_DBUS_VERSION_NUMBER} CACHE INTERNAL "hide this!" FORCE)
    SET(CommonAPI-DBus_VERSION ${CommonAPI-DBus_VERSION} PARENT_SCOPE)
ENDMACRO()

MACRO(LOAD_COMMONAPI_SOMEIP_LIB)
    FIND_PACKAGE (vsomeip REQUIRED)
    FIND_LIBRARY(vsomeip_LIBRARY 
                 REQUIRED
                 NAMES vsomeip
                 PATHS
                 "/usr/local/lib"
                 "/usr/lib"
                )    
    FIND_PACKAGE(CommonAPI-SomeIP REQUIRED)
    FIND_LIBRARY(CommonAPI-SomeIP_LIBRARY 
                 REQUIRED
                 NAMES CommonAPI-SomeIP
                 PATHS
                 "/usr/local/lib"
                 "/usr/lib"
                )        
    message(STATUS "CommonAPI-SomeIP Version: ${CommonAPI-SomeIP_VERSION}")
    string(REPLACE "." "" COMMONAPI_SOMEIP_VERSION_NUMBER ${CommonAPI-SomeIP_VERSION})
    SET(COMMONAPI_SOMEIP_VERSION_NUMBER ${COMMONAPI_SOMEIP_VERSION_NUMBER} CACHE INTERNAL "hide this!" FORCE)
    SET(CommonAPI-SomeIP_VERSION ${CommonAPI-SomeIP_VERSION} PARENT_SCOPE)
ENDMACRO()

MACRO(LOAD_COMMONAPI)
    #parse the input parameters
    set(options DBUS SOMEIP)
    set(oneValueArgs "")
    set(multiValueArgs "")
    cmake_parse_arguments(PARAMS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(PARAMS_DBUS)
        SET(SELECTED_CAPI_BINDING 0 CACHE INTERNAL "hide this!" FORCE)
    elseif(PARAMS_SOMEIP)
        SET(SELECTED_CAPI_BINDING 1 CACHE INTERNAL "hide this!" FORCE)
    endif()
    
    # load the proper libs ...
    LOAD_COMMONAPI_LIB()
    IF(${SELECTED_CAPI_BINDING} EQUAL 1)
	    LOAD_COMMONAPI_SOMEIP_LIB()
    ELSE()
		LOAD_COMMONAPI_DBUS_LIB()
    ENDIF()
    
ENDMACRO()

# helper function giving a string with the current architecture
function(GET_TARGET_ARCH OUT_ARCH)  
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8) 
        SET(${OUT_ARCH} "x86_64" PARENT_SCOPE)
    ELSE() 
        SET(${OUT_ARCH} "x86" PARENT_SCOPE)
    ENDIF()  
endfunction()

# helper function giving a string with the current host
function(GET_TARGET_HOST OUT_HOST)
    IF(CMAKE_HOST_WIN32)
        SET(${OUT_HOST} "windows" PARENT_SCOPE) 
    ELSE()#CMAKE_HOST_UNIX
        SET(${OUT_HOST} "linux" PARENT_SCOPE)
    ENDIF()
endfunction()

# get lists with headers and sources after they has been generated
macro(GET_GENERATED_FILES GEN_DESTINATION)
        
    IF(${SELECTED_CAPI_BINDING} EQUAL 1)
        SET(BINDING_SUFFIX "SomeIP")
    ELSE()
        SET(BINDING_SUFFIX "DBus")
    ENDIF()
       
  execute_process(COMMAND find ${GEN_DESTINATION} -type f -name "*${BINDING_SUFFIX}Stub*.hpp"
            RESULT_VARIABLE EXIT_CODE
            OUTPUT_VARIABLE _FIND_RESULT
            ERROR_VARIABLE _FIND_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE)

    IF(_FIND_ERROR)
        MESSAGE(FATAL_ERROR "...an error occurred: ${_FIND_ERROR}")
    ELSE()
        IF(NOT _FIND_RESULT)
            MESSAGE(STATUS "hint...can't find any binding stub headers!")
        ELSE()
             string(REPLACE "\n" ";" CAPI_BINDING_STUB_HEADERS ${_FIND_RESULT})   
        ENDIF()
    ENDIF()
        
    execute_process(COMMAND find ${GEN_DESTINATION} -type f -name "*${BINDING_SUFFIX}Proxy*.hpp"
                RESULT_VARIABLE EXIT_CODE
                OUTPUT_VARIABLE _FIND_RESULT
                ERROR_VARIABLE _FIND_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE)

    IF(_FIND_ERROR)
        MESSAGE(FATAL_ERROR "...an error occurred: ${_FIND_ERROR}")
    ELSE()
        IF(NOT _FIND_RESULT)
            MESSAGE(STATUS "hint...can't find any binding proxy headers!")
        ELSE()
            string(REPLACE "\n" ";" CAPI_BINDING_PROXY_HEADERS ${_FIND_RESULT})     
        ENDIF()
    ENDIF()
    
    execute_process(COMMAND find ${GEN_DESTINATION} -type f -name "*${BINDING_SUFFIX}Deployment.hpp"
            RESULT_VARIABLE EXIT_CODE
            OUTPUT_VARIABLE _FIND_RESULT
            ERROR_VARIABLE _FIND_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE)

    IF(_FIND_ERROR)
        MESSAGE(FATAL_ERROR "...an error occurred: ${_FIND_ERROR}")
    ELSE() 
        IF(NOT _FIND_RESULT)
            MESSAGE(STATUS "hint...can't find any binding deployment headers!")
        ELSE()
            string(REPLACE "\n" ";" CAPI_BINDING_DEPLOYMENT_HEADERS ${_FIND_RESULT})    
        ENDIF()   
    ENDIF()

    execute_process(COMMAND find ${GEN_DESTINATION} -type f -name *.hpp -and -not -name "*${BINDING_SUFFIX}Proxy*.hpp" -and -not -name "*${BINDING_SUFFIX}Stub*.hpp" -and -not -name "*${BINDING_SUFFIX}Deployment.hpp"
                RESULT_VARIABLE EXIT_CODE
                OUTPUT_VARIABLE _FIND_RESULT
                ERROR_VARIABLE _FIND_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE)

    IF(_FIND_ERROR)
        MESSAGE(FATAL_ERROR "...an error occurred: ${_FIND_ERROR}")
    ELSE()
        IF(NOT _FIND_RESULT)
            MESSAGE(STATUS "hint...can't any find generic headers!")
        ELSE()
            string(REPLACE "\n" ";" CAPI_COMMON_HEADERS ${_FIND_RESULT}) 
        ENDIF()
    ENDIF()
    
     execute_process(COMMAND find ${GEN_DESTINATION} -type f -name "*${BINDING_SUFFIX}Stub*.cpp"
            RESULT_VARIABLE EXIT_CODE
            OUTPUT_VARIABLE _FIND_RESULT
            ERROR_VARIABLE _FIND_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE)

    IF(_FIND_ERROR)
        MESSAGE(FATAL_ERROR "...an error occurred: ${_FIND_ERROR}")
    ELSE()
        IF(NOT _FIND_RESULT)
            MESSAGE(STATUS "hint...can't find any binding stub sources!")
        ELSE()
            string(REPLACE "\n" ";" CAPI_BINDING_STUB_SOURCES ${_FIND_RESULT})       
        ENDIF()
    ENDIF()
      
    execute_process(COMMAND find ${GEN_DESTINATION} -type f -name "*${BINDING_SUFFIX}Proxy*.cpp"
                RESULT_VARIABLE EXIT_CODE
                OUTPUT_VARIABLE _FIND_RESULT
                ERROR_VARIABLE _FIND_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE)

    IF(_FIND_ERROR)
        MESSAGE(FATAL_ERROR "...an error occurred: ${_FIND_ERROR}")
    ELSE()
        IF(NOT _FIND_RESULT)
            MESSAGE(STATUS "hint...can't find any binding proxy sources!")
        ELSE()    
            string(REPLACE "\n" ";" CAPI_BINDING_PROXY_SOURCES ${_FIND_RESULT}) 
        ENDIF()
    ENDIF()
    
    execute_process(COMMAND find ${GEN_DESTINATION} -type f -name "*${BINDING_SUFFIX}Deployment.cpp"
            RESULT_VARIABLE EXIT_CODE
            OUTPUT_VARIABLE _FIND_RESULT
            ERROR_VARIABLE _FIND_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE)

    IF(_FIND_ERROR)
        MESSAGE(FATAL_ERROR "...an error occurred: ${_FIND_ERROR}")
    ELSE()
        IF(NOT _FIND_RESULT)
            MESSAGE(STATUS "hint...can't find any binding deployment sources!")
        ELSE()
            string(REPLACE "\n" ";" CAPI_BINDING_DEPLOYMENT_SOURCES ${_FIND_RESULT})
        ENDIF()
    ENDIF()
    execute_process(COMMAND find ${GEN_DESTINATION} -type f -name *.cpp -and -not -name "*${BINDING_SUFFIX}Proxy*.cpp" -and -not -name "*${BINDING_SUFFIX}Stub*.cpp" -and -not -name "*${BINDING_SUFFIX}Deployment.cpp"
                RESULT_VARIABLE EXIT_CODE
                OUTPUT_VARIABLE _FIND_RESULT
                ERROR_VARIABLE _FIND_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE)

    IF(_FIND_ERROR)
        MESSAGE(FATAL_ERROR "...an error occurred: ${_FIND_ERROR}")
    ELSE()
        IF(NOT _FIND_RESULT)
            MESSAGE(STATUS "hint...can't find any generic sources!")
        ELSE()
             string(REPLACE "\n" ";" CAPI_COMMON_SOURCES ${_FIND_RESULT}) 
        ENDIF()
    ENDIF()
    
   set(${PARAMS_TARGET}_GEN_BINDING_DEPLOYMENT_HEADERS ${${PARAMS_TARGET}_GEN_BINDING_DEPLOYMENT_HEADERS} ${CAPI_BINDING_DEPLOYMENT_HEADERS} PARENT_SCOPE)
   set(${PARAMS_TARGET}_GEN_BINDING_STUB_HEADERS ${${PARAMS_TARGET}_GEN_BINDING_STUB_HEADERS} ${CAPI_BINDING_STUB_HEADERS} PARENT_SCOPE)
   set(${PARAMS_TARGET}_GEN_BINDING_PROXY_HEADERS ${${PARAMS_TARGET}_GEN_BINDING_PROXY_HEADERS} ${CAPI_BINDING_PROXY_HEADERS} PARENT_SCOPE)
   
   set(${PARAMS_TARGET}_GEN_BINDING_DEPLOYMENT_SOURCES ${${PARAMS_TARGET}_GEN_BINDING_DEPLOYMENT_SOURCES} ${CAPI_BINDING_DEPLOYMENT_SOURCES} PARENT_SCOPE)
   set(${PARAMS_TARGET}_GEN_BINDING_STUB_SOURCES ${${PARAMS_TARGET}_GEN_BINDING_STUB_SOURCES} ${CAPI_BINDING_STUB_SOURCES} PARENT_SCOPE)
   set(${PARAMS_TARGET}_GEN_BINDING_PROXY_SOURCES ${${PARAMS_TARGET}_GEN_BINDING_PROXY_SOURCES} ${CAPI_BINDING_PROXY_SOURCES} PARENT_SCOPE)
   
   set(${PARAMS_TARGET}_GEN_COMMON_HEADERS ${${PARAMS_TARGET}_GEN_COMMON_HEADERS} ${CAPI_COMMON_HEADERS} PARENT_SCOPE)
   set(${PARAMS_TARGET}_GEN_COMMON_SOURCES ${${PARAMS_TARGET}_GEN_COMMON_SOURCES} ${CAPI_COMMON_SOURCES} PARENT_SCOPE)         

  set(${PARAMS_TARGET}_GEN_HEADERS ${${PARAMS_TARGET}_GEN_HEADERS} ${CAPI_BINDING_DEPLOYMENT_HEADERS} ${CAPI_BINDING_STUB_HEADERS} ${CAPI_BINDING_PROXY_HEADERS} ${CAPI_COMMON_HEADERS} PARENT_SCOPE)
  set(${PARAMS_TARGET}_GEN_SOURCES ${${PARAMS_TARGET}_GEN_SOURCES} ${CAPI_BINDING_DEPLOYMENT_SOURCES} ${CAPI_BINDING_STUB_SOURCES} ${CAPI_BINDING_PROXY_SOURCES} ${CAPI_COMMON_SOURCES} PARENT_SCOPE)
                    
endmacro(GET_GENERATED_FILES)

macro(FIND_AND_EXEC_GENERATOR GENERATOR_EXECUTABLE SHOULD_GENERATE_STUB_DEFAULT FIDLS)
    MESSAGE(STATUS "Searching for common-api generator executable ${GENERATOR_EXECUTABLE} ...")
    # find the generator binary ...
    execute_process(COMMAND find "/usr/local/share/CommonAPI-${CommonAPI_VERSION}" -name ${GENERATOR_EXECUTABLE}
                        RESULT_VARIABLE EXIT_CODE
                        OUTPUT_VARIABLE OUT_RESULT
                        ERROR_VARIABLE OUT_ERROR
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_STRIP_TRAILING_WHITESPACE)
	
    IF(OUT_ERROR)
         # in case of error just try to find the sources in the alternative folder
        SET(TEMP_GEN_DST ${PARAMS_ALT_DESTINATION})
        message(STATUS "Common-api generator error / ${OUT_ERROR} /. Will try the alternative folder!")
    ELSEIF(NOT OUT_RESULT)
        # in case of error just try to find the sources in the alternative folder
        SET(TEMP_GEN_DST ${PARAMS_ALT_DESTINATION})
        message(STATUS "Common-api generator can't be found. Will try the alternative folder!")
    ELSE()
        # the generator binary is found
        MESSAGE(STATUS "Will execute common-api generator at path ${OUT_RESULT} with fidls:")
        FOREACH(FIDL ${FIDLS})
    	    MESSAGE(STATUS "${FIDL}")
        ENDFOREACH()
     
        function(mktmpdir OUTVAR)
            while(NOT TEMP_DESTINATION OR EXISTS ${TEMP_DESTINATION})
                string(RANDOM LENGTH 16 TEMP_DESTINATION)
                set(TEMP_DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${TEMP_DESTINATION}")
            endwhile()

           file(MAKE_DIRECTORY ${TEMP_DESTINATION})

           set(${OUTVAR} ${TEMP_DESTINATION} PARENT_SCOPE)
        endfunction()
        # execute the generate command ...             
        IF(${SHOULD_GENERATE_STUB_DEFAULT} EQUAL 1)  
            execute_process(COMMAND ${OUT_RESULT} -sk Default -d ${PARAMS_DESTINATION} ${FIDLS}
	                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	                        RESULT_VARIABLE EXIT_CODE
	                        OUTPUT_VARIABLE GENERATOR_OUTPUT
	                        ERROR_VARIABLE GENERATOR_ERR_OUTPUT
	                        OUTPUT_STRIP_TRAILING_WHITESPACE
	                        ERROR_STRIP_TRAILING_WHITESPACE)	                        
        ELSE()
      		execute_process(COMMAND ${OUT_RESULT} -d ${PARAMS_DESTINATION} ${FIDLS}
	                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	                        RESULT_VARIABLE EXIT_CODE
	                        OUTPUT_VARIABLE GENERATOR_OUTPUT
	                        ERROR_VARIABLE GENERATOR_ERR_OUTPUT
	                        OUTPUT_STRIP_TRAILING_WHITESPACE
	                        ERROR_STRIP_TRAILING_WHITESPACE)
        ENDIF()                
        if(EXIT_CODE)
            message(FATAL_ERROR "Failed to generate files from FIDL:${GENERATOR_OUTPUT}")
        elseif(GENERATOR_ERR_OUTPUT)
        	message(FATAL_ERROR "Common-API generator error:${GENERATOR_ERR_OUTPUT}")
        endif()
        SET(TEMP_GEN_DST ${PARAMS_DESTINATION})                
    ENDIF()
endmacro(FIND_AND_EXEC_GENERATOR GENERATOR_EXECUTABLE SHOULD_GENERATE_STUB_DEFAULT FIDLS)

# generate common-api sources and retreive a list with them 
MACRO(EXECUTE_GENERATOR)    
    # construct the generator binary name...
    GET_TARGET_HOST(_TARGET_HOST)
    GET_TARGET_ARCH(_TARGET_ARCH)
    SET(COMMONAPI_GENERATOR_EXECUTABLE commonapi-generator-${_TARGET_HOST}-${_TARGET_ARCH})
    IF(${SELECTED_CAPI_BINDING} EQUAL 1)
        SET(COMMONAPI_BINDING_GENERATOR_EXECUTABLE commonapi-someip-generator-${_TARGET_HOST}-${_TARGET_ARCH})
    ELSE()
        SET(COMMONAPI_BINDING_GENERATOR_EXECUTABLE commonapi-dbus-generator-${_TARGET_HOST}-${_TARGET_ARCH})
    ENDIF()
    # prepare an additional compatibilty flag for generators prior 3.x.x ...
    IF(${COMMONAPI_VERSION_NUMBER} GREATER 300 OR ${COMMONAPI_VERSION_NUMBER} EQUAL 300)
        # >= 3.x.x the stubs are generated by the binding generator
       	SET(GENERATE_STUB 1)         
    ELSE() 
        # < 3.0.0 the stubs are generated by the genric generator
    	SET(GENERATE_STUB 0) 
    ENDIF()
    SET(${PARAMS_TARGET}_GEN_INCLUDE_DIR ${PARAMS_DESTINATION} PARENT_SCOPE)
       
    # searching for common-api-generator executable ...
    FIND_AND_EXEC_GENERATOR(${COMMONAPI_GENERATOR_EXECUTABLE} ${GENERATE_STUB} "${IN_FIDLS_GENERIC}")
    SET(GENERIC_DST ${TEMP_GEN_DST})
    FIND_AND_EXEC_GENERATOR(${COMMONAPI_BINDING_GENERATOR_EXECUTABLE} FALSE "${IN_FIDLS_BINDING}")
    SET(BINDING_DST ${TEMP_GEN_DST})
    # get the lists with the sources and headers
    message(STATUS "Looking for generated common-api files...")
    IF(${GENERIC_DST} STREQUAL ${BINDING_DST})
        GET_GENERATED_FILES(${GENERIC_DST})
    ELSE()
       GET_GENERATED_FILES(${BINDING_DST})
       GET_GENERATED_FILES(${GENERIC_DST})
    ENDIF()
    
ENDMACRO(EXECUTE_GENERATOR)

# Function COMMON_API_GENERATE_SOUCRES 
#
# TARGET COMMON_API 
# FIDLS_GENERIC a list with fidls for the generic generator.
# FIDLS_BINDING a list with fidls for the binding generator.
# DESTINATION a relative path to the build directory or an absolute path.                                
# ALT_DESTINATION an alternative relative/absolute path with common-api sources, usually in the source tree.
FUNCTION(COMMON_API_GENERATE_SOURCES)
    #parse the input parameters
    set(options "")
    set(oneValueArgs TARGET DESTINATION ALT_DESTINATION HEADER_TEMPLATE)
    set(multiValueArgs FIDLS_GENERIC FIDLS_BINDING)

    cmake_parse_arguments(PARAMS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(NOT PARAMS_TARGET)
           message(FATAL_ERROR "TARGET must be specified")
    endif()
    
    if(NOT IS_ABSOLUTE ${PARAMS_DESTINATION})
         set(PARAMS_DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${PARAMS_DESTINATION})
    endif()
    
    if(NOT IS_ABSOLUTE ${PARAMS_ALT_DESTINATION})
         set(PARAMS_DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${PARAMS_ALT_DESTINATION})
    endif()
    # check the cmake option, whether to use the generator or not ...
    IF(NOT WITH_CAPI_GENERATOR_IF_AVAILABLE)
        message(STATUS "Looking for generated common-api files...")
        # check which of the given folders exists and get it as destination
        IF(EXISTS ${PARAMS_DESTINATION})
            GET_GENERATED_FILES(${PARAMS_DESTINATION})
        ELSE()
            GET_GENERATED_FILES(${PARAMS_ALT_DESTINATION})
        ENDIF()    
    ELSE()
        if(NOT PARAMS_FIDLS_GENERIC)
            message(STATUS "No fidls for the generic generator. The generation of generic code will be skipped !")
        endif()
        
        if(NOT PARAMS_FIDLS_BINDING)
            message(STATUS "No fidls for the binding generator. The generation of binding code will be skipped !")
        endif()
    
        if(PARAMS_HEADER_TEMPLATE)
            list(APPEND ARGS -pref ${PARAMS_HEADER_TEMPLATE})
        endif()
        
        # Run configure_file on each .fidl which forces cmake to reexecute its configure phase if the input file changes.
        foreach(FIDL ${PARAMS_FIDLS_GENERIC})
            get_filename_component(FIDL_PATH ${FIDL} ABSOLUTE)
#            string(MD5 FIDL_CHECKSUM ${FIDL_PATH}) 
#            configure_file(${FIDL_PATH} ${CMAKE_CURRENT_BINARY_DIR}/${FIDL_CHECKSUM}.fidl.done)
            list(APPEND IN_FIDLS_GENERIC ${FIDL_PATH})
        endforeach()

        if(PARAMS_FIDLS_BINDING)
            foreach(FIDL ${PARAMS_FIDLS_BINDING})
                get_filename_component(FIDL_PATH ${FIDL} ABSOLUTE)
#                string(MD5 FIDL_CHECKSUM ${FIDL_PATH})              
#                configure_file(${FIDL_PATH} ${CMAKE_CURRENT_BINARY_DIR}/${FIDL_CHECKSUM}.fidl.done)
                list(APPEND IN_FIDLS_BINDING ${FIDL_PATH})
            endforeach()
        endif()
        
        # run the generator ...
        EXECUTE_GENERATOR()
    ENDIF()
ENDFUNCTION()

