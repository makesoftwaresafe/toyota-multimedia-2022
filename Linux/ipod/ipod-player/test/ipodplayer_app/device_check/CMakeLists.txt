file(GLOB ipod_player_dev_check_test_src *.[ch])

add_executable(${IPOD_DEVICE_CHECK} ${ipod_player_dev_check_test_src})

target_link_libraries(${IPOD_DEVICE_CHECK} PUBLIC
		udev
		ipod-dlt_logging
		ipod-iap2-dlt_logging
		ipod-iap2-usb-role-switch
		ipod-iap1-dlt_logging
		${LIBUSB_LIBRARIES}
		${DLT_LIBRARIES})

install(TARGETS ${IPOD_DEVICE_CHECK}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
	COMPONENT runtime)