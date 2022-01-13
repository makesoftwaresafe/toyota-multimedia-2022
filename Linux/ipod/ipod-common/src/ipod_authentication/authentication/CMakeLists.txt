file(GLOB authentication_lib_src *.[hc])

add_library(${AUTH_NAME} SHARED ${authentication_lib_src})

target_link_libraries(${AUTH_NAME} PUBLIC
		${IAP2_DLT_LOGGING_COMP_NAME}
		${IPOD_DLT_LOGGING_COMP_NAME}
		${PLUG_NAME_AUTHCOM}
		${ADIT_PFCFG_SYS_LIBRARIES}
		pthread)

install(TARGETS ${AUTH_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES authentication.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)