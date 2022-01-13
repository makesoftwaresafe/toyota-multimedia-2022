add_definitions("-fPIC")

file(GLOB player_ipc_src *.[ch])

add_library(${IPC_NAME} STATIC ${player_ipc_src})

target_link_libraries(${IPC_NAME} PUBLIC ${UTIL_NAME})

if ( WITH_IPOD_PLAYER )
install(TARGETS ${IPC_NAME}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT runtime)

install(FILES
	iPodPlayerIPCLib.h
	iPodPlayerIPCLocal.h
	iPodPlayerIPCMessage.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adit-components
	COMPONENT devel)
endif()
