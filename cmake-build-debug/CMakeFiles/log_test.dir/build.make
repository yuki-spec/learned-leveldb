# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

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
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /users/yifann/research/leveldb/leveldb

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /users/yifann/research/leveldb/leveldb/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/log_test.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/log_test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/log_test.dir/flags.make

CMakeFiles/log_test.dir/util/testharness.cc.o: CMakeFiles/log_test.dir/flags.make
CMakeFiles/log_test.dir/util/testharness.cc.o: ../util/testharness.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/users/yifann/research/leveldb/leveldb/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/log_test.dir/util/testharness.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/log_test.dir/util/testharness.cc.o -c /users/yifann/research/leveldb/leveldb/util/testharness.cc

CMakeFiles/log_test.dir/util/testharness.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/log_test.dir/util/testharness.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /users/yifann/research/leveldb/leveldb/util/testharness.cc > CMakeFiles/log_test.dir/util/testharness.cc.i

CMakeFiles/log_test.dir/util/testharness.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/log_test.dir/util/testharness.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /users/yifann/research/leveldb/leveldb/util/testharness.cc -o CMakeFiles/log_test.dir/util/testharness.cc.s

CMakeFiles/log_test.dir/util/testutil.cc.o: CMakeFiles/log_test.dir/flags.make
CMakeFiles/log_test.dir/util/testutil.cc.o: ../util/testutil.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/users/yifann/research/leveldb/leveldb/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/log_test.dir/util/testutil.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/log_test.dir/util/testutil.cc.o -c /users/yifann/research/leveldb/leveldb/util/testutil.cc

CMakeFiles/log_test.dir/util/testutil.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/log_test.dir/util/testutil.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /users/yifann/research/leveldb/leveldb/util/testutil.cc > CMakeFiles/log_test.dir/util/testutil.cc.i

CMakeFiles/log_test.dir/util/testutil.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/log_test.dir/util/testutil.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /users/yifann/research/leveldb/leveldb/util/testutil.cc -o CMakeFiles/log_test.dir/util/testutil.cc.s

CMakeFiles/log_test.dir/db/log_test.cc.o: CMakeFiles/log_test.dir/flags.make
CMakeFiles/log_test.dir/db/log_test.cc.o: ../db/log_test.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/users/yifann/research/leveldb/leveldb/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/log_test.dir/db/log_test.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/log_test.dir/db/log_test.cc.o -c /users/yifann/research/leveldb/leveldb/db/log_test.cc

CMakeFiles/log_test.dir/db/log_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/log_test.dir/db/log_test.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /users/yifann/research/leveldb/leveldb/db/log_test.cc > CMakeFiles/log_test.dir/db/log_test.cc.i

CMakeFiles/log_test.dir/db/log_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/log_test.dir/db/log_test.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /users/yifann/research/leveldb/leveldb/db/log_test.cc -o CMakeFiles/log_test.dir/db/log_test.cc.s

# Object files for target log_test
log_test_OBJECTS = \
"CMakeFiles/log_test.dir/util/testharness.cc.o" \
"CMakeFiles/log_test.dir/util/testutil.cc.o" \
"CMakeFiles/log_test.dir/db/log_test.cc.o"

# External object files for target log_test
log_test_EXTERNAL_OBJECTS =

log_test: CMakeFiles/log_test.dir/util/testharness.cc.o
log_test: CMakeFiles/log_test.dir/util/testutil.cc.o
log_test: CMakeFiles/log_test.dir/db/log_test.cc.o
log_test: CMakeFiles/log_test.dir/build.make
log_test: libleveldb.a
log_test: CMakeFiles/log_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/users/yifann/research/leveldb/leveldb/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX executable log_test"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/log_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/log_test.dir/build: log_test

.PHONY : CMakeFiles/log_test.dir/build

CMakeFiles/log_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/log_test.dir/cmake_clean.cmake
.PHONY : CMakeFiles/log_test.dir/clean

CMakeFiles/log_test.dir/depend:
	cd /users/yifann/research/leveldb/leveldb/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /users/yifann/research/leveldb/leveldb /users/yifann/research/leveldb/leveldb /users/yifann/research/leveldb/leveldb/cmake-build-debug /users/yifann/research/leveldb/leveldb/cmake-build-debug /users/yifann/research/leveldb/leveldb/cmake-build-debug/CMakeFiles/log_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/log_test.dir/depend

