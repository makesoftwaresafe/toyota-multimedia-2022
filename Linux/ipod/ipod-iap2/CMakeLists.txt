cmake_minimum_required(VERSION 2.6)

project( adit-ipod-iap2 )

# ToDo : Need to set proper version and descrption
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

set(${PROJECT_NAME}_VERSION ${PROJECT_VERSION})
set(${PROJECT_NAME}_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(${PROJECT_NAME}_VERSION_MINOR ${PROJECT_VERSION_MINOR})

set(PROJECT_DESCRIPTION "ADIT Ipod Iap2")

option(WITH_IPOD_IAP2              "Set ON to build ipod iap2"                 OFF)
option(WITH_IPOD_IAP2_SMOKETEST    "Set ON to build ipod iap2 smoke tests"     OFF)

SET(LIB_INSTALL_SUFFIX "lib suffix" CACHE STRING "Library suffix : 64 for 64-bit & No suffix for 32-bit")

include(CMakeDependentOption)
include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

set(
	CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/"
)

SET(TARGET_ARCH    "NO_ARCH"    CACHE STRING "Build Target Arch type : arm, x86_64 or aarch64")

find_package(SYSTEMD REQUIRED)

find_package(LINUX_HEADERS REQUIRED)

pkg_check_modules(ADIT_IPOD_COMMON REQUIRED adit-ipod-common)

pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

pkg_check_modules(DLT REQUIRED automotive-dlt)

pkg_check_modules(ADIT_PFCFG_SYS REQUIRED adit-pfcfg-system)

if(SYSTEMD_FOUND)
	set(SYSTEMD_CONFIGURATIONS_FILES_DIR "/lib/systemd/system" )
endif()


set(IAP2_INTERFACE_COMP_NAME              ipod-iap2-interface)
set(IAP2_LINK_COMP_NAME                   ipod-iap2-linklayer)
set(IAP2_SESSION_COMP_NAME                ipod-iap2-session)
set(IAP2_TRANSPORT_COMP_NAME              ipod-iap2-transport)
set(IAP2_SERVICE                          iap2_service)
set(IPOD_IAP2_BT_PLUGIN                   ipod-iap2_bluetooth_plugin)
set(IPOD_IAP2_OVER_CARPLAY_PLUGIN         ipod-iap2_over_carplay_plugin)
set(IPOD_IAP2_USB_DEV_MODE_PLUGIN         ipod-iap2_usb_device_mode_plugin)
set(IPOD_IAP2_USB_HOST_MODE_PLUGIN        ipod-iap2_usb_host_mode_plugin)
set(IPOD_IAP2_USB_MULTI_HOST_MODE_PLUGIN  ipod-iap2_usb_multi_host_mode_plugin)
set(IAP2_FFS_GADGET_COMP_NAME             ipod-iap2-ffs-gadget)
set(IAP2_USB_GADGET_COMP_NAME             ipod-iap2-usb-gadget)

set(IPOD_COMPILER_FLAGS "")

if (${TARGET_ARCH} MATCHES "arm")
	set(IPOD_COMPILER_FLAGS " -DIPOD_ARCH_ARM ")
endif()
if(${TARGET_ARCH} MATCHES "x86_64")
	set(IPOD_COMPILER_FLAGS " -DIPOD_ARCH_X86_64 ")
endif()
if(${TARGET_ARCH} MATCHES "aarch64")
	set(IPOD_COMPILER_FLAGS " -DIPOD_ARCH_ARM64 ")
endif()

set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -Wno-error=clobbered -D_GNU_SOURCE -DIAP2_DLT_ENABLE " )
set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -DBUILDENV_LINUX " )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IPOD_COMPILER_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IPOD_COMPILER_FLAGS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src/iAP2/interface/
	${CMAKE_CURRENT_SOURCE_DIR}/src/iAP2/transport/
	${CMAKE_CURRENT_SOURCE_DIR}/src/iAP2/session/
	${CMAKE_CURRENT_SOURCE_DIR}/src/iap2_gadget/ffs_gadget/
	${CMAKE_CURRENT_SOURCE_DIR}/src/iap2_gadget/usb_gadget/
	${CMAKE_CURRENT_SOURCE_DIR}/src/common/
	${LINUX_HEADERS_INCLUDE_DIR}
	${LIBUSB_INCLUDE_DIRS}
	${ADIT_IPOD_COMMON_INCLUDE_DIRS}
	${ADIT_PFCFG_SYS_INCLUDE_DIRS}
	)

add_subdirectory( src )

if (WITH_IPOD_IAP2)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/config/${PROJECT_NAME}.pc.in
		${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc
		@ONLY
		)
	install(
		FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
		COMPONENT devel
		)
endif()


if (WITH_IPOD_IAP2_SMOKETEST)
	set(IAP2_TEST_UTILITY                     ipod-iap2_test_utility)
	set(IAP2_SMOKETEST                        ipod-iap2_smoketest.out)
	set(IAP2_SERVICE_SMOKETEST                ipod-iap2_service_smoketest.out)

	pkg_check_modules(GSTREAMER gstreamer-0.10)
	if (NOT GSTREAMER_FOUND)
		pkg_check_modules(GSTREAMER gstreamer-1.0)
	endif()

	find_package(LibXml2)

		if(GSTREAMER_FOUND)
			if ((GSTREAMER_VERSION EQUAL 1.0) OR (GSTREAMER_VERSION GREATER 1.0))
				set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -DGSTREAMER_VERSION_10 ")
			elseif (GSTREAMER_VERSION LESS 1.0)
				set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -DGSTREAMER_VERSION_010 ")
			endif()
		else()
			message(FATAL_ERROR "Gstreamer not found")
		endif()

	set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -Wno-error=clobbered -DIPOD_HOSTPLUGIN_HIDAPI -DBUILDENV_LINUX " )

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IPOD_COMPILER_FLAGS}")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IPOD_COMPILER_FLAGS}")

	include_directories(
		${CMAKE_CURRENT_SOURCE_DIR}/test/iap2_test_utility/
		${LIBXML2_INCLUDE_DIR}
		${GSTREAMER_INCLUDE_DIRS}
	)

	add_subdirectory( test )

endif()
