set(cinemo_auth_plugin_src  iap2_cinemo_apple_auth_interface.c
                            cinemo_apple_auth_interface.h)

add_library(${CINEMO_AUTH_LIB} SHARED "${cinemo_auth_plugin_src}")

target_link_libraries(${CINEMO_AUTH_LIB} PUBLIC
                      ipod-dlt_logging
                      ipod-iap2-dlt_logging
                      ipod-auth-com
                      ipod-auth)
		
install(TARGETS ${CINEMO_AUTH_LIB}
		DESTINATION ${CINEMO_INSTALL_LIBDIR}
		COMPONENT runtime)

