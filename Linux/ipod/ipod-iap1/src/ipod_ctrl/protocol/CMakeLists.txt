file(GLOB prot_lib_src *.[ch])

add_library(${PROT_NAME} SHARED ${prot_lib_src})

target_link_libraries(${PROT_NAME} PUBLIC
		ipod-auth
		ipod-auth-com
		ipod-iap2-dlt_logging
		${TRANS_NAME}
		${IAP1_DLT_LOGGING_COMP_NAME})

if (WITH_IPOD_IAP1)
install(TARGETS ${PROT_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES
		iap_callback.h
		iap_commands.h
		iap_common.h
		iap_database.h
		iap_digitalaudio.h
		iap_display.h
		iap_displayremote.h
		iap_general.h
		iap_init.h
		iap_ipodout.h
		iap_playback.h
		iap_storage.h
		iap_types.h
		iap_devconf.h
		ipodcommon.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
endif()