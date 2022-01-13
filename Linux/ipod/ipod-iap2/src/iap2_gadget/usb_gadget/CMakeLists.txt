file(GLOB iap2_usb_gadget_lib_src *.[hc])

add_library(${IAP2_USB_GADGET_COMP_NAME} SHARED ${iap2_usb_gadget_lib_src})

target_link_libraries(${IAP2_USB_GADGET_COMP_NAME} PUBLIC
		usbg
		ipod-dlt_logging
		ipod-iap2-dlt_logging
		${IAP2_FFS_GADGET_COMP_NAME})

if (WITH_IPOD_IAP2)
install(TARGETS ${IAP2_USB_GADGET_COMP_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES iap2_initialize_usb_gadget.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
endif()