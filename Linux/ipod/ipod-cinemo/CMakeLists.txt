cmake_minimum_required(VERSION 2.6)

project( adit-ipod-cinemo )

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

set(${PROJECT_NAME}_VERSION ${PROJECT_VERSION})
set(${PROJECT_NAME}_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(${PROJECT_NAME}_VERSION_MINOR ${PROJECT_VERSION_MINOR})

set(PROJECT_DESCRIPTION "ADIT Ipod Cinemo Plugins")

option(WITH_IPOD_IAP2_CINEMO              "Set ON to build ipod iap2 cinemo libraries"            OFF)
option(WITH_IPOD_IAP2_CINEMO_TEST         "Set ON to build ipod iap2 cinemo smoke tests"          OFF)

SET(LIB_INSTALL_SUFFIX "lib suffix" CACHE STRING "Library suffix : 64 for 64-bit & No suffix for 32-bit")

include(CMakeDependentOption)
include(GNUInstallDirs)

set(
	CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/"
)

find_package(PkgConfig REQUIRED)

pkg_check_modules(DLT REQUIRED automotive-dlt)
pkg_check_modules(UTIL_AUDIO REQUIRED adit-util-audio)
pkg_check_modules(ADIT_PFCFG_SYS REQUIRED adit-pfcfg-system)

find_package(LINUX_HEADERS REQUIRED)

find_package(ALSA REQUIRED)

pkg_check_modules(ADIT_IPOD_COMMON REQUIRED adit-ipod-common)

set(CINEMO_AUTH_LIB                       ADITAppleAuthImpl)
set(CINEMO_AUDIO_LIB                       ADITAppleAudioImpl)
set(IAP2_CINEMO_HOST_PLUGIN        ADITiap2_cinemo_host_plugin)

set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -Wno-error=clobbered -DIAP2_DLT_ENABLE -DBUILDENV_LINUX " )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IPOD_COMPILER_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IPOD_COMPILER_FLAGS}")

set(CINEMO_INSTALL_LIBDIR /usr/local/lib/cinemo/cinemo/)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src/iap2_cinemo_host_plugin/
	${ADIT_PFCFG_SYS_INCLUDE_DIRS}
	${ADIT_IPOD_COMMON_INCLUDE_DIRS}
	${LINUX_HEADERS_INCLUDE_DIR}
	)

add_subdirectory( src )

if (WITH_IPOD_IAP2_CINEMO)
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


if (WITH_IPOD_IAP2_CINEMO_TEST)
	set(IAP2_CINEMO_TEST               ADITCinemo_test)

	pkg_check_modules(ADIT_IPOD_IAP2 REQUIRED adit-ipod-iap2)

	set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -Wno-error=clobbered -DBUILDENV_LINUX " )

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IPOD_COMPILER_FLAGS}")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IPOD_COMPILER_FLAGS}")

	include_directories(
		${ADIT_IPOD_COMMON_INCLUDE_DIRS}
		${ADIT_IPOD_IAP2_INCLUDE_DIRS}
	)

	add_subdirectory( test )

endif()
