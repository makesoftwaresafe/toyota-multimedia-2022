cmake_minimum_required(VERSION 2.6)

project( adit-ipod-iap1 )

# ToDo : Need to set proper version and descrption
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})


set(${PROJECT_NAME}_VERSION ${PROJECT_VERSION})
set(${PROJECT_NAME}_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(${PROJECT_NAME}_VERSION_MINOR ${PROJECT_VERSION_MINOR})

set(PROJECT_DESCRIPTION "ADIT Ipod Iap1")

option(WITH_IPOD_IAP1              "Set ON to build ipod iap1"                 OFF)
option(WITH_IPOD_IAP1_SMOKETEST    "Set ON to build ipod iap1 smoke tests"     OFF)

SET(LIB_INSTALL_SUFFIX "lib suffix" CACHE STRING "Library suffix : 64 for 64-bit & No suffix for 32-bit")

include(CMakeDependentOption)
include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

pkg_check_modules(ADIT_IPOD_COMMON REQUIRED adit-ipod-common)

pkg_check_modules(DLT REQUIRED automotive-dlt)

pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

pkg_check_modules(ADIT_PFCFG_SYS REQUIRED adit-pfcfg-system)

set(IAP1_DLT_LOGGING_COMP_NAME            ipod-iap1-dlt_logging)
set(TRANS_NAME                            ipod-trans)
set(PLUG_NAME_BTCOM                       ipod-bt-com)
set(PLUG_NAME_USB_HIDAPI                  ipod-usb-host_hidapi)
set(PROT_NAME                             ipod-prot)

set(IPOD_COMPILER_FLAGS "")

set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -Wno-error=clobbered -DIPOD_USB_HOST_PLUGIN -DBUILDENV_LINUX -DIPOD_BT_PLUGIN " )
set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -D_GNU_SOURCE -DIPOD_FAST_DB -DIPOD_DB_BULK_TRANSFER -DIAP1_DLT_ENABLE " )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IPOD_COMPILER_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IPOD_COMPILER_FLAGS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src/ipod_ctrl/transport/
	${CMAKE_CURRENT_SOURCE_DIR}/src/ipod_ctrl/protocol/
	${CMAKE_CURRENT_SOURCE_DIR}/src/iap1_dlt_logging/
	${ADIT_IPOD_COMMON_INCLUDE_DIRS}
	${ADIT_PFCFG_SYS_INCLUDE_DIRS}
	)


add_subdirectory( src )

if (WITH_IPOD_IAP1)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/config/${PROJECT_NAME}.pc.in
		${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc
		@ONLY)

	install(
		FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
		COMPONENT devel)
endif()

if (WITH_IPOD_IAP1_SMOKETEST)

	set(IPOD_CTRL_TEST          ipod_ctrl_smoketest.out)
	set(DEST_SMOKETEST_CMD_DIR  /opt/ltp/runtest/smoketest)
	set(DEST_SMOKETEST_DIR      /opt/ltp/testcases/bin)


	set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -DIPOD_HOSTPLUGIN_HIDAPI -DBUILDENV_LINUX " )

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IPOD_COMPILER_FLAGS}")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IPOD_COMPILER_FLAGS}")

	add_subdirectory( test )

endif()
