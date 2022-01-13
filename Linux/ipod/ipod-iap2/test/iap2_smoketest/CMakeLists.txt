file(GLOB iap2_smoketest_src *.[ch])

add_executable(${IAP2_SMOKETEST} "${iap2_smoketest_src}")

target_link_libraries(${IAP2_SMOKETEST} PUBLIC
		usb-1.0
		udev
		${ADIT_PFCFG_SYS_LIBRARIES}
		xml2
		pthread
		glib-2.0
		gmodule-2.0
		gobject-2.0
		kmod
		rt
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
		${IAP2_TEST_UTILITY}
		${ADIT_IPOD_COMMON_LIBRARIES}
		${GSTREAMER_LIBRARIES}
		)

install(TARGETS ${IAP2_SMOKETEST}
		DESTINATION ${CMAKE_INSTALL_BINDIR}
		COMPONENT runtime)

install(FILES iap2_bluetooth.service
		DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR}
		PERMISSIONS
					OWNER_WRITE OWNER_READ OWNER_EXECUTE
					GROUP_READ  GROUP_EXECUTE
					WORLD_READ  WORLD_EXECUTE
		COMPONENT runtime)

install(FILES
		ipod-iap2-gadget-smoketest.sh
		ipod-iap2-host-smoketest.sh
		ipod-iap2-multi-host-smoketest.sh
		ipod-iap2-smoketest.sh
		../ipod-auth.func
		DESTINATION /opt/platform/
		PERMISSIONS
					OWNER_WRITE OWNER_READ OWNER_EXECUTE
					GROUP_READ  GROUP_EXECUTE
					WORLD_READ  WORLD_EXECUTE
		COMPONENT runtime)
