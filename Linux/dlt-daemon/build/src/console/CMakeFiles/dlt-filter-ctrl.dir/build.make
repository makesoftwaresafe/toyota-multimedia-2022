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
include src/console/CMakeFiles/dlt-filter-ctrl.dir/depend.make

# Include the progress variables for this target.
include src/console/CMakeFiles/dlt-filter-ctrl.dir/progress.make

# Include the compile flags for this target's objects.
include src/console/CMakeFiles/dlt-filter-ctrl.dir/flags.make

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o: src/console/CMakeFiles/dlt-filter-ctrl.dir/flags.make
src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console/dlt-filter-ctrl.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console/dlt-filter-ctrl.c

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console/dlt-filter-ctrl.c > CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.i

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console/dlt-filter-ctrl.c -o CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.s

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o.requires:

.PHONY : src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o.requires

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o.provides: src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o.requires
	$(MAKE) -f src/console/CMakeFiles/dlt-filter-ctrl.dir/build.make src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o.provides.build
.PHONY : src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o.provides

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o.provides.build: src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o


src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o: src/console/CMakeFiles/dlt-filter-ctrl.dir/flags.make
src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o: /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console/dlt-control-common.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o   -c /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console/dlt-control-common.c

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.i"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console/dlt-control-common.c > CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.i

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.s"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console && /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/qnx-sdp7/host/linux/x86_64/usr/bin/x86_64-pc-nto-qnx7.0.0-gcc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console/dlt-control-common.c -o CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.s

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o.requires:

.PHONY : src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o.requires

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o.provides: src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o.requires
	$(MAKE) -f src/console/CMakeFiles/dlt-filter-ctrl.dir/build.make src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o.provides.build
.PHONY : src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o.provides

src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o.provides.build: src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o


# Object files for target dlt-filter-ctrl
dlt__filter__ctrl_OBJECTS = \
"CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o" \
"CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o"

# External object files for target dlt-filter-ctrl
dlt__filter__ctrl_EXTERNAL_OBJECTS =

src/console/dlt-filter-ctrl: src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o
src/console/dlt-filter-ctrl: src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o
src/console/dlt-filter-ctrl: src/console/CMakeFiles/dlt-filter-ctrl.dir/build.make
src/console/dlt-filter-ctrl: src/lib/libdlt.so.2.16.0
src/console/dlt-filter-ctrl: src/console/CMakeFiles/dlt-filter-ctrl.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable dlt-filter-ctrl"
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dlt-filter-ctrl.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/console/CMakeFiles/dlt-filter-ctrl.dir/build: src/console/dlt-filter-ctrl

.PHONY : src/console/CMakeFiles/dlt-filter-ctrl.dir/build

src/console/CMakeFiles/dlt-filter-ctrl.dir/requires: src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-filter-ctrl.c.o.requires
src/console/CMakeFiles/dlt-filter-ctrl.dir/requires: src/console/CMakeFiles/dlt-filter-ctrl.dir/dlt-control-common.c.o.requires

.PHONY : src/console/CMakeFiles/dlt-filter-ctrl.dir/requires

src/console/CMakeFiles/dlt-filter-ctrl.dir/clean:
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console && $(CMAKE_COMMAND) -P CMakeFiles/dlt-filter-ctrl.dir/cmake_clean.cmake
.PHONY : src/console/CMakeFiles/dlt-filter-ctrl.dir/clean

src/console/CMakeFiles/dlt-filter-ctrl.dir/depend:
	cd /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/src/src/console /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console /home/ivilinux/work/01_pfinteg/pf_integ_tools/dn-delivery-common/build-qnx-SDP7/build/tmp/work/x86_64/dlt-daemon/build/src/console/CMakeFiles/dlt-filter-ctrl.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/console/CMakeFiles/dlt-filter-ctrl.dir/depend
