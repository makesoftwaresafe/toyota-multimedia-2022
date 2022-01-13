file(GLOB iap1_dlt_log_lib_src *.[hc])

add_library(${IAP1_DLT_LOGGING_COMP_NAME} SHARED ${iap1_dlt_log_lib_src})

target_link_libraries(${IAP1_DLT_LOGGING_COMP_NAME} PUBLIC
		${DLT_LIBRARIES}
		ipod-dlt_logging)

if (WITH_IPOD_IAP1)
install(TARGETS ${IAP1_DLT_LOGGING_COMP_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES
	iap1_dlt_log.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
endif()