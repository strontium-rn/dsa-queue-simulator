# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.31

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/mac/Desktop/dsa-queue-simulator

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/mac/Desktop/dsa-queue-simulator/build

# Include any dependencies generated for this target.
include CMakeFiles/traffic_generator.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/traffic_generator.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/traffic_generator.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/traffic_generator.dir/flags.make

CMakeFiles/traffic_generator.dir/codegen:
.PHONY : CMakeFiles/traffic_generator.dir/codegen

CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.o: CMakeFiles/traffic_generator.dir/flags.make
CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.o: /Users/mac/Desktop/dsa-queue-simulator/traffic_generator/src/main.cpp
CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.o: CMakeFiles/traffic_generator.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/mac/Desktop/dsa-queue-simulator/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.o -MF CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.o.d -o CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.o -c /Users/mac/Desktop/dsa-queue-simulator/traffic_generator/src/main.cpp

CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/mac/Desktop/dsa-queue-simulator/traffic_generator/src/main.cpp > CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.i

CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/mac/Desktop/dsa-queue-simulator/traffic_generator/src/main.cpp -o CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.s

CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.o: CMakeFiles/traffic_generator.dir/flags.make
CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.o: /Users/mac/Desktop/dsa-queue-simulator/traffic_generator/src/Generator.cpp
CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.o: CMakeFiles/traffic_generator.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/mac/Desktop/dsa-queue-simulator/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.o -MF CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.o.d -o CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.o -c /Users/mac/Desktop/dsa-queue-simulator/traffic_generator/src/Generator.cpp

CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/mac/Desktop/dsa-queue-simulator/traffic_generator/src/Generator.cpp > CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.i

CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/mac/Desktop/dsa-queue-simulator/traffic_generator/src/Generator.cpp -o CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.s

# Object files for target traffic_generator
traffic_generator_OBJECTS = \
"CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.o" \
"CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.o"

# External object files for target traffic_generator
traffic_generator_EXTERNAL_OBJECTS =

traffic_generator: CMakeFiles/traffic_generator.dir/traffic_generator/src/main.cpp.o
traffic_generator: CMakeFiles/traffic_generator.dir/traffic_generator/src/Generator.cpp.o
traffic_generator: CMakeFiles/traffic_generator.dir/build.make
traffic_generator: CMakeFiles/traffic_generator.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/Users/mac/Desktop/dsa-queue-simulator/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable traffic_generator"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/traffic_generator.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/traffic_generator.dir/build: traffic_generator
.PHONY : CMakeFiles/traffic_generator.dir/build

CMakeFiles/traffic_generator.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/traffic_generator.dir/cmake_clean.cmake
.PHONY : CMakeFiles/traffic_generator.dir/clean

CMakeFiles/traffic_generator.dir/depend:
	cd /Users/mac/Desktop/dsa-queue-simulator/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/mac/Desktop/dsa-queue-simulator /Users/mac/Desktop/dsa-queue-simulator /Users/mac/Desktop/dsa-queue-simulator/build /Users/mac/Desktop/dsa-queue-simulator/build /Users/mac/Desktop/dsa-queue-simulator/build/CMakeFiles/traffic_generator.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/traffic_generator.dir/depend

