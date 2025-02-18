// Lane.h
#pragma once
#include "Vehicle.h"
#include "utils/Queue.h"
#include <memory>
#include <string>

class Lane {
private:
    LaneId id;
    Queue<std::shared_ptr<Vehicle>> vehicleQueue;
    bool isPriority;
    std::string dataFile;

public:
    Lane(LaneId id, bool isPriority);

    void addVehicle(std::shared_ptr<Vehicle> vehicle);
    std::shared_ptr<Vehicle> removeVehicle();
    Direction getVehicleDirection(size_t index) const;  // Add this method
    size_t getQueueSize() const;
    bool isPriorityLane() const;
    LaneId getId() const;
    const std::string& getDataFile() const;
    void update();
};
