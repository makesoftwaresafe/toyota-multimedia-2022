configure_file(
	iPodPlayerCoreDef.h.in
	${PROJECT_BINARY_DIR}/iPodPlayerCoreDef.h
	@ONLY
	)

include_directories(
	./
	)

file(GLOB iPodPlayerCore_src *.[ch])

add_executable("${CORE_NAME}" ${iPodPlayerCore_src})

target_include_directories(${CORE_NAME} PRIVATE ${PROJECT_BINARY_DIR})

target_link_libraries(${CORE_NAME} PUBLIC
		sqlite3
		pthread
		rt
		udev
		usbg
		${LIBXML2_LIBRARIES}
		${LIBUSB_LIBRARIES}
		${DLT_LIBRARIES}
		${ADIT_IPOD_COMMON_LIBRARIES}
		${ADIT_IPOD_IAP2_LIBRARIES}
		${ADIT_IPOD_IAP1_LIBRARIES}
		${IPC_NAME}
		${UTIL_NAME})

if (WITH_IPOD_PLAYER)
install(TARGETS ${CORE_NAME}
		DESTINATION ${CMAKE_INSTALL_BINDIR}
		COMPONENT runtime)

#QNX will be installed in "gr-mrb-64" and BUILDENV_KP platform is not in use


configure_file(
	ipod-player.service.in
	${PROJECT_BINARY_DIR}/ipod-player.service
	@ONLY
	)

install(FILES ${PROJECT_BINARY_DIR}/ipod-player.service
	DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR}
	PERMISSIONS
		OWNER_READ OWNER_WRITE
		GROUP_READ GROUP_WRITE
		WORLD_READ
	COMPONENT runtime)

if (${TARGET_MACHINE} MATCHES "gr-mrb-64") #Installing for BXT

	install(FILES IPOD_PLAYER_CFG_BXT.xml
		DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/pfcfg
		RENAME IPOD_PLAYER_CFG.xml
		COMPONENT runtime)

elseif(${TARGET_MACHINE} MATCHES "rcar-gen3") #Installing for RCAR

	install(FILES IPOD_PLAYER_CFG_BXT.xml
		DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/pfcfg
		RENAME IPOD_PLAYER_CFG.xml
		COMPONENT runtime)

else()

	install(FILES IPOD_PLAYER_CFG.xml
		DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/pfcfg
		COMPONENT runtime)

endif() # TARGET_MACHINE
endif() # WITH_IPOD_PLAYER
