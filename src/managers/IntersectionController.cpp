// src/managers/IntersectionController.cpp
#include "managers/IntersectionController.h"
#include <numeric>
#include <algorithm>
#include <cmath>

IntersectionController::IntersectionController(std::vector<std::unique_ptr<Lane>>& lanes)
    : lanes(lanes)
    , isPriorityMode(false)
    , stateTimer(0.0f)
    , vehicleProcessTime(2.0f) {
    // Initialize lane queue with default priorities
    for (const auto& lane : lanes) {
        if (lane->getId() != LaneId::AL3_FREELANE &&
            lane->getId() != LaneId::BL3_FREELANE &&
            lane->getId() != LaneId::CL3_FREELANE &&
            lane->getId() != LaneId::DL3_FREELANE) {
            laneQueue.enqueuePriority(lane->getId(), 1);
        }
    }
}

void IntersectionController::update(float deltaTime) {
    stateTimer += deltaTime;

    // Update lane priorities based on waiting vehicles
    updateLanePriorities();

    // Check priority conditions
    auto priorityLane = std::find_if(lanes.begin(), lanes.end(),
        [](const auto& lane) {
            return lane->isPriorityLane() && lane->getQueueSize() > 10;
        });

    if (priorityLane != lanes.end()) {
        isPriorityMode = true;
        handlePriorityLane();
    } else {
        isPriorityMode = false;
        if (stateTimer >= vehicleProcessTime) {
            float avgVehicles = calculateAverageWaitingVehicles();
            processVehicles(static_cast<size_t>(std::ceil(avgVehicles)));
            stateTimer = 0.0f;
        }
    }

    // Process free lanes continuously
    for (auto& lane : lanes) {
        if (lane->getId() == LaneId::AL3_FREELANE ||
            lane->getId() == LaneId::BL3_FREELANE ||
            lane->getId() == LaneId::CL3_FREELANE ||
            lane->getId() == LaneId::DL3_FREELANE) {
            while (lane->getQueueSize() > 0) {
                lane->removeVehicle();
            }
        }
    }
}

void IntersectionController::updateLanePriorities() {
    // Clear and update lane queue
    while (!laneQueue.isEmpty()) {
        laneQueue.dequeue();
    }

    for (const auto& lane : lanes) {
        LaneId id = lane->getId();
        if (id != LaneId::AL3_FREELANE &&
            id != LaneId::BL3_FREELANE &&
            id != LaneId::CL3_FREELANE &&
            id != LaneId::DL3_FREELANE) {

            int priority = 1;
            if (lane->isPriorityLane() && lane->getQueueSize() > 5) {
                priority = 3;
            } else if (lane->getQueueSize() > 8) {
                priority = 2;
            }

            laneQueue.enqueuePriority(id, priority);
        }
    }
}

float IntersectionController::calculateAverageWaitingVehicles() const {
    size_t totalVehicles = 0;
    size_t normalLaneCount = 0;

    for (const auto& lane : lanes) {
        if (!lane->isPriorityLane() &&
            lane->getId() != LaneId::AL3_FREELANE &&
            lane->getId() != LaneId::BL3_FREELANE &&
            lane->getId() != LaneId::CL3_FREELANE &&
            lane->getId() != LaneId::DL3_FREELANE) {
            totalVehicles += lane->getQueueSize();
            normalLaneCount++;
        }
    }

    return normalLaneCount > 0 ?
        static_cast<float>(totalVehicles) / static_cast<float>(normalLaneCount) : 0.0f;
}

void IntersectionController::processVehicles(size_t count) {
    if (count == 0) return;

    // Process vehicles based on priority queue
    std::vector<LaneId> processedLanes;
    while (!laneQueue.isEmpty() && processedLanes.size() < count) {
        LaneId currentLaneId = laneQueue.dequeue();

        auto laneIt = std::find_if(lanes.begin(), lanes.end(),
            [currentLaneId](const auto& lane) {
                return lane->getId() == currentLaneId;
            });

        if (laneIt != lanes.end() && (*laneIt)->getQueueSize() > 0) {
            (*laneIt)->removeVehicle();
            processedLanes.push_back(currentLaneId);
        }
    }
}

void IntersectionController::handlePriorityLane() {
    for (auto& lane : lanes) {
        if (lane->isPriorityLane()) {
            while (lane->getQueueSize() > 5) {
                lane->removeVehicle();
            }
            break;
        }
    }
}

