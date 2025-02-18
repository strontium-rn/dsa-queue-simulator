// include/cor

// src/core/Vehicle.cpp
#include "core/Vehicle.h"

Vehicle::Vehicle(uint32_t id, Direction dir, LaneId lane)
    : id(id), direction(dir), currentLane(lane) {}

uint32_t Vehicle::getId() const { return id; }
Direction Vehicle::getDirection() const { return direction; }
LaneId Vehicle::getCurrentLane() const { return currentLane; }

