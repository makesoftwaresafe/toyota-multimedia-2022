set(iap2_cinemo_test_src    iap2_cinemo_test.c
                            iap2_cinemo_test.h)

add_executable(${IAP2_CINEMO_TEST} ${iap2_cinemo_test_src})

target_link_libraries(${IAP2_CINEMO_TEST} PUBLIC
    udev
    ${ADIT_PFCFG_SYS_LIBRARIES}
    rt
    pthread
    kmod
    ${ADIT_IPOD_COMMON_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBUSB_LIBRARIES}
    ${CINEMO_AUTH_LIB}
    ${ADIT_IPOD_IAP2_LIBRARIES})

install(TARGETS ${IAP2_CINEMO_TEST}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
	COMPONENT runtime)

