# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build

# Include any dependencies generated for this target.
include src/lib/CMakeFiles/dlt.dir/depend.make

# Include the progress variables for this target.
include src/lib/CMakeFiles/dlt.dir/progress.make

# Include the compile flags for this target's objects.
include src/lib/CMakeFiles/dlt.dir/flags.make

src/lib/CMakeFiles/dlt.dir/dlt_user.c.o: src/lib/CMakeFiles/dlt.dir/flags.make
src/lib/CMakeFiles/dlt.dir/dlt_user.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_user.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/lib/CMakeFiles/dlt.dir/dlt_user.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt.dir/dlt_user.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_user.c

src/lib/CMakeFiles/dlt.dir/dlt_user.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt.dir/dlt_user.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_user.c > CMakeFiles/dlt.dir/dlt_user.c.i

src/lib/CMakeFiles/dlt.dir/dlt_user.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt.dir/dlt_user.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_user.c -o CMakeFiles/dlt.dir/dlt_user.c.s

src/lib/CMakeFiles/dlt.dir/dlt_user.c.o.requires:

.PHONY : src/lib/CMakeFiles/dlt.dir/dlt_user.c.o.requires

src/lib/CMakeFiles/dlt.dir/dlt_user.c.o.provides: src/lib/CMakeFiles/dlt.dir/dlt_user.c.o.requires
	$(MAKE) -f src/lib/CMakeFiles/dlt.dir/build.make src/lib/CMakeFiles/dlt.dir/dlt_user.c.o.provides.build
.PHONY : src/lib/CMakeFiles/dlt.dir/dlt_user.c.o.provides

src/lib/CMakeFiles/dlt.dir/dlt_user.c.o.provides.build: src/lib/CMakeFiles/dlt.dir/dlt_user.c.o


src/lib/CMakeFiles/dlt.dir/dlt_client.c.o: src/lib/CMakeFiles/dlt.dir/flags.make
src/lib/CMakeFiles/dlt.dir/dlt_client.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_client.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object src/lib/CMakeFiles/dlt.dir/dlt_client.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt.dir/dlt_client.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_client.c

src/lib/CMakeFiles/dlt.dir/dlt_client.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt.dir/dlt_client.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_client.c > CMakeFiles/dlt.dir/dlt_client.c.i

src/lib/CMakeFiles/dlt.dir/dlt_client.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt.dir/dlt_client.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_client.c -o CMakeFiles/dlt.dir/dlt_client.c.s

src/lib/CMakeFiles/dlt.dir/dlt_client.c.o.requires:

.PHONY : src/lib/CMakeFiles/dlt.dir/dlt_client.c.o.requires

src/lib/CMakeFiles/dlt.dir/dlt_client.c.o.provides: src/lib/CMakeFiles/dlt.dir/dlt_client.c.o.requires
	$(MAKE) -f src/lib/CMakeFiles/dlt.dir/build.make src/lib/CMakeFiles/dlt.dir/dlt_client.c.o.provides.build
.PHONY : src/lib/CMakeFiles/dlt.dir/dlt_client.c.o.provides

src/lib/CMakeFiles/dlt.dir/dlt_client.c.o.provides.build: src/lib/CMakeFiles/dlt.dir/dlt_client.c.o


src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o: src/lib/CMakeFiles/dlt.dir/flags.make
src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_filetransfer.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt.dir/dlt_filetransfer.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_filetransfer.c

src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt.dir/dlt_filetransfer.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_filetransfer.c > CMakeFiles/dlt.dir/dlt_filetransfer.c.i

src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt.dir/dlt_filetransfer.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_filetransfer.c -o CMakeFiles/dlt.dir/dlt_filetransfer.c.s

src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o.requires:

.PHONY : src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o.requires

src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o.provides: src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o.requires
	$(MAKE) -f src/lib/CMakeFiles/dlt.dir/build.make src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o.provides.build
.PHONY : src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o.provides

src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o.provides.build: src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o


src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o: src/lib/CMakeFiles/dlt.dir/flags.make
src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_env_ll.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt.dir/dlt_env_ll.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_env_ll.c

src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt.dir/dlt_env_ll.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_env_ll.c > CMakeFiles/dlt.dir/dlt_env_ll.c.i

src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt.dir/dlt_env_ll.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib/dlt_env_ll.c -o CMakeFiles/dlt.dir/dlt_env_ll.c.s

src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o.requires:

.PHONY : src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o.requires

src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o.provides: src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o.requires
	$(MAKE) -f src/lib/CMakeFiles/dlt.dir/build.make src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o.provides.build
.PHONY : src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o.provides

src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o.provides.build: src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o


