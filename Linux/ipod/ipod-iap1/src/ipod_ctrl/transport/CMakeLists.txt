file(GLOB ipod_trans_src *.[ch])

add_library(${TRANS_NAME} SHARED ${ipod_trans_src})

target_link_libraries(${TRANS_NAME} PUBLIC
		${ADIT_PFCFG_SYS_LIBRARIES}
		pthread
		rt
		ipod-auth
		ipod-iap2-dlt_logging
		ipod-dlt_logging
		${IAP1_DLT_LOGGING_COMP_NAME}
		${PLUG_NAME_BTCOM}
		${PLUG_NAME_USB_HIDAPI})

if (WITH_IPOD_IAP1)
install(TARGETS ${TRANS_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES IPOD_CTRL.cfg
	DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/pfcfg
	COMPONENT runtime)

install(FILES
	iPodDataCom.h
	iap_transport_message.h
	iap_transport_os.h
	iap_transport_configuration.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
endif()