cmake_minimum_required(VERSION 3.10)
project(TrafficSimulator VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add SDL3 installation path
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/libs/SDL3_install")

# Find SDL3
find_package(SDL3 REQUIRED)

# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Main simulator executable
add_executable(simulator
    src/main.cpp
    src/core/Vehicle.cpp
    src/core/Lane.cpp
    src/core/TrafficLight.cpp
    src/managers/TrafficManager.cpp
    src/managers/IntersectionController.cpp
    src/managers/FileHandler.cpp
    src/utils/Queue.cpp
    src/visualization/Renderer.cpp
    src/visualization/DebugOverlay.cpp
)

# Traffic generator executable
add_executable(traffic_generator
    traffic_generator/src/main.cpp
    traffic_generator/src/Generator.cpp
)

# Link SDL3
target_link_libraries(simulator PRIVATE SDL3::SDL3)

# Create data directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data/lanes)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Optional: Add compiler warnings
if(MSVC)
    target_compile_options(simulator PRIVATE /W4)
    target_compile_options(traffic_generator PRIVATE /W4)
else()
    target_compile_options(simulator PRIVATE -Wall -Wextra)
    target_compile_options(traffic_generator PRIVATE -Wall -Wextra)
endif()