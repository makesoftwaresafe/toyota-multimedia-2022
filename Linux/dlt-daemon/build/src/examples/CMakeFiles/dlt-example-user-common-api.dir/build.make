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
include src/examples/CMakeFiles/dlt-example-user-common-api.dir/depend.make

# Include the progress variables for this target.
include src/examples/CMakeFiles/dlt-example-user-common-api.dir/progress.make

# Include the compile flags for this target's objects.
include src/examples/CMakeFiles/dlt-example-user-common-api.dir/flags.make

src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o: src/examples/CMakeFiles/dlt-example-user-common-api.dir/flags.make
src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/examples/dlt-example-user-common-api.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/examples && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/examples/dlt-example-user-common-api.c

src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/examples && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/examples/dlt-example-user-common-api.c > CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.i

src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/examples && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/examples/dlt-example-user-common-api.c -o CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.s

src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o.requires:

.PHONY : src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o.requires

src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o.provides: src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o.requires
	$(MAKE) -f src/examples/CMakeFiles/dlt-example-user-common-api.dir/build.make src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o.provides.build
.PHONY : src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o.provides

src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o.provides.build: src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o


# Object files for target dlt-example-user-common-api
dlt__example__user__common__api_OBJECTS = \
"CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o"

# External object files for target dlt-example-user-common-api
dlt__example__user__common__api_EXTERNAL_OBJECTS =

src/examples/dlt-example-user-common-api: src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o
src/examples/dlt-example-user-common-api: src/examples/CMakeFiles/dlt-example-user-common-api.dir/build.make
src/examples/dlt-example-user-common-api: src/lib/libdlt.so.2.16.0
src/examples/dlt-example-user-common-api: src/examples/CMakeFiles/dlt-example-user-common-api.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable dlt-example-user-common-api"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/examples && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dlt-example-user-common-api.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/examples/CMakeFiles/dlt-example-user-common-api.dir/build: src/examples/dlt-example-user-common-api

.PHONY : src/examples/CMakeFiles/dlt-example-user-common-api.dir/build

src/examples/CMakeFiles/dlt-example-user-common-api.dir/requires: src/examples/CMakeFiles/dlt-example-user-common-api.dir/dlt-example-user-common-api.c.o.requires

.PHONY : src/examples/CMakeFiles/dlt-example-user-common-api.dir/requires

src/examples/CMakeFiles/dlt-example-user-common-api.dir/clean:
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/examples && $(CMAKE_COMMAND) -P CMakeFiles/dlt-example-user-common-api.dir/cmake_clean.cmake
.PHONY : src/examples/CMakeFiles/dlt-example-user-common-api.dir/clean

src/examples/CMakeFiles/dlt-example-user-common-api.dir/depend:
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/examples /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/examples /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/examples/CMakeFiles/dlt-example-user-common-api.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/examples/CMakeFiles/dlt-example-user-common-api.dir/depend
