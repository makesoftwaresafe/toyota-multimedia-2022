file(GLOB iap2_test_utility_lib_src *.[ch])

add_library(${IAP2_TEST_UTILITY} SHARED ${iap2_test_utility_lib_src})

target_link_libraries(${IAP2_TEST_UTILITY} PUBLIC
		kmod
		usbg
		udev
		rt
		usb-1.0
		glib-2.0
		gobject-2.0
		pthread
		${ADIT_PFCFG_SYS_LIBRARIES}
		${ADIT_IPOD_DLT_LOG_LIBRARIES}
		${ADIT_IPOD_IAP2_DLT_LOG_LIBRARIES}
		${ADIT_IPOD_IAP2_USB_ROLE_SWITCH_LIBRARIES}
		${ADIT_IPOD_IAP2_SESSION_LIBRARIES}
		${ADIT_IPOD_IAP2_INTF_LIBRARIES}
		asound
		${GSTREAMER_LIBRARIES})

install(TARGETS ${IAP2_TEST_UTILITY}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		COMPONENT runtime)
