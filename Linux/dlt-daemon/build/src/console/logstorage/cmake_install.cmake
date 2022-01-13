# Install script for directory: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console/logstorage

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "base")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-logstorage-ctrl" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-logstorage-ctrl")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-logstorage-ctrl"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console/logstorage/dlt-logstorage-ctrl")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-logstorage-ctrl" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-logstorage-ctrl")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-logstorage-ctrl"
         OLD_RPATH "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-logstorage-ctrl")
    endif()
  endif()
endif()

