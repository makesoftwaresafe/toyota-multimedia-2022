add_definitions("-fPIC")

file(GLOB player_util_src *.[ch])

add_library(${UTIL_NAME} STATIC ${player_util_src})

target_link_libraries(${UTIL_NAME} PUBLIC
		${LIBXML2_LIBRARIES}
		${DLT_LIBRARIES})

if (WITH_IPOD_PLAYER)
install(TARGETS ${UTIL_NAME}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

file(GLOB player_util_header *.[h])

install(FILES ${player_util_header}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
endif()
