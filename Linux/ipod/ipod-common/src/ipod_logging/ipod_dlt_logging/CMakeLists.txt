file(GLOB ipod_dlt_log_lib_src *.[hc])

add_library(${IPOD_DLT_LOGGING_COMP_NAME} SHARED ${ipod_dlt_log_lib_src})

target_link_libraries(${IPOD_DLT_LOGGING_COMP_NAME} PUBLIC ${DLT_LIBRARIES})

install(TARGETS ${IPOD_DLT_LOGGING_COMP_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES ipod_dlt_log.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
