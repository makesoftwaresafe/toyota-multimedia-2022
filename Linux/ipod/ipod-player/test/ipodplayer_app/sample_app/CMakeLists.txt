file(GLOB ipod_sample_app_src *.[ch])

add_executable(${IPOD_SAMPLE_APP} ${ipod_sample_app_src})

target_link_libraries(${IPOD_SAMPLE_APP} PUBLIC
		rt
		dlt
		pthread
		${LIBUSB_LIBRARIES}
		${UTIL_NAME}
		${IPC_NAME}
		${IF_NAME})

install(TARGETS ${IPOD_SAMPLE_APP}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
	COMPONENT runtime)

configure_file(
	ipod-player-smoketest.sh.in
	${PROJECT_BINARY_DIR}/ipod-player-smoketest.sh
	@ONLY)

install(FILES ${PROJECT_BINARY_DIR}/ipod-player-smoketest.sh
	DESTINATION /opt/platform/
	PERMISSIONS
		OWNER_WRITE OWNER_READ OWNER_EXECUTE
		GROUP_READ  GROUP_EXECUTE
		WORLD_READ  WORLD_EXECUTE
	COMPONENT runtime)

configure_file(
	ipod-audio-test.sh.in
	${PROJECT_BINARY_DIR}/ipod-audio-test.sh
	@ONLY)

install(FILES ${PROJECT_BINARY_DIR}/ipod-audio-test.sh
	DESTINATION /opt/platform/
	PERMISSIONS
		OWNER_WRITE OWNER_READ OWNER_EXECUTE
		GROUP_READ  GROUP_EXECUTE
		WORLD_READ  WORLD_EXECUTE
	COMPONENT runtime)

install(FILES ipod-smoketest_01
	DESTINATION ${DEST_SMOKETEST_CMD_DIR}
	PERMISSIONS
		OWNER_WRITE OWNER_READ OWNER_EXECUTE
		GROUP_READ  GROUP_EXECUTE
		WORLD_READ  WORLD_EXECUTE
	COMPONENT runtime)

configure_file(
	ipod-smoketest_01.sh.in
	${PROJECT_BINARY_DIR}/ipod-smoketest_01.sh
	@ONLY)

install(FILES ${PROJECT_BINARY_DIR}/ipod-smoketest_01.sh
	DESTINATION ${DEST_SMOKETEST_BIN_DIR}
	PERMISSIONS
		OWNER_WRITE OWNER_READ OWNER_EXECUTE
		GROUP_READ  GROUP_EXECUTE
		WORLD_READ  WORLD_EXECUTE
	COMPONENT runtime)

configure_file(
	run_ipod-smoketest.sh.in
	${PROJECT_BINARY_DIR}/run_ipod-smoketest.sh
	@ONLY)

install(FILES ${PROJECT_BINARY_DIR}/run_ipod-smoketest.sh
	DESTINATION ${DEST_SMOKETEST_SCRIPT_DIR}
	PERMISSIONS
		OWNER_WRITE OWNER_READ OWNER_EXECUTE
		GROUP_READ  GROUP_EXECUTE
		WORLD_READ  WORLD_EXECUTE
	COMPONENT runtime)