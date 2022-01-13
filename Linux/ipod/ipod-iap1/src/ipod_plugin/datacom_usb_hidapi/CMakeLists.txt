
file(GLOB usb_host_hidapi_src *.[c])

add_library(${PLUG_NAME_USB_HIDAPI} SHARED ${usb_host_hidapi_src})

target_link_libraries(${PLUG_NAME_USB_HIDAPI} PUBLIC
		pthread
		ipod-iap2-dlt_logging
		ipod-dlt_logging
		${LIBUSB_LIBRARIES}
		${IAP1_DLT_LOGGING_COMP_NAME})

if (WITH_IPOD_IAP1)
install(TARGETS ${PLUG_NAME_USB_HIDAPI}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)
endif()
