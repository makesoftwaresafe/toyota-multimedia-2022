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
include src/tests/CMakeFiles/dlt-test-client.dir/depend.make

# Include the progress variables for this target.
include src/tests/CMakeFiles/dlt-test-client.dir/progress.make

# Include the compile flags for this target's objects.
include src/tests/CMakeFiles/dlt-test-client.dir/flags.make

src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o: src/tests/CMakeFiles/dlt-test-client.dir/flags.make
src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/tests/dlt-test-client.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/tests && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/tests/dlt-test-client.c

src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt-test-client.dir/dlt-test-client.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/tests && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/tests/dlt-test-client.c > CMakeFiles/dlt-test-client.dir/dlt-test-client.c.i

src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt-test-client.dir/dlt-test-client.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/tests && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/tests/dlt-test-client.c -o CMakeFiles/dlt-test-client.dir/dlt-test-client.c.s

src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o.requires:

.PHONY : src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o.requires

src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o.provides: src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o.requires
	$(MAKE) -f src/tests/CMakeFiles/dlt-test-client.dir/build.make src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o.provides.build
.PHONY : src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o.provides

src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o.provides.build: src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o


# Object files for target dlt-test-client
dlt__test__client_OBJECTS = \
"CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o"

# External object files for target dlt-test-client
dlt__test__client_EXTERNAL_OBJECTS =

src/tests/dlt-test-client: src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o
src/tests/dlt-test-client: src/tests/CMakeFiles/dlt-test-client.dir/build.make
src/tests/dlt-test-client: src/lib/libdlt.so.2.16.0
src/tests/dlt-test-client: src/tests/CMakeFiles/dlt-test-client.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable dlt-test-client"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dlt-test-client.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/tests/CMakeFiles/dlt-test-client.dir/build: src/tests/dlt-test-client

.PHONY : src/tests/CMakeFiles/dlt-test-client.dir/build

src/tests/CMakeFiles/dlt-test-client.dir/requires: src/tests/CMakeFiles/dlt-test-client.dir/dlt-test-client.c.o.requires

.PHONY : src/tests/CMakeFiles/dlt-test-client.dir/requires

src/tests/CMakeFiles/dlt-test-client.dir/clean:
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/tests && $(CMAKE_COMMAND) -P CMakeFiles/dlt-test-client.dir/cmake_clean.cmake
.PHONY : src/tests/CMakeFiles/dlt-test-client.dir/clean

src/tests/CMakeFiles/dlt-test-client.dir/depend:
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/tests /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/tests /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/tests/CMakeFiles/dlt-test-client.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/tests/CMakeFiles/dlt-test-client.dir/depend

