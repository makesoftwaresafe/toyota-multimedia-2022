add_definitions("-fPIC")

file(GLOB iPodPlayerIF_src *.[ch])

add_library(${IF_NAME} STATIC ${iPodPlayerIF_src})

target_link_libraries(${IF_NAME} PUBLIC ${IPC_NAME})

if (WITH_IPOD_PLAYER)
install(TARGETS ${IF_NAME}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES
		iPodPlayerAPI.h
		iPodPlayerCB.h
		iPodPlayerDebug.h
		iPodPlayerDef.h
		iPodPlayerDef_in.h
		iPodPlayerDeviceDetection.h
		ipp_audio_common.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
endif()