add_definitions("-Wno-error")

file(GLOB linklayer_lib_src iAP2Link/*.[hc] iAP2Utility/*.[hc] iAP2UtilityImplementation/*.[hc])

add_library(${IAP2_LINK_COMP_NAME} SHARED ${linklayer_lib_src})

target_link_libraries(${IAP2_LINK_COMP_NAME} PUBLIC
	ipod-dlt_logging
	ipod-iap2-dlt_logging)

if (WITH_IPOD_IAP2)
install(TARGETS ${IAP2_LINK_COMP_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)
endif()