add_definitions("-std=c++11")

file(GLOB iap2_interface_comp_lib_include *.[h])

add_library(${IAP2_INTERFACE_COMP_NAME} SHARED
			iap2_init.c
			iap2_service_init.cpp
			iap2_service_messages.cpp
			${iap2_interface_comp_lib_include})

target_link_libraries(${IAP2_INTERFACE_COMP_NAME} PUBLIC
			stdc++
			ipod-iap2-dlt_logging
			ipod-dlt_logging
			${IAP2_LINK_COMP_NAME}
			${IAP2_SESSION_COMP_NAME}
			${IAP2_TRANSPORT_COMP_NAME}
			${AUTH_NAME})

if (WITH_IPOD_IAP2)
install(TARGETS ${IAP2_INTERFACE_COMP_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES
		iap2_callbacks.h
		iap2_commands.h
		iap2_cs_callbacks.h
		iap2_enums.h
		iap2_external_accessory_protocol_session.h
		iap2_file_transfer.h
		iap2_init.h
		iap2_parameter_free.h
		iap2_parameters.h
		iap2_service_init.h
		iap2_subparameters.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
endif()