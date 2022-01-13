cmake_minimum_required(VERSION 2.6)

project( adit-ipod-player )

# ToDo : Need to set proper version and descrption
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

set(${PROJECT_NAME}_VERSION ${PROJECT_VERSION})
set(${PROJECT_NAME}_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(${PROJECT_NAME}_VERSION_MINOR ${PROJECT_VERSION_MINOR})

set(PROJECT_DESCRIPTION "ADIT Ipod Player")

option(WITH_IPOD_PLAYER            "Set ON to build ipod player"               OFF)
option(WITH_IPOD_PLAYER_SMOKETEST  "Set ON to build ipod player smoke tests"   OFF)

include(CMakeDependentOption)
include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

set(
	CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/"
)

SET(TARGET_MACHINE "NO_MACHINE" CACHE STRING "Build Target Machine type : mx6q, oracle-virtualbox, gr-mrb-64 or rcar-gen3")

SET(LIB_INSTALL_SUFFIX "lib suffix" CACHE STRING "Library suffix : 64 for 64-bit & No suffix for 32-bit")

pkg_check_modules(ADIT_IPOD_COMMON REQUIRED adit-ipod-common)

pkg_check_modules(ADIT_IPOD_IAP2 REQUIRED adit-ipod-iap2)

pkg_check_modules(ADIT_IPOD_IAP1 REQUIRED adit-ipod-iap1)

pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

pkg_check_modules(DLT REQUIRED automotive-dlt)

pkg_check_modules(ADIT_PFCFG_SYS REQUIRED adit-pfcfg-system)

find_package(SYSTEMD REQUIRED)

find_package(LINUX_HEADERS REQUIRED)

if(SYSTEMD_FOUND)
	set(SYSTEMD_CONFIGURATIONS_FILES_DIR "/lib/systemd/system" )
endif()

find_package(LibXml2)

set(UTIL_NAME                             iPodPlayerUtility)
set(IPC_NAME                              iPodPlayerIPC)
set(IF_NAME                               iPodPlayerIF)
set(CORE_NAME                             iPodPlayerCore.out)

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

set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -Wno-error=clobbered -DIPOD_HAS_DLT -DIPOD_PLAYER_CORE_SELF_RUNNING " )
set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -DBUILDENV_LINUX " )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IPOD_COMPILER_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IPOD_COMPILER_FLAGS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src/ipodplayer_lib/ipc/
	${CMAKE_CURRENT_SOURCE_DIR}/src/ipodplayer_lib/utility/
	${CMAKE_CURRENT_SOURCE_DIR}/src/ipodplayer_wrapper/interface/
	${LINUX_HEADERS_INCLUDE_DIR}
	${LIBXML2_INCLUDE_DIR}
	${LIBUSB_INCLUDE_DIRS}
	${DLT_INCLUDE_DIRS}
	${ADIT_IPOD_COMMON_INCLUDE_DIRS}
	${ADIT_IPOD_IAP2_INCLUDE_DIRS}
	${ADIT_IPOD_IAP1_INCLUDE_DIRS}
	${ADIT_PFCFG_SYS_INCLUDE_DIRS}
	)

add_subdirectory( src )

if (WITH_IPOD_PLAYER)
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

if (WITH_IPOD_PLAYER_SMOKETEST)
	set(IPOD_SAMPLE_APP                 ipod-sample_app)
	set(IPOD_PLAYER_DEV_DETECTION       iPodPlayerDeviceDetection)
	set(IPOD_PLAYER_AUDIO               iPodPlayerAudio)
	set(IPOD_DEVICE_CHECK               ipod-device_check)

	set(DEST_SMOKETEST_CMD_DIR     /opt/ltp/runtest/smoketest)
	set(DEST_SMOKETEST_BIN_DIR     /opt/ltp/testcases/bin)
	set(DEST_SMOKETEST_SCRIPT_DIR  /opt/ltp/testscripts)

	set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -Wno-error=clobbered -D_GNU_SOURCE -DBUILDENV_LINUX " )

	if (${IPOD_AUDIO_SINK} MATCHES "GST")
		pkg_check_modules(GSTREAMER gstreamer-0.10)
		if(NOT GSTREAMER_FOUND)
			pkg_check_modules(GSTREAMER gstreamer-1.0)
		endif()

		if(GSTREAMER_FOUND)
			if ((GSTREAMER_VERSION GREATER 1.0) OR (GSTREAMER_VERSION EQUAL 1.0))
				set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -DGSTREAMER_VERSION_10 ")
			elseif (GSTREAMER_VERSION LESS 1.0)
				set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -DGSTREAMER_VERSION_010 ")
			endif()
		else()
			message(FATAL_ERROR "Gstreamer not found")
		endif()

		include_directories(
			${GSTREAMER_INCLUDE_DIRS}
		)
	elseif(${IPOD_AUDIO_SINK} MATCHES "ALSA")
		find_package(ALSA)

		include_directories(
			${ALSA_INCLUDE_DIRS}
		)
	endif()

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IPOD_COMPILER_FLAGS}")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IPOD_COMPILER_FLAGS}")

	add_subdirectory( test )

endif()
