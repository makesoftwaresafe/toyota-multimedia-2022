file(GLOB iap2_dlt_log_lib_src *.[hc])

add_library(${IAP2_DLT_LOGGING_COMP_NAME} SHARED ${iap2_dlt_log_lib_src})

target_link_libraries(${IAP2_DLT_LOGGING_COMP_NAME} PUBLIC
		${DLT_LIBRARIES}
		${IPOD_DLT_LOGGING_COMP_NAME})

install(TARGETS ${IAP2_DLT_LOGGING_COMP_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES iap2_dlt_log.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)