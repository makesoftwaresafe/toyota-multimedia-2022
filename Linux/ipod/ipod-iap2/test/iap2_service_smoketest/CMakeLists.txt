file(GLOB iap2_service_test_src *.[ch])

add_executable(${IAP2_SERVICE_SMOKETEST} ${iap2_service_test_src})

target_link_libraries(${IAP2_SERVICE_SMOKETEST} PUBLIC
	udev
	${ADIT_PFCFG_SYS_LIBRARIES}
	rt
	pthread
	kmod
	usbg
	ipod-iap2-interface
	ipod-iap2-linklayer
	ipod-iap2-session
	ipod-iap2-transport
	ipod-iap2-ffs-gadget
	ipod-iap2_usb_device_mode_plugin
	ipod-iap2_usb_multi_host_mode_plugin
	ipod-iap2_bluetooth_plugin
	ipod-iap2_usb_host_mode_plugin
	ipod-iap2-usb-gadget
	ipod-iap2_over_carplay_plugin
	gobject-2.0
	gmodule-2.0
	glib-2.0
	${IAP2_TEST_UTILITY}
	${ADIT_IPOD_COMMON_LIBRARIES}
	${LIBXML2_LIBRARIES}
	${LIBUSB_LIBRARIES}
	${GSTREAMER_LIBRARIES})

install(TARGETS ${IAP2_SERVICE_SMOKETEST}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
	COMPONENT runtime)

install(FILES
		ipod-iap2_service-gadget-smoketest.sh
		ipod-iap2_service-host-smoketest.sh
		ipod-iap2_service-smoketest.sh
		ipod-iap2_service-multihost-smoketest.sh
		DESTINATION /opt/platform/
		PERMISSIONS
			OWNER_WRITE OWNER_READ OWNER_EXECUTE
			GROUP_READ  GROUP_EXECUTE
			WORLD_READ  WORLD_EXECUTE
		COMPONENT runtime)