src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o: src/lib/CMakeFiles/dlt.dir/flags.make
src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_common.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt.dir/__/shared/dlt_common.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_common.c

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt.dir/__/shared/dlt_common.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_common.c > CMakeFiles/dlt.dir/__/shared/dlt_common.c.i

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt.dir/__/shared/dlt_common.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_common.c -o CMakeFiles/dlt.dir/__/shared/dlt_common.c.s

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o.requires:

.PHONY : src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o.requires

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o.provides: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o.requires
	$(MAKE) -f src/lib/CMakeFiles/dlt.dir/build.make src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o.provides.build
.PHONY : src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o.provides

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o.provides.build: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o


src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o: src/lib/CMakeFiles/dlt.dir/flags.make
src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_user_shared.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_user_shared.c

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_user_shared.c > CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.i

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_user_shared.c -o CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.s

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o.requires:

.PHONY : src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o.requires

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o.provides: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o.requires
	$(MAKE) -f src/lib/CMakeFiles/dlt.dir/build.make src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o.provides.build
.PHONY : src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o.provides

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o.provides.build: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o


src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o: src/lib/CMakeFiles/dlt.dir/flags.make
src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_protocol.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_protocol.c

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_protocol.c > CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.i

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/shared/dlt_protocol.c -o CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.s

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o.requires:

.PHONY : src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o.requires

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o.provides: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o.requires
	$(MAKE) -f src/lib/CMakeFiles/dlt.dir/build.make src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o.provides.build
.PHONY : src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o.provides

src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o.provides.build: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o


# Object files for target dlt
dlt_OBJECTS = \
"CMakeFiles/dlt.dir/dlt_user.c.o" \
"CMakeFiles/dlt.dir/dlt_client.c.o" \
"CMakeFiles/dlt.dir/dlt_filetransfer.c.o" \
"CMakeFiles/dlt.dir/dlt_env_ll.c.o" \
"CMakeFiles/dlt.dir/__/shared/dlt_common.c.o" \
"CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o" \
"CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o"

# External object files for target dlt
dlt_EXTERNAL_OBJECTS =

src/lib/libdlt.so.2.16.0: src/lib/CMakeFiles/dlt.dir/dlt_user.c.o
src/lib/libdlt.so.2.16.0: src/lib/CMakeFiles/dlt.dir/dlt_client.c.o
src/lib/libdlt.so.2.16.0: src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o
src/lib/libdlt.so.2.16.0: src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o
src/lib/libdlt.so.2.16.0: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o
src/lib/libdlt.so.2.16.0: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o
src/lib/libdlt.so.2.16.0: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o
src/lib/libdlt.so.2.16.0: src/lib/CMakeFiles/dlt.dir/build.make
src/lib/libdlt.so.2.16.0: src/lib/CMakeFiles/dlt.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking C shared library libdlt.so"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dlt.dir/link.txt --verbose=$(VERBOSE)
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && $(CMAKE_COMMAND) -E cmake_symlink_library libdlt.so.2.16.0 libdlt.so.2 libdlt.so

src/lib/libdlt.so.2: src/lib/libdlt.so.2.16.0
	@$(CMAKE_COMMAND) -E touch_nocreate src/lib/libdlt.so.2

src/lib/libdlt.so: src/lib/libdlt.so.2.16.0
	@$(CMAKE_COMMAND) -E touch_nocreate src/lib/libdlt.so

# Rule to build all files generated by this target.
src/lib/CMakeFiles/dlt.dir/build: src/lib/libdlt.so

.PHONY : src/lib/CMakeFiles/dlt.dir/build

src/lib/CMakeFiles/dlt.dir/requires: src/lib/CMakeFiles/dlt.dir/dlt_user.c.o.requires
src/lib/CMakeFiles/dlt.dir/requires: src/lib/CMakeFiles/dlt.dir/dlt_client.c.o.requires
src/lib/CMakeFiles/dlt.dir/requires: src/lib/CMakeFiles/dlt.dir/dlt_filetransfer.c.o.requires
src/lib/CMakeFiles/dlt.dir/requires: src/lib/CMakeFiles/dlt.dir/dlt_env_ll.c.o.requires
src/lib/CMakeFiles/dlt.dir/requires: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_common.c.o.requires
src/lib/CMakeFiles/dlt.dir/requires: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_user_shared.c.o.requires
src/lib/CMakeFiles/dlt.dir/requires: src/lib/CMakeFiles/dlt.dir/__/shared/dlt_protocol.c.o.requires

.PHONY : src/lib/CMakeFiles/dlt.dir/requires

src/lib/CMakeFiles/dlt.dir/clean:
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib && $(CMAKE_COMMAND) -P CMakeFiles/dlt.dir/cmake_clean.cmake
.PHONY : src/lib/CMakeFiles/dlt.dir/clean

src/lib/CMakeFiles/dlt.dir/depend:
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/lib /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/lib/CMakeFiles/dlt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/lib/CMakeFiles/dlt.dir/depend

