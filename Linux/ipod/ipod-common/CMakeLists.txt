cmake_minimum_required(VERSION 2.6)

project( adit-ipod-common )

# ToDo : Need to set proper version and descrption
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})


set(${PROJECT_NAME}_VERSION ${PROJECT_VERSION})
set(${PROJECT_NAME}_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(${PROJECT_NAME}_VERSION_MINOR ${PROJECT_VERSION_MINOR})

set(PROJECT_DESCRIPTION "ADIT Ipod Authentication")

SET(TARGET_ARCH    "NO_ARCH"    CACHE STRING "Build Target Arch type : arm, x86_64 or aarch64")
SET(LIB_INSTALL_SUFFIX "lib suffix" CACHE STRING "Library suffix : 64 for 64-bit & No suffix for 32-bit")

option(WITH_IPOD_COMMON            "Set ON to build ipod Authentication"       OFF)

include(CMakeDependentOption)
include(GNUInstallDirs)

set(
	CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/"
)

find_package(PkgConfig REQUIRED)

find_package(SYSTEMD REQUIRED)

pkg_check_modules(DLT REQUIRED automotive-dlt)

pkg_check_modules(ADIT_PFCFG_SYS REQUIRED adit-pfcfg-system)

pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

if(SYSTEMD_FOUND)
	set(SYSTEMD_CONFIGURATIONS_FILES_DIR "/lib/systemd/system" )
endif()

SET(AUTH_PLUGIN "auth_sub20_plugin" CACHE STRING
				"Authentication plugin I2C or SUB20 choose : auth_i2c_plugin or auth_sub20_plugin")

set(IPOD_DLT_LOGGING_COMP_NAME            ipod-dlt_logging)
set(IAP2_DLT_LOGGING_COMP_NAME            ipod-iap2-dlt_logging)
set(PLUG_NAME_AUTHCOM                     ipod-auth-com)
set(AUTH_NAME                             ipod-auth)
set(IAP2_USB_ROLE_SWITCH_COMP_NAME        ipod-iap2-usb-role-switch)

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

set(IPOD_COMPILER_FLAGS " ${IPOD_COMPILER_FLAGS} -Wno-error=clobbered -DIAP2_DLT_ENABLE -DBUILDENV_LINUX -DIAP2_DLT_ENABLE " )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IPOD_COMPILER_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IPOD_COMPILER_FLAGS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src/ipod_logging/iap2_dlt_logging/
	${CMAKE_CURRENT_SOURCE_DIR}/src/ipod_logging/ipod_dlt_logging/
	${CMAKE_CURRENT_SOURCE_DIR}/src/auth_plugin/${AUTH_PLUGIN}/
	${CMAKE_CURRENT_SOURCE_DIR}/src/ipod_authentication/authentication/
	${ADIT_PFCFG_SYS_INCLUDE_DIRS}
	${LIBUSB_INCLUDE_DIRS}
	)

add_subdirectory( src )

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
