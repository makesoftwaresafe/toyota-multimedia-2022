include_directories(
	./
)
file(GLOB iap2_service_daemon_src "*.cpp")

add_definitions("-DADIT_LOGGING_DLT -std=c++0x")

add_executable(${IAP2_SERVICE} "${iap2_service_daemon_src}")

target_link_libraries(${IAP2_SERVICE} PUBLIC
		pthread
		stdc++
		dlt
		kmod
		rt
		ipod-iap2-usb-role-switch
		${LIBUSB_LIBRARIES}
		${SYSTEMD_LIBRARIES}
		${DLT_LIBRARIES}
		${ADIT_IPOD_COMMON_LIBRARIES}
		${IAP2_FFS_GADGET_COMP_NAME}
		${IPOD_IAP2_BT_PLUGIN}
		${IPOD_IAP2_USB_DEV_MODE_PLUGIN}
		${IPOD_IAP2_USB_HOST_MODE_PLUGIN}
		${IPOD_IAP2_OVER_CARPLAY_PLUGIN}
		${IPOD_IAP2_USB_MULTI_HOST_MODE_PLUGIN}
		${IAP2_LINK_COMP_NAME}
		${IAP2_SESSION_COMP_NAME}
		${IAP2_TRANSPORT_COMP_NAME}
		${IAP2_INTERFACE_COMP_NAME})

if (WITH_IPOD_IAP2)
install(TARGETS ${IAP2_SERVICE}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
	COMPONENT runtime)

configure_file(
	iap2_service.service.in
	${PROJECT_BINARY_DIR}/iap2_service.service
	@ONLY)

install(
	FILES ${PROJECT_BINARY_DIR}/iap2_service.service
	DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR}
	COMPONENT runtime)
endif()