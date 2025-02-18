// include/core/Constants.h
#pragma once
#include <cstdint>

enum class LaneId {
    AL1_INCOMING,
    AL2_PRIORITY,
    AL3_FREELANE,
    BL1_INCOMING,
    BL2_NORMAL,
    BL3_FREELANE,
    CL1_INCOMING,
    CL2_NORMAL,
    CL3_FREELANE,
    DL1_INCOMING,
    DL2_NORMAL,
    DL3_FREELANE
};

enum class Direction {
    STRAIGHT,
    LEFT,
    RIGHT
};

enum class LightState {
    RED,
    GREEN
};

// Add operator== for LightState
inline bool operator==(LightState lhs, LightState rhs) {
    return static_cast<int>(lhs) == static_cast<int>(rhs);
}
