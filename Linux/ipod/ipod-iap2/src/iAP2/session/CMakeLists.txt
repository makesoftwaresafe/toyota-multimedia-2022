file(GLOB session_lib_src *.[ch])

add_library(${IAP2_SESSION_COMP_NAME}  SHARED ${session_lib_src})

target_link_libraries(${IAP2_SESSION_COMP_NAME}  PUBLIC
	ipod-dlt_logging
	ipod-iap2-dlt_logging
	ipod-auth
	${IAP2_LINK_COMP_NAME})

if (WITH_IPOD_IAP2)
install(TARGETS ${IAP2_SESSION_COMP_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)
endif()