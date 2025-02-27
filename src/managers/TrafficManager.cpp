// FILE: src/managers/TrafficManager.cpp
#include "managers/TrafficManager.h"
#include "utils/DebugLogger.h"
#include <sstream>
#include <algorithm>
#include <wchar.h>
#include "core/Constants.h"

TrafficManager::TrafficManager()
    : trafficLight(nullptr),
      fileHandler(nullptr),
      lastFileCheckTime(0),
      lastPriorityUpdateTime(0),
      running(false) {

    DebugLogger::log("TrafficManager created");
}

TrafficManager::~TrafficManager() {
    // Clean up resources
    for (auto* lane : lanes) {
        delete lane;
    }
    lanes.clear();

    if (trafficLight) {
        delete trafficLight;
        trafficLight = nullptr;
    }

    if (fileHandler) {
        delete fileHandler;
        fileHandler = nullptr;
    }

    DebugLogger::log("TrafficManager destroyed");
}

bool TrafficManager::initialize() {
    // Create file handler with consistent path
    fileHandler = new FileHandler(Constants::DATA_PATH);
    if (!fileHandler->initializeFiles()) {
        DebugLogger::log("Failed to initialize lane files", DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Create lanes for each road and lane number
    for (char road : {'A', 'B', 'C', 'D'}) {
        for (int laneNum = 1; laneNum <= 3; laneNum++) {
            Lane* lane = new Lane(road, laneNum);
            lanes.push_back(lane);

            // Add to priority queue with initial priority
            lanePriorityQueue.enqueue(lane, lane->getPriority());
        }
    }

    // Create traffic light
    trafficLight = new TrafficLight();

    std::ostringstream oss;
    oss << "TrafficManager initialized with " << lanes.size() << " lanes";
    DebugLogger::log(oss.str());

    return true;
}

void TrafficManager::start() {
    running = true;
    DebugLogger::log("TrafficManager started");
}

void TrafficManager::stop() {
    running = false;
    DebugLogger::log("TrafficManager stopped");
}

void TrafficManager::update(uint32_t delta) {
    if (!running) return;

    uint32_t currentTime = SDL_GetTicks();

    // Check for new vehicles more frequently (every 200ms)
    if (currentTime - lastFileCheckTime >= 200) {
        readVehicles();
        lastFileCheckTime = currentTime;
    }

    // CRITICAL: Update lane priorities FIRST - this must happen before traffic light updates
    updatePriorities();

    // CRITICAL: Process vehicles based on traffic light state and lane type
    processVehicles(delta);

    // Check for vehicles leaving the simulation
    checkVehicleBoundaries();

    // Update traffic light - AFTER priorities have been updated
    if (trafficLight) {
        trafficLight->update(lanes);
    }

    // Debug log current state
    static uint32_t lastDebugTime = 0;
    if (currentTime - lastDebugTime > 2000) {  // Every 2 seconds
        Lane* priorityLane = findLane('A', 2);
        if (priorityLane) {
            DebugLogger::log("A2 (Priority lane) has " + std::to_string(priorityLane->getVehicleCount()) +
                          " vehicles (Priority: " + std::to_string(priorityLane->getPriority()) + ")",
                          DebugLogger::LogLevel::INFO);
        }

        // Log traffic light state
        if (trafficLight) {
            std::string stateStr;
            switch (trafficLight->getCurrentState()) {
                case TrafficLight::State::ALL_RED: stateStr = "ALL_RED"; break;
                case TrafficLight::State::A_GREEN: stateStr = "A_GREEN"; break;
                case TrafficLight::State::B_GREEN: stateStr = "B_GREEN"; break;
                case TrafficLight::State::C_GREEN: stateStr = "C_GREEN"; break;
                case TrafficLight::State::D_GREEN: stateStr = "D_GREEN"; break;
            }
            DebugLogger::log("Current light state: " + stateStr, DebugLogger::LogLevel::INFO);
        }

        lastDebugTime = currentTime;
    }
}

void TrafficManager::readVehicles() {
    if (!fileHandler) {
        DebugLogger::log("FileHandler not initialized", DebugLogger::LogLevel::ERROR);
        return;
    }

    // Ensure directories exist before reading
    if (!fileHandler->checkFilesExist()) {
        if (!fileHandler->initializeFiles()) {
            DebugLogger::log("Failed to initialize files", DebugLogger::LogLevel::ERROR);
            return;
        }
    }

    // Read new vehicles from files
    std::vector<Vehicle*> newVehicles = fileHandler->readVehiclesFromFiles();

    if (!newVehicles.empty()) {
        std::ostringstream oss;
        oss << "Read " << newVehicles.size() << " new vehicles from files";
        DebugLogger::log(oss.str());

        // Add vehicles to appropriate lanes
        for (auto* vehicle : newVehicles) {
            addVehicle(vehicle);
        }
    }

    // Write status to file periodically for monitoring
    static uint32_t lastStatusTime = 0;
    uint32_t currentTime = SDL_GetTicks();

    if (currentTime - lastStatusTime >= 5000) { // Every 5 seconds
        for (auto* lane : lanes) {
            if (fileHandler && lane->getVehicleCount() > 0) {
                fileHandler->writeLaneStatus(
                    lane->getLaneId(),
                    lane->getLaneNumber(),
                    lane->getVehicleCount(),
                    lane->isPriorityLane() && lane->getPriority() > 0
                );
            }
        }
        lastStatusTime = currentTime;
    }
}

void TrafficManager::addVehicle(Vehicle* vehicle) {
    if (!vehicle) return;

    Lane* targetLane = findLane(vehicle->getLane(), vehicle->getLaneNumber());
    if (targetLane) {
        targetLane->enqueue(vehicle);

        // Log the action
        std::ostringstream oss;
        oss << "Added vehicle " << vehicle->getId() << " to lane "
            << vehicle->getLane() << vehicle->getLaneNumber();
        DebugLogger::log(oss.str());
    } else {
        // Clean up if lane not found
        delete vehicle;
        DebugLogger::log("Error: No matching lane found for vehicle", DebugLogger::LogLevel::ERROR);
    }
}


void TrafficManager::updatePriorities() {
    // CRITICAL: First retrieve the priority lane (A2)
    Lane* priorityLane = nullptr;
    for (auto* lane : lanes) {
        if (lane->getLaneId() == 'A' && lane->getLaneNumber() == 2) {
            priorityLane = lane;
            break;
        }
    }

    if (!priorityLane) {
        DebugLogger::log("ERROR: Priority lane A2 not found!", DebugLogger::LogLevel::ERROR);
        return;
    }

    // CRITICAL: Check if priority condition is met (>10 vehicles in A2)
    int vehicleCount = priorityLane->getVehicleCount();
    int oldPriority = priorityLane->getPriority();

    // PRIORITY CONDITION: A2 lane has more than 10 vehicles
    if (vehicleCount > Constants::PRIORITY_THRESHOLD_HIGH && oldPriority == 0) {
        // Activate priority mode
        priorityLane->updatePriority();  // This will set priority to 100

        DebugLogger::log("*** PRIORITY MODE ACTIVATED: A2 has " + std::to_string(vehicleCount) +
                      " vehicles (>10) ***", DebugLogger::LogLevel::INFO);

        // CRITICAL: Force traffic light to A green if not already
        if (trafficLight && trafficLight->getCurrentState() != TrafficLight::State::A_GREEN) {
            // Set next state to ALL_RED (transitional state)
            trafficLight->setNextState(TrafficLight::State::ALL_RED);
            DebugLogger::log("Forcing light transition to ALL_RED then A_GREEN due to priority mode");
        }
    }
    // Check if we should exit priority mode (<5 vehicles)
    else if (vehicleCount < Constants::PRIORITY_THRESHOLD_LOW && oldPriority > 0) {
        // Deactivate priority mode
        priorityLane->updatePriority();  // This will reset priority to 0

        DebugLogger::log("*** PRIORITY MODE DEACTIVATED: A2 now has " + std::to_string(vehicleCount) +
                      " vehicles (<5) ***", DebugLogger::LogLevel::INFO);
    }

    // CRITICAL: Also log current lane state
    std::ostringstream oss;
    oss << "Lane Status: ";
    for (auto* lane : lanes) {
        if (lane->getVehicleCount() > 0) {
            oss << lane->getName() << ":" << lane->getVehicleCount() << " ";
            if (lane->getPriority() > 0) {
                oss << "(PRIORITY) ";
            }
        }
    }
    DebugLogger::log(oss.str(), DebugLogger::LogLevel::DEBUG);
}


void TrafficManager::processVehicles(uint32_t delta) {
    // Determine which road has green light
    char greenRoad = ' ';
    if (trafficLight) {
        auto state = trafficLight->getCurrentState();
        if (state == TrafficLight::State::A_GREEN) greenRoad = 'A';
        else if (state == TrafficLight::State::B_GREEN) greenRoad = 'B';
        else if (state == TrafficLight::State::C_GREEN) greenRoad = 'C';
        else if (state == TrafficLight::State::D_GREEN) greenRoad = 'D';
    }

    // CRITICAL: Process each lane independently with special rules
    for (auto* lane : lanes) {
        bool isGreenLight = false;

        // RULE 1: If this is lane's road has green light, it can move
        if (lane->getLaneId() == greenRoad) {
            isGreenLight = true;
        }
        // RULE 2: Lane 3 (free lane) can ALWAYS move regardless of traffic light
        else if (lane->getLaneNumber() == 3) {
            isGreenLight = true;  // FREE LANE ALWAYS HAS GREEN LIGHT
        }

        // Get all vehicles in this lane
        const auto& vehicles = lane->getVehicles();
        int queuePos = 0;

        // Update each vehicle
        for (auto* vehicle : vehicles) {
            if (vehicle) {
                // CRITICAL: Update vehicle with correct light status
                vehicle->update(delta, isGreenLight, 0.0f);
                queuePos++;
            }
        }

        // For priority lane A2, log movement status
        if (lane->getLaneId() == 'A' && lane->getLaneNumber() == 2 && !vehicles.empty()) {
            DebugLogger::log("A2 (Priority): " + std::to_string(vehicles.size()) +
                          " vehicles, GreenLight=" + std::to_string(isGreenLight),
                          DebugLogger::LogLevel::DEBUG);
        }

        // For free lanes, verify they're moving
        if (lane->getLaneNumber() == 3 && !vehicles.empty()) {
            DebugLogger::log(lane->getName() + " (Free lane): " +
                          std::to_string(vehicles.size()) + " vehicles, GreenLight=true",
                          DebugLogger::LogLevel::DEBUG);
        }
    }
}

void TrafficManager::checkVehicleBoundaries() {
    for (auto* lane : lanes) {
        // Check each vehicle
        while (!lane->isEmpty()) {
            Vehicle* vehicle = lane->peek();

            if (vehicle && vehicle->hasExited()) {
                // Remove the vehicle from the queue
                Vehicle* removedVehicle = lane->dequeue();

                // Log vehicle exit with lane info
                std::ostringstream oss;
                oss << "Vehicle " << removedVehicle->getId() << " exited the simulation from lane "
                    << removedVehicle->getLane() << removedVehicle->getLaneNumber();
                DebugLogger::log(oss.str());

                // Delete the vehicle
                delete removedVehicle;
            } else {
                // If the first vehicle hasn't exited, the rest haven't either
                break;
            }
        }
    }
}

Lane* TrafficManager::findLane(char laneId, int laneNumber) const {
    for (auto* lane : lanes) {
        if (lane->getLaneId() == laneId && lane->getLaneNumber() == laneNumber) {
            return lane;
        }
    }
    return nullptr;
}

const std::vector<Lane*>& TrafficManager::getLanes() const {
    return lanes;
}

TrafficLight* TrafficManager::getTrafficLight() const {
    return trafficLight;
}

bool TrafficManager::isLanePrioritized(char laneId, int laneNumber) const {
    Lane* lane = findLane(laneId, laneNumber);
    return lane && lane->isPriorityLane() && lane->getPriority() > 0;
}

Lane* TrafficManager::getPriorityLane() const {
    return findLane('A', 2); // AL2 is the priority lane
}

std::string TrafficManager::getStatistics() const {
    std::ostringstream stats;
    stats << "Lane Statistics:\n";
    int totalVehicles = 0;

    for (auto* lane : lanes) {
        int count = lane->getVehicleCount();
        totalVehicles += count;

        stats << lane->getName() << ": " << count << " vehicles";
        if (lane->isPriorityLane() && lane->getPriority() > 0) {
            stats << " (PRIORITY)";
        }
        stats << "\n";
    }

    stats << "Total Vehicles: " << totalVehicles << "\n";

    // Add traffic light status
    if (trafficLight) {
        stats << "Traffic Light: ";
        switch (trafficLight->getCurrentState()) {
            case TrafficLight::State::ALL_RED: stats << "ALL RED"; break;
            case TrafficLight::State::A_GREEN: stats << "A GREEN"; break;
            case TrafficLight::State::B_GREEN: stats << "B GREEN"; break;
            case TrafficLight::State::C_GREEN: stats << "C GREEN"; break;
            case TrafficLight::State::D_GREEN: stats << "D GREEN"; break;
        }
        stats << "\n";
    }

    return stats.str();
}


void TrafficManager::limitVehiclesPerLane() {
    const int MAX_VEHICLES_PER_LANE = 12; // Maximum vehicles allowed in a single lane

    for (auto* lane : lanes) {
        int count = lane->getVehicleCount();

        // If lane has too many vehicles, remove some from the end (farthest from intersection)
        if (count > MAX_VEHICLES_PER_LANE) {
            int toRemove = count - MAX_VEHICLES_PER_LANE;

            DebugLogger::log("Lane " + lane->getName() + " has " + std::to_string(count) +
                           " vehicles (max " + std::to_string(MAX_VEHICLES_PER_LANE) +
                           ") - removing " + std::to_string(toRemove),
                           DebugLogger::LogLevel::WARNING);

            // Get all vehicles
            const auto& vehicles = lane->getVehicles();

            // Remove vehicles starting from the end of the queue (furthest from intersection)
            for (int i = 0; i < toRemove && !lane->isEmpty(); i++) {
                // We have to dequeue all vehicles and re-enqueue except the last one
                std::vector<Vehicle*> tempVehicles;
                int keepCount = count - (i + 1);

                // Dequeue all vehicles except the one to remove
                for (int j = 0; j < keepCount; j++) {
                    if (!lane->isEmpty()) {
                        Vehicle* v = lane->dequeue();
                        if (v) {
                            tempVehicles.push_back(v);
                        }
                    }
                }

                // Now remove the target vehicle
                if (!lane->isEmpty()) {
                    Vehicle* toDelete = lane->dequeue();
                    if (toDelete) {
                        delete toDelete;
                    }
                }

                // Re-enqueue the kept vehicles
                for (auto* v : tempVehicles) {
                    lane->enqueue(v);
                }
            }
        }
    }
}



void TrafficManager::preventVehicleOverlap() {
    // For each lane, check for vehicles that are too close to each other
    for (auto* lane : lanes) {
        const auto& vehicles = lane->getVehicles();

        // Skip if fewer than 2 vehicles
        if (vehicles.size() < 2) continue;

        // Check each vehicle against others in the lane
        for (size_t i = 0; i < vehicles.size() - 1; i++) {
            Vehicle* current = vehicles[i];
            Vehicle* next = vehicles[i+1];

            if (!current || !next) continue;

            // Calculate distance between vehicles
            float dx = current->getTurnPosX() - next->getTurnPosX();
            float dy = current->getTurnPosY() - next->getTurnPosY();
            float distance = std::sqrt(dx*dx + dy*dy);

            // Minimum distance between vehicles
            const float MIN_DISTANCE = 35.0f;

            // If too close, adjust next vehicle position
            if (distance < MIN_DISTANCE) {
                // Normalize direction vector
                float normalize = distance > 0 ? distance : 1.0f;
                dx /= normalize;
                dy /= normalize;

                // Move next vehicle back to maintain spacing
                float moveDistance = MIN_DISTANCE - distance;
                next->setTurnPosX(next->getTurnPosX() - dx * moveDistance);
                next->setTurnPosY(next->getTurnPosY() - dy * moveDistance);
            }
        }
    }
}
