file(GLOB usb_device_mode_lib_src *.[ch])

add_library(${IPOD_IAP2_USB_DEV_MODE_PLUGIN} SHARED ${usb_device_mode_lib_src})

target_link_libraries(${IPOD_IAP2_USB_DEV_MODE_PLUGIN} PUBLIC
		ipod-dlt_logging
		ipod-iap2-dlt_logging
		pthread
		${LIBUSB_LIBRARIES})

if (WITH_IPOD_IAP2)
install(TARGETS ${IPOD_IAP2_USB_DEV_MODE_PLUGIN}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)
endif()