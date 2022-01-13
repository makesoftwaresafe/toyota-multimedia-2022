file(GLOB iap2_bluetooth_plugin_lib_src *.[hc])

add_library(${IPOD_IAP2_BT_PLUGIN} SHARED ${iap2_bluetooth_plugin_lib_src})

target_link_libraries(${IPOD_IAP2_BT_PLUGIN} PUBLIC
		bluetooth
		ipod-dlt_logging
		ipod-iap2-dlt_logging)

if (WITH_IPOD_IAP2)
install(TARGETS ${IPOD_IAP2_BT_PLUGIN}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)
endif()
