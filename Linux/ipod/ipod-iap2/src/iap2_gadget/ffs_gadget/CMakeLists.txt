file(GLOB iap2_ffs_gadget_lib_src *.[hc])

add_library(${IAP2_FFS_GADGET_COMP_NAME} SHARED ${iap2_ffs_gadget_lib_src})

target_link_libraries(${IAP2_FFS_GADGET_COMP_NAME} PUBLIC
		usbg
		pthread
		ipod-dlt_logging
		ipod-iap2-dlt_logging)

if (WITH_IPOD_IAP2)
install(TARGETS ${IAP2_FFS_GADGET_COMP_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES iap2_configure_ffs_gadget.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
endif()