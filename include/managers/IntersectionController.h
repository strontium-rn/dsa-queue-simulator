// include/managers/IntersectionController.h
#pragma once
#include "core/Constants.h"
#include "core/Lane.h"
#include "utils/PriorityQueue.h"
#include <memory>
#include <vector>

class IntersectionController {
private:
    PriorityQueue<LaneId> laneQueue;
    std::vector<std::unique_ptr<Lane>>& lanes;
    bool isPriorityMode;
    float stateTimer;
    float vehicleProcessTime;

    // Helper methods
    void updateLanePriorities();
    float calculateAverageWaitingVehicles() const;
    void processVehicles(size_t count);
    void handlePriorityLane();

public:
    IntersectionController(std::vector<std::unique_ptr<Lane>>& lanes);
    void update(float deltaTime);
    bool isInPriorityMode() const { return isPriorityMode; }
    void setVehicleProcessTime(float time) { vehicleProcessTime = time; }
};


