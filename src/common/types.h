// src/common/types.h
#pragma once
#include <cstdint>
#include <string>

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

enum class LightState {
    RED,
    GREEN
};

;
