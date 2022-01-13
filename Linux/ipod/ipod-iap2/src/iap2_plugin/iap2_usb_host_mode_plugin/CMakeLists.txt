file(GLOB usb_host_mode_lib_src *.[ch])

add_library(${IPOD_IAP2_USB_HOST_MODE_PLUGIN} SHARED ${usb_host_mode_lib_src})

target_link_libraries(${IPOD_IAP2_USB_HOST_MODE_PLUGIN} PUBLIC
		ipod-iap2-dlt_logging
		ipod-dlt_logging
		${IAP2_FFS_GADGET_COMP_NAME})

if (WITH_IPOD_IAP2)
install(TARGETS ${IPOD_IAP2_USB_HOST_MODE_PLUGIN}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		COMPONENT runtime)
endif()