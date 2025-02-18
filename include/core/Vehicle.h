// include/core/Vehicle.hpp
#pragma once
#include "Constants.h"
#include <cstdint>

class Vehicle {
private:
    uint32_t id;
    Direction direction;
    LaneId currentLane;

public:
    Vehicle(uint32_t id, Direction dir, LaneId lane);
    uint32_t getId() const;
    Direction getDirection() const;
    LaneId getCurrentLane() const;
};
