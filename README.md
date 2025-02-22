# DSA Queue Simulator - Traffic Light System


A C++/SDL3 implementation of a traffic light queue management system for COMP202 Data Structures and Algorithms assignment.

## 📋 Description
Simulation of a traffic junction management system using queue data structures. Features:
- Priority-based vehicle queue management
- SDL3 graphical visualization
- Traffic light state management
- Vehicle generation system
- Lane prioritization logic

## 🚀 Features
- Real-time traffic simulation
- Priority lane handling (AL2)
- Normal/High-priority mode switching
- Vehicle queue visualization
- Traffic light state synchronization
- File-based inter-process communication

## 🛠️ Build Instructions

### Prerequisites
- C++17 compatible compiler
- CMake 3.12+
- SDL3 library
- Git

### Build Steps
```bash
# Clone repository with submodules
git clone --recursive https://github.com/strontium-rn/dsa-queue-simulator.git
cd dsa-queue-simulator

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. && cmake --build .

# Run the simulation
cd Debug && ./comp_assignment

```

## Note

You will need to copy .dll file to where your executable (.exe) file is,
bash
```
cd libs/SDL
mkdir build && cd build
cmake ..
cmake --build .
cmake --install . --prefix ../../SDL3_install
```

This is something that you could use but i don't think it's necessary 


## Note Again 

You will need to build SDL and SDL_ttf for it to work 

```bash

# To build SDL

cd libs/SDL
rm -rf build
rm -rf ../SDL3_install
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
cmake --install . --prefix ../../SDL3_install
```

And to build SDL_ttf

```bash
cd ../../SDL_ttf
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DSDL3_DIR="E:/KU/comp_Assignment/libs/SDL3_install/cmake"
cmake --build . --config Release
cmake --install . --prefix ../../SDL3_install
```








