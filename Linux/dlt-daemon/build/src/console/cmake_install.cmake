# Install script for directory: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console

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
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-convert" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-convert")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-convert"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console/dlt-convert")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-convert" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-convert")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-convert"
         OLD_RPATH "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-convert")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "base")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-receive" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-receive")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-receive"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console/dlt-receive")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-receive" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-receive")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-receive"
         OLD_RPATH "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-receive")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "base")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-control" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-control")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-control"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console/dlt-control")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-control" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-control")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-control"
         OLD_RPATH "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-control")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "base")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-filter-ctrl" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-filter-ctrl")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-filter-ctrl"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console/dlt-filter-ctrl")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-filter-ctrl" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-filter-ctrl")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-filter-ctrl"
         OLD_RPATH "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-filter-ctrl")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "base")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-passive-node-ctrl" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-passive-node-ctrl")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-passive-node-ctrl"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console/dlt-passive-node-ctrl")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-passive-node-ctrl" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-passive-node-ctrl")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-passive-node-ctrl"
         OLD_RPATH "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-passive-node-ctrl")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "base")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-blockmode-ctrl" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-blockmode-ctrl")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-blockmode-ctrl"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console/dlt-blockmode-ctrl")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-blockmode-ctrl" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-blockmode-ctrl")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-blockmode-ctrl"
         OLD_RPATH "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/dlt-blockmode-ctrl")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console/logstorage/cmake_install.cmake")

endif()

