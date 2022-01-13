if (${IPOD_AUDIO_SINK} MATCHES "GST")
	set(ipod_player_audio_stream_test_src gst_com_server.c ipp_audio_common.c)
endif()

if (${IPOD_AUDIO_SINK} MATCHES "ALSA")
	set(ipod_player_audio_stream_test_src alsa_com_server.c ipp_audio_common.c)
endif()


add_executable(${IPOD_PLAYER_AUDIO} ${ipod_player_audio_stream_test_src})

target_link_libraries(${IPOD_PLAYER_AUDIO} PUBLIC
	udev
	rt
	m
	z
	pthread
	${DLT_LIBRARIES}
	${IPC_NAME}
	${UTIL_NAME}
	${IF_NAME}
	${LIBXML2_LIBRARIES}
	${LIBUSB_LIBRARIES})

if (${IPOD_AUDIO_SINK} MATCHES "GST")
	target_link_libraries(${IPOD_PLAYER_AUDIO} PUBLIC
		glib-2.0
		gobject-2.0
		gthread-2.0
		gmodule-2.0
		${GSTREAMER_LIBRARIES})
endif()

if (${IPOD_AUDIO_SINK} MATCHES "ALSA")
	target_link_libraries(${IPOD_PLAYER_AUDIO} PUBLIC
		${ALSA_LIBRARIES})
endif()

install(TARGETS ${IPOD_PLAYER_AUDIO}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
	COMPONENT runtime)