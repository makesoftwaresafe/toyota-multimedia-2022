file(GLOB usb_host_mode_lib_src  *.[ch])

add_library(${IAP2_CINEMO_HOST_PLUGIN} SHARED ${usb_host_mode_lib_src})

target_link_libraries(${IAP2_CINEMO_HOST_PLUGIN} PUBLIC
                      ${ADIT_IPOD_COMMON_LIBRARIES}
                      ADITAppleAudioImpl)

install(TARGETS ${IAP2_CINEMO_HOST_PLUGIN}
        DESTINATION ${CINEMO_INSTALL_LIBDIR}
        COMPONENT runtime)

install(FILES
	iap2_cinemo_host_datacom.h
	cinemo_transport.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)

