file(GLOB iap2_over_carplay_plugin_lib_src *.[hc])

add_library(${IPOD_IAP2_OVER_CARPLAY_PLUGIN} SHARED ${iap2_over_carplay_plugin_lib_src})

target_link_libraries(${IPOD_IAP2_OVER_CARPLAY_PLUGIN} PUBLIC
		ipod-iap2-dlt_logging
		ipod-dlt_logging)

if (WITH_IPOD_IAP2)
install(TARGETS ${IPOD_IAP2_OVER_CARPLAY_PLUGIN}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)
endif()
