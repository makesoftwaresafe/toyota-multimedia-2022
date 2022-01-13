file(GLOB transport_lib_src *.[ch])

add_library(${IAP2_TRANSPORT_COMP_NAME} SHARED ${transport_lib_src})

target_link_libraries(${IAP2_TRANSPORT_COMP_NAME} PUBLIC
			ipod-iap2-dlt_logging
			ipod-dlt_logging
			${IPOD_IAP2_BT_PLUGIN}
			${IPOD_IAP2_USB_HOST_MODE_PLUGIN}
			${IPOD_IAP2_USB_DEV_MODE_PLUGIN}
			${IPOD_IAP2_USB_MULTI_HOST_MODE_PLUGIN}
			${IPOD_IAP2_OVER_CARPLAY_PLUGIN}
			${IAP2_LINK_COMP_NAME})

if (WITH_IPOD_IAP2)
install(TARGETS ${IAP2_TRANSPORT_COMP_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES iap2_datacom.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
endif()
