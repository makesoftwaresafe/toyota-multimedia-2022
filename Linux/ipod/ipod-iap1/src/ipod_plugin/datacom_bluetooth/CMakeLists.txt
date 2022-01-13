file(GLOB bt_com_src *.[c])

add_library(${PLUG_NAME_BTCOM} SHARED ${bt_com_src})

target_link_libraries(${PLUG_NAME_BTCOM} PUBLIC
		ipod-dlt_logging
		ipod-iap2-dlt_logging
		${IAP1_DLT_LOGGING_COMP_NAME})

if (WITH_IPOD_IAP1)
install(TARGETS ${PLUG_NAME_BTCOM}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)
endif()
