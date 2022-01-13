file(GLOB ipod_player_dev_detect_test_src *.[ch])

add_executable(${IPOD_PLAYER_DEV_DETECTION} ${ipod_player_dev_detect_test_src})

target_link_libraries(${IPOD_PLAYER_DEV_DETECTION} PUBLIC
		rt
		dlt
		udev
		pthread
		${LIBUSB_LIBRARIES}
		${UTIL_NAME}
		${IPC_NAME}
		${IF_NAME})

install(TARGETS ${IPOD_PLAYER_DEV_DETECTION}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
	COMPONENT runtime)