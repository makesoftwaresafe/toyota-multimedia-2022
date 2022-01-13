file(GLOB usb_role_switch_lib_src *.[ch])

add_library(${IAP2_USB_ROLE_SWITCH_COMP_NAME} SHARED ${usb_role_switch_lib_src})

target_link_libraries(${IAP2_USB_ROLE_SWITCH_COMP_NAME} PUBLIC
		kmod
		udev
		${LIBUSB_LIBRARIES}
		${IPOD_DLT_LOGGING_COMP_NAME}
		${IAP2_DLT_LOGGING_COMP_NAME})

install(TARGETS ${IAP2_USB_ROLE_SWITCH_COMP_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

file(GLOB usb_role_switch_lib_inlude *.[h])

install(FILES
	iap2_usb_gadget_load_modules.h
	iap2_usb_role_switch.h
	iap2_usb_udc.h
	iap2_defines.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)