// FILE: src/core/Vehicle.cpp
#include "core/Vehicle.h"
#include "core/Constants.h"
#include "utils/DebugLogger.h"
#include <cmath>
#include <sstream>
#include <random> // Add this for random number generation

Vehicle::Vehicle(const std::string& id, char lane, int laneNumber, bool isEmergency)
    : id(id),
      lane(lane),
      laneNumber(laneNumber),
      isEmergency(isEmergency),
      arrivalTime(time(nullptr)),
      animPos(0.0f),
      turning(false),
      turnProgress(0.0f),
      turnPosX(0.0f),
      turnPosY(0.0f),
      queuePos(0),
      destination(Destination::STRAIGHT),
      currentDirection(Direction::DOWN),
      state(VehicleState::APPROACHING),
      currentWaypoint(0) {

    // Log creation
    std::ostringstream oss;
    oss << "Created vehicle " << id << " in lane " << lane << laneNumber;
    DebugLogger::log(oss.str());

    // Window dimensions
    const int windowWidth = 800;
    const int windowHeight = 800;
    const int centerX = windowWidth / 2;
    const int centerY = windowHeight / 2;

    // Determine current direction based on road (lane letter)
    // A is North (top), B is East (right), C is South (bottom), D is West (left)
    switch (lane) {
        case 'A':
            currentDirection = Direction::DOWN;  // Road A (North) vehicles move DOWN
            break;
        case 'B':
            currentDirection = Direction::LEFT;  // Road B (East) vehicles move LEFT
            break;
        case 'C':
            currentDirection = Direction::UP;    // Road C (South) vehicles move UP
            break;
        case 'D':
            currentDirection = Direction::RIGHT; // Road D (West) vehicles move RIGHT
            break;
        default:
            DebugLogger::log("Invalid lane ID: " + std::string(1, lane), DebugLogger::LogLevel::ERROR);
            currentDirection = Direction::DOWN;
            break;
    }

    // Lane spacing - wider for better visibility
    const float laneWidth = 50.0f;
    const float laneOffset = 50.0f;
    const float roadWidth = 150.0f;

    // Set initial position based on lane and laneNumber (1, 2, or 3)
    // IMPORTANT: We position vehicles according to ONE-WAY lanes on each road
    switch (lane) {
        case 'A': // North road (top)
            // Lane 1 is not spawned (it's incoming)
            // Lane 2 is middle lane - can go straight or left
            // Lane 3 is rightmost lane - always turns left
            switch (laneNumber) {
                case 2:
                    turnPosX = centerX; // Center lane
                    break;
                case 3:
                    turnPosX = centerX + laneOffset; // Right lane
                    break;
                default:
                    turnPosX = centerX; // Default to center if invalid
                    DebugLogger::log("Invalid lane number for Road A: " + std::to_string(laneNumber), DebugLogger::LogLevel::WARNING);
                    break;
            }
            turnPosY = 20.0f; // Near top of screen
            break;

        case 'B': // East road (right)
            switch (laneNumber) {
                case 2:
                    turnPosY = centerY; // Center lane
                    break;
                case 3:
                    turnPosY = centerY + laneOffset; // Bottom lane
                    break;
                default:
                    turnPosY = centerY; // Default to center if invalid
                    DebugLogger::log("Invalid lane number for Road B: " + std::to_string(laneNumber), DebugLogger::LogLevel::WARNING);
                    break;
            }
            turnPosX = windowWidth - 20.0f; // Near right edge of screen
            break;

        case 'C': // South road (bottom)
            switch (laneNumber) {
                case 2:
                    turnPosX = centerX; // Center lane
                    break;
                case 3:
                    turnPosX = centerX - laneOffset; // Left lane
                    break;
                default:
                    turnPosX = centerX; // Default to center if invalid
                    DebugLogger::log("Invalid lane number for Road C: " + std::to_string(laneNumber), DebugLogger::LogLevel::WARNING);
                    break;
            }
            turnPosY = windowHeight - 20.0f; // Near bottom of screen
            break;

        case 'D': // West road (left)
            switch (laneNumber) {
                case 2:
                    turnPosY = centerY; // Center lane
                    break;
                case 3:
                    turnPosY = centerY - laneOffset; // Top lane
                    break;
                default:
                    turnPosY = centerY; // Default to center if invalid
                    DebugLogger::log("Invalid lane number for Road D: " + std::to_string(laneNumber), DebugLogger::LogLevel::WARNING);
                    break;
            }
            turnPosX = 20.0f; // Near left edge of screen
            break;
    }

    // Set initial animation position
    animPos = (currentDirection == Direction::UP || currentDirection == Direction::DOWN) ?
              turnPosY : turnPosX;

    // Determine destination based on lane number following the assignment rules
    if (laneNumber == 3) {
        // Lane 3 (L3) always turns left
        destination = Destination::LEFT;
        std::string msg = "Vehicle " + id + " on lane " + lane + std::to_string(laneNumber) + " will turn LEFT (free lane rule)";
        DebugLogger::log(msg);
    }
    else if (laneNumber == 2) {
        // Lane 2 (L2) can go straight or left (not right)
        // Check for direction indication in ID
        if (id.find("_LEFT") != std::string::npos) {
            destination = Destination::LEFT;
        } else if (id.find("_STRAIGHT") != std::string::npos) {
            destination = Destination::STRAIGHT;
        } else {
            // Default behavior: 60% straight, 40% left
            int idHash = 0;
            for (char c : id) idHash += c;
            destination = (idHash % 10 < 6) ? Destination::STRAIGHT : Destination::LEFT;
        }

        std::string destStr = (destination == Destination::STRAIGHT) ? "STRAIGHT" : "LEFT";
        DebugLogger::log("Vehicle " + id + " on lane " + lane + std::to_string(laneNumber) + " will go " + destStr);
    }
    else if (laneNumber == 1) {
        // Lane 1 (L1) is incoming lane (vehicles don't spawn here)
        destination = Destination::STRAIGHT;
        DebugLogger::log("WARNING: Vehicle " + id + " created in lane " + lane + "1 (incoming lane)");
    }

    // Initialize waypoints for movement
    initializeWaypoints();
}

Vehicle::~Vehicle() {
    std::ostringstream oss;
    oss << "Destroyed vehicle " << id;
    DebugLogger::log(oss.str());
}

void Vehicle::initializeWaypoints() {
    // Window dimensions
    const int windowWidth = 800;
    const int windowHeight = 800;
    const int centerX = windowWidth / 2;
    const int centerY = windowHeight / 2;

    // Clear existing waypoints
    waypoints.clear();

    // Adjust intersection boundaries
    const float intersectionHalf = 70.0f; // Intersection size
    const float leftEdge = centerX - intersectionHalf;
    const float rightEdge = centerX + intersectionHalf;
    const float topEdge = centerY - intersectionHalf;
    const float bottomEdge = centerY + intersectionHalf;

    // CRITICAL: Lane offsets for proper alignment
    // These define the x/y positions of lanes for proper alignment
    const float laneOffset = Constants::LANE_WIDTH;

    // L1 (incoming lane) should be offset inward from center
    const float lane1Offset = -20.0f;

    // L2 is in the center of each road
    const float lane2Offset = 0.0f;

    // L3 (free lane) should be offset outward from center
    const float lane3Offset = 20.0f;

    // Add the starting position as first waypoint
    waypoints.push_back({turnPosX, turnPosY});

    // Add approach to intersection waypoint
    switch (currentDirection) {
        case Direction::DOWN: // From North (A)
            waypoints.push_back({turnPosX, topEdge - 5.0f});
            break;
        case Direction::UP: // From South (C)
            waypoints.push_back({turnPosX, bottomEdge + 5.0f});
            break;
        case Direction::LEFT: // From East (B)
            waypoints.push_back({rightEdge + 5.0f, turnPosY});
            break;
        case Direction::RIGHT: // From West (D)
            waypoints.push_back({leftEdge - 5.0f, turnPosY});
            break;
    }

    // CRITICAL: Strictly follow the assignment routing requirements:
    // AL3 → BL1; AL2: Straight → CL1, Left → DL1
    // BL3 → CL1; BL2: Straight → DL1, Left → AL1
    // CL3 → DL1; CL2: Straight → AL1, Left → BL1
    // DL3 → AL1; DL2: Straight → BL1, Left → CL1

    // CRITICAL: Before initializing, log the vehicle's details
    std::ostringstream debugLog;
    debugLog << "Routing vehicle " << id << " from " << lane << laneNumber;
    if (laneNumber == 3) {
        debugLog << " (free lane, always turns LEFT)";
    } else if (laneNumber == 2) {
        debugLog << " going " << (destination == Destination::LEFT ? "LEFT" : "STRAIGHT");
    }
    DebugLogger::log(debugLog.str(), DebugLogger::LogLevel::ERROR);

    if (laneNumber == 3) {
        // CRITICAL: Lane 3 MUST ALWAYS turn left to L1
        switch (currentDirection) {
            case Direction::DOWN:  // A(North) to B(East) - AL3 → BL1
                // First intermediate point inside intersection
                waypoints.push_back({centerX, topEdge + 20.0f});

                // Second intermediate point
                waypoints.push_back({rightEdge - 20.0f, centerY});

                // CRITICAL: Exit precisely in BL1
                waypoints.push_back({rightEdge + 5.0f, centerY + lane1Offset});

                // Off screen
                waypoints.push_back({windowWidth + 30.0f, centerY + lane1Offset});

                DebugLogger::log("AL3 route: LEFT to BL1", DebugLogger::LogLevel::ERROR);
                break;

            case Direction::UP:    // C(South) to D(West) - CL3 → DL1
                // First intermediate point
                waypoints.push_back({centerX, bottomEdge - 20.0f});

                // Second intermediate point
                waypoints.push_back({leftEdge + 20.0f, centerY});

                // CRITICAL: Exit precisely in DL1
                waypoints.push_back({leftEdge - 5.0f, centerY + lane1Offset});

                // Off screen
                waypoints.push_back({-30.0f, centerY + lane1Offset});

                DebugLogger::log("CL3 route: LEFT to DL1", DebugLogger::LogLevel::ERROR);
                break;

            case Direction::LEFT:  // B(East) to C(South) - BL3 → CL1
                // First intermediate point
                waypoints.push_back({rightEdge - 20.0f, centerY});

                // Second intermediate point
                waypoints.push_back({centerX, bottomEdge - 20.0f});

                // CRITICAL: Exit precisely in CL1
                waypoints.push_back({centerX + lane1Offset, bottomEdge + 5.0f});

                // Off screen
                waypoints.push_back({centerX + lane1Offset, windowHeight + 30.0f});

                DebugLogger::log("BL3 route: LEFT to CL1", DebugLogger::LogLevel::ERROR);
                break;

            case Direction::RIGHT: // D(West) to A(North) - DL3 → AL1
                // First intermediate point
                waypoints.push_back({leftEdge + 20.0f, centerY});

                // Second intermediate point
                waypoints.push_back({centerX, topEdge + 20.0f});

                // CRITICAL: Exit precisely in AL1
                waypoints.push_back({centerX + lane1Offset, topEdge - 5.0f});

                // Off screen
                waypoints.push_back({centerX + lane1Offset, -30.0f});

                DebugLogger::log("DL3 route: LEFT to AL1", DebugLogger::LogLevel::ERROR);
                break;
        }
    }
    else if (laneNumber == 2) {
        if (destination == Destination::STRAIGHT) {
            // CRITICAL: L2 straight must go to L1 (incoming lane)
            switch (currentDirection) {
                case Direction::DOWN:  // A(North) to C(South) - AL2(straight) → CL1
                    // Through intersection
                    waypoints.push_back({centerX, centerY});

                    // CRITICAL: Exit precisely in CL1
                    waypoints.push_back({centerX + lane1Offset, bottomEdge + 5.0f});

                    // Off screen
                    waypoints.push_back({centerX + lane1Offset, windowHeight + 30.0f});

                    DebugLogger::log("AL2 route: STRAIGHT to CL1", DebugLogger::LogLevel::ERROR);
                    break;

                case Direction::UP:    // C(South) to A(North) - CL2(straight) → AL1
                    // Through intersection
                    waypoints.push_back({centerX, centerY});

                    // CRITICAL: Exit precisely in AL1
                    waypoints.push_back({centerX + lane1Offset, topEdge - 5.0f});

                    // Off screen
                    waypoints.push_back({centerX + lane1Offset, -30.0f});

                    DebugLogger::log("CL2 route: STRAIGHT to AL1", DebugLogger::LogLevel::ERROR);
                    break;

                case Direction::LEFT:  // B(East) to D(West) - BL2(straight) → DL1
                    // Through intersection
                    waypoints.push_back({centerX, centerY});

                    // CRITICAL: Exit precisely in DL1
                    waypoints.push_back({leftEdge - 5.0f, centerY + lane1Offset});

                    // Off screen
                    waypoints.push_back({-30.0f, centerY + lane1Offset});

                    DebugLogger::log("BL2 route: STRAIGHT to DL1", DebugLogger::LogLevel::ERROR);
                    break;

                case Direction::RIGHT: // D(West) to B(East) - DL2(straight) → BL1
                    // Through intersection
                    waypoints.push_back({centerX, centerY});

                    // CRITICAL: Exit precisely in BL1
                    waypoints.push_back({rightEdge + 5.0f, centerY + lane1Offset});

                    // Off screen
                    waypoints.push_back({windowWidth + 30.0f, centerY + lane1Offset});

                    DebugLogger::log("DL2 route: STRAIGHT to BL1", DebugLogger::LogLevel::ERROR);
                    break;
            }
        }
        else if (destination == Destination::LEFT) {
            // CRITICAL: L2 left turn must go to L1 (incoming lane)
            switch (currentDirection) {
                case Direction::DOWN:  // A(North) to D(West) - AL2(left) → DL1
                    // First turn point
                    waypoints.push_back({centerX, topEdge + 20.0f});

                    // Second turn point
                    waypoints.push_back({leftEdge + 20.0f, centerY});

                    // CRITICAL: Exit precisely in DL1
                    waypoints.push_back({leftEdge - 5.0f, centerY + lane1Offset});

                    // Off screen
                    waypoints.push_back({-30.0f, centerY + lane1Offset});

                    DebugLogger::log("AL2 route: LEFT to DL1", DebugLogger::LogLevel::ERROR);
                    break;

                case Direction::UP:    // C(South) to B(East) - CL2(left) → BL1
                    // First turn point
                    waypoints.push_back({centerX, bottomEdge - 20.0f});

                    // Second turn point
                    waypoints.push_back({rightEdge - 20.0f, centerY});

                    // CRITICAL: Exit precisely in BL1
                    waypoints.push_back({rightEdge + 5.0f, centerY + lane1Offset});

                    // Off screen
                    waypoints.push_back({windowWidth + 30.0f, centerY + lane1Offset});

                    DebugLogger::log("CL2 route: LEFT to BL1", DebugLogger::LogLevel::ERROR);
                    break;

                case Direction::LEFT:  // B(East) to A(North) - BL2(left) → AL1
                    // First turn point
                    waypoints.push_back({rightEdge - 20.0f, centerY});

                    // Second turn point
                    waypoints.push_back({centerX, topEdge + 20.0f});

                    // CRITICAL: Exit precisely in AL1
                    waypoints.push_back({centerX + lane1Offset, topEdge - 5.0f});

                    // Off screen
                    waypoints.push_back({centerX + lane1Offset, -30.0f});

                    DebugLogger::log("BL2 route: LEFT to AL1", DebugLogger::LogLevel::ERROR);
                    break;

                case Direction::RIGHT: // D(West) to C(South) - DL2(left) → CL1
                    // First turn point
                    waypoints.push_back({leftEdge + 20.0f, centerY});

                    // Second turn point
                    waypoints.push_back({centerX, bottomEdge - 20.0f});

                    // CRITICAL: Exit precisely in CL1
                    waypoints.push_back({centerX + lane1Offset, bottomEdge + 5.0f});

                    // Off screen
                    waypoints.push_back({centerX + lane1Offset, windowHeight + 30.0f});

                    DebugLogger::log("DL2 route: LEFT to CL1", DebugLogger::LogLevel::ERROR);
                    break;
            }
        }
    }

    // Set current waypoint index
    currentWaypoint = 0;
    turning = false;

    // CRITICAL: log the total waypoints for debugging
    DebugLogger::log("Vehicle " + id + " initialized with " +
                   std::to_string(waypoints.size()) + " waypoints");
}

std::string Vehicle::getId() const {
    return id;
}

char Vehicle::getLane() const {
    return lane;
}

void Vehicle::setLane(char lane) {
    this->lane = lane;
}

int Vehicle::getLaneNumber() const {
    return laneNumber;
}

void Vehicle::setLaneNumber(int number) {
    this->laneNumber = number;
}

bool Vehicle::isEmergencyVehicle() const {
    return isEmergency;
}

time_t Vehicle::getArrivalTime() const {
    return arrivalTime;
}

float Vehicle::getAnimationPos() const {
    return animPos;
}

void Vehicle::setAnimationPos(float pos) {
    this->animPos = pos;
}

bool Vehicle::isTurning() const {
    return turning;
}

void Vehicle::setTurning(bool turning) {
    this->turning = turning;
}

float Vehicle::getTurnProgress() const {
    return turnProgress;
}

void Vehicle::setTurnProgress(float progress) {
    this->turnProgress = progress;
}

float Vehicle::getTurnPosX() const {
    return turnPosX;
}

void Vehicle::setTurnPosX(float x) {
    this->turnPosX = x;
}

float Vehicle::getTurnPosY() const {
    return turnPosY;
}

void Vehicle::setTurnPosY(float y) {
    this->turnPosY = y;
}

void Vehicle::setDestination(Destination dest) {
    if (this->destination != dest) {
        this->destination = dest;

        // When destination changes, reinitialize waypoints to update the path
        initializeWaypoints();

        // Log the destination change
        std::ostringstream oss;
        std::string destStr;
        switch (dest) {
            case Destination::STRAIGHT: destStr = "STRAIGHT"; break;
            case Destination::LEFT: destStr = "LEFT"; break;
            case Destination::RIGHT: destStr = "RIGHT"; break;
        }
        oss << "Vehicle " << id << " destination set to " << destStr;
        DebugLogger::log(oss.str());
    }
}

Destination Vehicle::getDestination() const {
    return destination;
}

float Vehicle::easeInOutQuad(float t) const {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

void Vehicle::update(uint32_t delta, bool isGreenLight, float targetPos) {
    // CRITICAL FIX: Free lane vehicles (L3) can ALWAYS move regardless of traffic light
    bool canMove = isGreenLight;

    // L3 is free lane - ALWAYS allowed to move
    if (laneNumber == 3) {
        canMove = true;

        // Debug log for free lane
        static uint32_t lastLogTime = 0;
        uint32_t currentTime = SDL_GetTicks();
        if (currentTime - lastLogTime > 3000) {
DebugLogger::log("FREE LANE (" + std::string(1, lane) + "3): Vehicle " + id + " moving freely",
               DebugLogger::LogLevel::ERROR);
            lastLogTime = currentTime;
        }
    }

    // DEBUG: Log A2 priority lane status
    if (lane == 'A' && laneNumber == 2) {
        static uint32_t lastLogTime = 0;
        uint32_t currentTime = SDL_GetTicks();
        if (currentTime - lastLogTime > 3000) {
            DebugLogger::log("PRIORITY LANE (A2): Vehicle " + id + " canMove=" +
                         (canMove ? "true" : "false"), DebugLogger::LogLevel::ERROR);
            lastLogTime = currentTime;
        }
    }

    // Fine-tune speed for smoother animation
    const float SPEED_BASE = 0.018f;
    const float SPEED = SPEED_BASE * delta;
    const float VEHICLE_SPACING = 50.0f; // Increased from 35.0f for better separation

    if (canMove) {
        // We have more waypoints to travel
        if (currentWaypoint < waypoints.size() - 1) {
            // Get current and next waypoint
            auto& current = waypoints[currentWaypoint];
            auto& next = waypoints[currentWaypoint + 1];

            // Calculate direction vector
            float dx = next.x - turnPosX;
            float dy = next.y - turnPosY;

            // Calculate distance to next waypoint
            float distance = std::sqrt(dx*dx + dy*dy);

            // If close enough to waypoint, move to next
            if (distance < 3.0f) {
                currentWaypoint++;

                // Log progress through waypoints for debugging
                if (laneNumber == 3 || (lane == 'A' && laneNumber == 2)) {
                    DebugLogger::log("Vehicle " + id + " on " + lane + std::to_string(laneNumber) +
                                 " reached waypoint " + std::to_string(currentWaypoint) +
                                 " of " + std::to_string(waypoints.size()),
                                 DebugLogger::LogLevel::DEBUG);
                }

                // For L3 (always turns left) and L2 (turns left if specified)
                if ((laneNumber == 3) ||
                    (laneNumber == 2 && destination == Destination::LEFT)) {

                    // When entering turning points (varies by direction)
                    if (currentWaypoint == 2) {
                        turning = true;
                        turnProgress = 0.0f;
                        state = VehicleState::IN_INTERSECTION;

                        // Log turn start
                        std::ostringstream oss;
                        oss << "Vehicle " << id << " on " << lane << laneNumber << " is now turning LEFT";
                        DebugLogger::log(oss.str(), DebugLogger::LogLevel::ERROR);
                    }
                }

                // Determine when a vehicle has exited the intersection
                bool isExiting = false;

                if (laneNumber == 3) {
                    // L3 vehicles typically reach exit point at waypoint 3
                    isExiting = (currentWaypoint == 3);
                } else if (laneNumber == 2) {
                    if (destination == Destination::LEFT) {
                        // L2 turning left typically reaches exit at waypoint 3
                        isExiting = (currentWaypoint == 3);
                    } else {
                        // L2 going straight typically reaches exit at waypoint 2
                        isExiting = (currentWaypoint == 2);
                    }
                }

                // Update vehicle state when exiting
                if (isExiting) {
                    turning = false;
                    state = VehicleState::EXITING;

                    // CRITICAL: Ensure the lane assignments strictly follow the rules
                    std::string newLaneStr;
                    switch (currentDirection) {
                        case Direction::DOWN:  // From North (A)
                            if (laneNumber == 3) {
                                // AL3 → BL1
                                lane = 'B';
                                laneNumber = 1;
                                currentDirection = Direction::LEFT;
                                newLaneStr = "B1 (turned LEFT from A3)";
                            }
                            else if (destination == Destination::LEFT) {
                                // AL2(left) → DL1
                                lane = 'D';
                                laneNumber = 1;
                                currentDirection = Direction::RIGHT;
                                newLaneStr = "D1 (turned LEFT from A2)";
                            }
                            else {
                                // AL2(straight) → CL1
                                lane = 'C';
                                laneNumber = 1;
                                currentDirection = Direction::UP;
                                newLaneStr = "C1 (going STRAIGHT from A2)";
                            }
                            break;

                        case Direction::UP:    // From South (C)
                            if (laneNumber == 3) {
                                // CL3 → DL1
                                lane = 'D';
                                laneNumber = 1;
                                currentDirection = Direction::RIGHT;
                                newLaneStr = "D1 (turned LEFT from C3)";
                            }
                            else if (destination == Destination::LEFT) {
                                // CL2(left) → BL1
                                lane = 'B';
                                laneNumber = 1;
                                currentDirection = Direction::LEFT;
                                newLaneStr = "B1 (turned LEFT from C2)";
                            }
                            else {
                                // CL2(straight) → AL1
                                lane = 'A';
                                laneNumber = 1;
                                currentDirection = Direction::DOWN;
                                newLaneStr = "A1 (going STRAIGHT from C2)";
                            }
                            break;

                        case Direction::LEFT:  // From East (B)
                            if (laneNumber == 3) {
                                // BL3 → CL1
                                lane = 'C';
                                laneNumber = 1;
                                currentDirection = Direction::UP;
                                newLaneStr = "C1 (turned LEFT from B3)";
                            }
                            else if (destination == Destination::LEFT) {
                                // BL2(left) → AL1
                                lane = 'A';
                                laneNumber = 1;
                                currentDirection = Direction::DOWN;
                                newLaneStr = "A1 (turned LEFT from B2)";
                            }
                            else {
                                // BL2(straight) → DL1
                                lane = 'D';
                                laneNumber = 1;
                                currentDirection = Direction::RIGHT;
                                newLaneStr = "D1 (going STRAIGHT from B2)";
                            }
                            break;

                        case Direction::RIGHT: // From West (D)
                            if (laneNumber == 3) {
                                // DL3 → AL1
                                lane = 'A';
                                laneNumber = 1;
                                currentDirection = Direction::DOWN;
                                newLaneStr = "A1 (turned LEFT from D3)";
                            }
                            else if (destination == Destination::LEFT) {
                                // DL2(left) → CL1
                                lane = 'C';
                                laneNumber = 1;
                                currentDirection = Direction::UP;
                                newLaneStr = "C1 (turned LEFT from D2)";
                            }
                            else {
                                // DL2(straight) → BL1
                                lane = 'B';
                                laneNumber = 1;
                                currentDirection = Direction::LEFT;
                                newLaneStr = "B1 (going STRAIGHT from D2)";
                            }
                            break;
                    }

                    // Log lane change
                    DebugLogger::log("==================== Vehicle " + id + " now on " + newLaneStr +
                                  " ====================", DebugLogger::LogLevel::ERROR);
                }
            }

            // Adjust speed based on position and turn status
            float adjustedSpeed = SPEED;

            // Slower when approaching intersection
            if (currentWaypoint == 1) {
                adjustedSpeed *= 0.9f;
            }
            // Even slower in turning phase
            else if (turning) {
                adjustedSpeed *= 0.7f;
            }
            // Faster when exiting intersection
            else if (currentWaypoint >= 3) {
                adjustedSpeed *= 1.2f;
            }

            // Move toward next waypoint
            if (distance > 0) {
                // Normalize direction vector
                dx /= distance;
                dy /= distance;

                // Move toward waypoint with adjusted speed
                turnPosX += dx * adjustedSpeed;
                turnPosY += dy * adjustedSpeed;

                // Update animation position
                animPos = (currentDirection == Direction::UP || currentDirection == Direction::DOWN) ?
                         turnPosY : turnPosX;
            }

            // Update turn progress for visualization
            if (turning) {
                turnProgress = std::min(1.0f, turnProgress + 0.002f * delta);
            }
        }

        // Check if we've reached the last waypoint
        if (currentWaypoint == waypoints.size() - 1) {
            // Get screen dimensions
            const int windowWidth = 800;
            const int windowHeight = 800;

            // Check if off-screen
            if (turnPosX < -30.0f || turnPosX > windowWidth + 30.0f ||
                turnPosY < -30.0f || turnPosY > windowHeight + 30.0f) {
                // Flag for removal
                state = VehicleState::EXITED;
                DebugLogger::log("Vehicle " + id + " has left the screen", DebugLogger::LogLevel::DEBUG);
            }
        }
    }
    else {
        // Red light - handle queue positioning with deceleration
        if (currentWaypoint <= 1) {  // Vehicle is approaching or at the stop line
            // Get the stop line waypoint
            auto& stopLine = waypoints[1];

            // Calculate target position based on queue position with improved spacing
            float queueOffsetDistance = VEHICLE_SPACING * (queuePos + 0.2f); // Added small offset for better staggering
            float queueStopX = stopLine.x;
            float queueStopY = stopLine.y;

            // Adjust target position based on direction of travel
            switch (currentDirection) {
                case Direction::DOWN:  // From North (A)
                    queueStopY -= queueOffsetDistance;
                    break;
                case Direction::UP:    // From South (C)
                    queueStopY += queueOffsetDistance;
                    break;
                case Direction::LEFT:  // From East (B)
                    queueStopX += queueOffsetDistance;
                    break;
                case Direction::RIGHT: // From West (D)
                    queueStopX -= queueOffsetDistance;
                    break;
            }

            // Calculate direction and distance to queue position
            float dx = queueStopX - turnPosX;
            float dy = queueStopY - turnPosY;
            float distance = std::sqrt(dx*dx + dy*dy);

            // Adjust speed based on distance (decelerate as approaching)
            float adjustedSpeed = SPEED;
            if (distance < 50.0f) {
                // Slow down as approaching the stop position
                adjustedSpeed *= (distance / 50.0f) + 0.2f;
            }

            // Only move if far enough from target position (prevents jitter)
            if (distance > 2.0f) {
                // Normalize direction
                dx /= distance;
                dy /= distance;

                // Move toward queue position with adjusted speed
                turnPosX += dx * adjustedSpeed;
                turnPosY += dy * adjustedSpeed;

                // Update animation position
                animPos = (currentDirection == Direction::UP || currentDirection == Direction::DOWN) ?
                        turnPosY : turnPosX;
            }
        }
    }
}

void Vehicle::calculateTurnPath(float startX, float startY, float controlX, float controlY,
                              float endX, float endY, float progress) {
    // Quadratic bezier curve calculation for smooth turning
    float oneMinusT = 1.0f - progress;

    // Calculate position on the curve
    turnPosX = oneMinusT * oneMinusT * startX +
               2.0f * oneMinusT * progress * controlX +
               progress * progress * endX;

    turnPosY = oneMinusT * oneMinusT * startY +
               2.0f * oneMinusT * progress * controlY +
               progress * progress * endY;
}

void Vehicle::render(SDL_Renderer* renderer, SDL_Texture* vehicleTexture, int queuePos) {
    // Store queue position for use in update method
    this->queuePos = queuePos;

    // ENHANCED VEHICLE RENDERING FOR BETTER VISUALIZATION
    SDL_Color color;

    // STEP 1: Choose appropriate vehicle color based on lane and type
    if (isEmergency) {
        // Emergency vehicles are bright red with flashing effect
        uint32_t time = SDL_GetTicks();
        bool flash = (time / 250) % 2 == 0; // Flash every 250ms
        color = flash ? SDL_Color{255, 0, 0, 255} : SDL_Color{180, 0, 0, 255};
    }
    else {
        // Base color determined by lane and lane number
        switch (lane) {
            case 'A': // North road
                if (laneNumber == 1) {
                    color = {30, 144, 255, 255}; // Dodger Blue for A1
                } else if (laneNumber == 2) {
                    // AL2 is priority lane - orange with intensity based on count
                    // Brighter orange when more vehicles (simulating priority)
                    color = {255, 140, 0, 255}; // Orange for A2 (Priority)
                } else {
                    // Free lane - green
                    color = {50, 205, 50, 255}; // Lime Green for A3 (Free)
                }
                break;

            case 'B': // East road
                if (laneNumber == 1) {
                    color = {75, 0, 130, 255}; // Indigo for B1
                } else if (laneNumber == 2) {
                    color = {218, 165, 32, 255}; // Goldenrod for B2
                } else {
                    color = {34, 139, 34, 255}; // Forest Green for B3 (Free)
                }
                break;

            case 'C': // South road
                if (laneNumber == 1) {
                    color = {70, 130, 180, 255}; // Steel Blue for C1
                } else if (laneNumber == 2) {
                    color = {210, 105, 30, 255}; // Chocolate for C2
                } else {
                    color = {60, 179, 113, 255}; // Medium Sea Green for C3 (Free)
                }
                break;

            case 'D': // West road
                if (laneNumber == 1) {
                    color = {138, 43, 226, 255}; // Blue Violet for D1
                } else if (laneNumber == 2) {
                    color = {205, 133, 63, 255}; // Peru for D2
                } else {
                    color = {46, 139, 87, 255}; // Sea Green for D3 (Free)
                }
                break;

            default:
                color = {150, 150, 150, 255}; // Grey default
                break;
        }
    }

    // Make vehicles brighter when turning for better visibility
    if (turning) {
        color.r = std::min(255, color.r + 40);
        color.g = std::min(255, color.g + 40);
        color.b = std::min(255, color.b + 40);
    }

    // Set color for vehicle body
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // STEP 2: Determine vehicle dimensions - LARGER for better visibility
    float vehicleWidth = 14.0f;  // Wider than original
    float vehicleLength = 26.0f; // Longer than original

    // STEP 3: Create vehicle rectangle based on orientation
    SDL_FRect vehicleRect;

    if (turning) {
        // For turning vehicles, adjust dimensions gradually for smooth turns
        float progress = turnProgress;
        float width = vehicleWidth;
        float length = vehicleLength;

        // During turn, gradually change dimensions
        if (currentDirection == Direction::UP || currentDirection == Direction::DOWN) {
            // Transitioning from vertical to horizontal
            if (destination == Destination::LEFT || destination == Destination::RIGHT) {
                width = vehicleWidth * (1.0f - progress) + vehicleLength * progress;
                length = vehicleLength * (1.0f - progress) + vehicleWidth * progress;
            }
        } else {
            // Transitioning from horizontal to vertical
            if (destination == Destination::LEFT || destination == Destination::RIGHT) {
                width = vehicleLength * (1.0f - progress) + vehicleWidth * progress;
                length = vehicleWidth * (1.0f - progress) + vehicleLength * progress;
            }
        }

        vehicleRect = {turnPosX - width/2, turnPosY - length/2, width, length};
    } else {
        // Non-turning vehicles have fixed orientation based on direction
        switch (currentDirection) {
            case Direction::DOWN:
            case Direction::UP:
                // Vertical roads (taller than wide)
                vehicleRect = {turnPosX - vehicleWidth/2, turnPosY - vehicleLength/2, vehicleWidth, vehicleLength};
                break;
            case Direction::LEFT:
            case Direction::RIGHT:
                // Horizontal roads (wider than tall)
                vehicleRect = {turnPosX - vehicleLength/2, turnPosY - vehicleWidth/2, vehicleLength, vehicleWidth};
                break;
        }
    }

    // STEP 4: Draw the vehicle body with border
    SDL_RenderFillRect(renderer, &vehicleRect);

    // Add 3D effect with gradient
    SDL_Color shadowColor = {
        static_cast<Uint8>(color.r * 0.7f),
        static_cast<Uint8>(color.g * 0.7f),
        static_cast<Uint8>(color.b * 0.7f),
        color.a
    };

    SDL_Color highlightColor = {
        static_cast<Uint8>(std::min(255, color.r + 40)),
        static_cast<Uint8>(std::min(255, color.g + 40)),
        static_cast<Uint8>(std::min(255, color.b + 40)),
        color.a
    };

    // Add shadow edge
    SDL_SetRenderDrawColor(renderer, shadowColor.r, shadowColor.g, shadowColor.b, shadowColor.a);
    SDL_FRect shadowEdge;

    if (currentDirection == Direction::DOWN || currentDirection == Direction::UP) {
        shadowEdge = {vehicleRect.x + vehicleRect.w * 0.6f, vehicleRect.y, vehicleRect.w * 0.4f, vehicleRect.h};
    } else {
        shadowEdge = {vehicleRect.x, vehicleRect.y + vehicleRect.h * 0.6f, vehicleRect.w, vehicleRect.h * 0.4f};
    }
    SDL_RenderFillRect(renderer, &shadowEdge);

    // Add highlight edge
    SDL_SetRenderDrawColor(renderer, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
    SDL_FRect highlightEdge;

    if (currentDirection == Direction::DOWN || currentDirection == Direction::UP) {
        highlightEdge = {vehicleRect.x, vehicleRect.y, vehicleRect.w * 0.3f, vehicleRect.h};
    } else {
        highlightEdge = {vehicleRect.x, vehicleRect.y, vehicleRect.w, vehicleRect.h * 0.3f};
    }
    SDL_RenderFillRect(renderer, &highlightEdge);

    // Add border outline for better definition
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black border
    SDL_RenderRect(renderer, &vehicleRect);

    // STEP 5: Draw destination indicator - VERY CLEAR directional arrows
    // This shows exactly where each vehicle is going - LEFT or STRAIGHT

    if (destination == Destination::LEFT) {
        // LEFT TURN indicator - arrow pointing left relative to vehicle direction
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Bright yellow

        // Draw left arrow based on vehicle direction
        SDL_FPoint arrow[3];
        int arrowSize = 8; // Larger arrow for better visibility

        switch (currentDirection) {
            case Direction::DOWN: // From North (A)
                // Draw on left side of vehicle
                arrow[0] = {vehicleRect.x, vehicleRect.y + vehicleRect.h/3}; // Point
                arrow[1] = {vehicleRect.x + arrowSize, vehicleRect.y + vehicleRect.h/3 - arrowSize/2}; // Top
                arrow[2] = {vehicleRect.x + arrowSize, vehicleRect.y + vehicleRect.h/3 + arrowSize/2}; // Bottom
                break;

            case Direction::UP: // From South (C)
                // Draw on right side of vehicle
                arrow[0] = {vehicleRect.x + vehicleRect.w, vehicleRect.y + vehicleRect.h*2/3};
                arrow[1] = {vehicleRect.x + vehicleRect.w - arrowSize, vehicleRect.y + vehicleRect.h*2/3 - arrowSize/2};
                arrow[2] = {vehicleRect.x + vehicleRect.w - arrowSize, vehicleRect.y + vehicleRect.h*2/3 + arrowSize/2};
                break;

            case Direction::LEFT: // From East (B)
                // Draw on left side of vehicle
                arrow[0] = {vehicleRect.x + vehicleRect.w/3, vehicleRect.y};
                arrow[1] = {vehicleRect.x + vehicleRect.w/3 - arrowSize/2, vehicleRect.y + arrowSize};
                arrow[2] = {vehicleRect.x + vehicleRect.w/3 + arrowSize/2, vehicleRect.y + arrowSize};
                break;

            case Direction::RIGHT: // From West (D)
                // Draw on right side of vehicle
                arrow[0] = {vehicleRect.x + vehicleRect.w*2/3, vehicleRect.y + vehicleRect.h};
                arrow[1] = {vehicleRect.x + vehicleRect.w*2/3 - arrowSize/2, vehicleRect.y + vehicleRect.h - arrowSize};
                arrow[2] = {vehicleRect.x + vehicleRect.w*2/3 + arrowSize/2, vehicleRect.y + vehicleRect.h - arrowSize};
                break;
        }

        // Draw filled triangle
        SDL_RenderFillTriangleF(renderer, arrow[0].x, arrow[0].y, arrow[1].x, arrow[1].y, arrow[2].x, arrow[2].y);

        // Draw "L" symbol in bright yellow to indicate LEFT turn
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        float centerX = vehicleRect.x + vehicleRect.w/2;
        float centerY = vehicleRect.y + vehicleRect.h/2;
        float symbolSize = 6.0f;

        SDL_RenderLine(renderer, centerX - symbolSize/2, centerY - symbolSize/2,
                       centerX - symbolSize/2, centerY + symbolSize/2);
        SDL_RenderLine(renderer, centerX - symbolSize/2, centerY + symbolSize/2,
                       centerX + symbolSize/2, centerY + symbolSize/2);
    }
    else if (destination == Destination::STRAIGHT) {
        // STRAIGHT indicator - double parallel lines
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Bright yellow

        SDL_FRect line1, line2;
        float lineWidth = 2.5f;
        float lineLength = 8.0f;
        float lineGap = 4.0f;

        switch (currentDirection) {
            case Direction::DOWN:
                // Two vertical lines on top part of vehicle
                line1 = {vehicleRect.x + vehicleRect.w*0.33f, vehicleRect.y + 5.0f, lineWidth, lineLength};
                line2 = {vehicleRect.x + vehicleRect.w*0.67f, vehicleRect.y + 5.0f, lineWidth, lineLength};
                break;

            case Direction::UP:
                // Two vertical lines on bottom part of vehicle
                line1 = {vehicleRect.x + vehicleRect.w*0.33f, vehicleRect.y + vehicleRect.h - lineLength - 5.0f, lineWidth, lineLength};
                line2 = {vehicleRect.x + vehicleRect.w*0.67f, vehicleRect.y + vehicleRect.h - lineLength - 5.0f, lineWidth, lineLength};
                break;

            case Direction::LEFT:
                // Two horizontal lines on right part of vehicle
                line1 = {vehicleRect.x + vehicleRect.w - lineLength - 5.0f, vehicleRect.y + vehicleRect.h*0.33f, lineLength, lineWidth};
                line2 = {vehicleRect.x + vehicleRect.w - lineLength - 5.0f, vehicleRect.y + vehicleRect.h*0.67f, lineLength, lineWidth};
                break;

            case Direction::RIGHT:
                // Two horizontal lines on left part of vehicle
                line1 = {vehicleRect.x + 5.0f, vehicleRect.y + vehicleRect.h*0.33f, lineLength, lineWidth};
                line2 = {vehicleRect.x + 5.0f, vehicleRect.y + vehicleRect.h*0.67f, lineLength, lineWidth};
                break;
        }

        SDL_RenderFillRect(renderer, &line1);
        SDL_RenderFillRect(renderer, &line2);

        // Draw "S" symbol in bright yellow to indicate STRAIGHT
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        float centerX = vehicleRect.x + vehicleRect.w/2;
        float centerY = vehicleRect.y + vehicleRect.h/2;
        float symbolSize = 6.0f;

        // Draw S shape with 5 line segments
        SDL_RenderLine(renderer, centerX + symbolSize/2, centerY - symbolSize/2,
                      centerX - symbolSize/2, centerY - symbolSize/2);
        SDL_RenderLine(renderer, centerX - symbolSize/2, centerY - symbolSize/2,
                      centerX - symbolSize/2, centerY);
        SDL_RenderLine(renderer, centerX - symbolSize/2, centerY,
                      centerX + symbolSize/2, centerY);
        SDL_RenderLine(renderer, centerX + symbolSize/2, centerY,
                      centerX + symbolSize/2, centerY + symbolSize/2);
        SDL_RenderLine(renderer, centerX + symbolSize/2, centerY + symbolSize/2,
                      centerX - symbolSize/2, centerY + symbolSize/2);
    }

    // STEP 6: Add lane number indicators as distinctive marks
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White for indicators

    // Draw large lane number on vehicle
    float numX = vehicleRect.x + vehicleRect.w*0.5f;
    float numY = vehicleRect.y + vehicleRect.h*0.5f;
    float numSize = 8.0f;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for number

    switch (laneNumber) {
        case 1: // Draw "1"
            SDL_RenderLine(renderer, numX, numY - numSize/2, numX, numY + numSize/2);
            break;

        case 2: // Draw "2"
            SDL_RenderLine(renderer, numX - numSize/2, numY - numSize/2, numX + numSize/2, numY - numSize/2);
            SDL_RenderLine(renderer, numX + numSize/2, numY - numSize/2, numX + numSize/2, numY);
            SDL_RenderLine(renderer, numX + numSize/2, numY, numX - numSize/2, numY);
            SDL_RenderLine(renderer, numX - numSize/2, numY, numX - numSize/2, numY + numSize/2);
            SDL_RenderLine(renderer, numX - numSize/2, numY + numSize/2, numX + numSize/2, numY + numSize/2);
            break;

        case 3: // Draw "3"
            SDL_RenderLine(renderer, numX - numSize/2, numY - numSize/2, numX + numSize/2, numY - numSize/2);
            SDL_RenderLine(renderer, numX + numSize/2, numY - numSize/2, numX + numSize/2, numY);
            SDL_RenderLine(renderer, numX - numSize/2, numY, numX + numSize/2, numY);
            SDL_RenderLine(renderer, numX + numSize/2, numY, numX + numSize/2, numY + numSize/2);
            SDL_RenderLine(renderer, numX - numSize/2, numY + numSize/2, numX + numSize/2, numY + numSize/2);
            break;
    }

    // STEP 7: Emergency vehicle indicators (if applicable)
    if (isEmergency) {
        // Draw a flashing effect
        uint32_t time = SDL_GetTicks();
        bool flash = (time / 200) % 2 == 0; // Flash every 200ms

        if (flash) {
            // Draw a cross symbol for emergency vehicles when flashing
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White

            float crossSize = 10.0f;
            SDL_FRect crossV, crossH;

            crossH = {turnPosX - crossSize/2, turnPosY - 1.5f, crossSize, 3.0f};
            crossV = {turnPosX - 1.5f, turnPosY - crossSize/2, 3.0f, crossSize};

            SDL_RenderFillRect(renderer, &crossH);
            SDL_RenderFillRect(renderer, &crossV);

            // Draw "E" for Emergency
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            float eX = vehicleRect.x + vehicleRect.w*0.3f;
            float eY = vehicleRect.y + vehicleRect.h*0.3f;
            float eSize = 6.0f;

            SDL_RenderLine(renderer, eX, eY, eX, eY + eSize);
            SDL_RenderLine(renderer, eX, eY, eX + eSize/2, eY);
            SDL_RenderLine(renderer, eX, eY + eSize/2, eX + eSize/2, eY + eSize/2);
            SDL_RenderLine(renderer, eX, eY + eSize, eX + eSize/2, eY + eSize);
        }
    }

    // STEP 8: Add road indicator
    // Draw small road letter (A,B,C,D) on each vehicle for easy identification
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    float roadX = vehicleRect.x + vehicleRect.w*0.25f;
    float roadY = vehicleRect.y + vehicleRect.h*0.25f;
    float roadSize = 6.0f;

    switch (lane) {
        case 'A': // Draw "A"
            SDL_RenderLine(renderer, roadX - roadSize/2, roadY + roadSize/2, roadX, roadY - roadSize/2);
            SDL_RenderLine(renderer, roadX, roadY - roadSize/2, roadX + roadSize/2, roadY + roadSize/2);
            SDL_RenderLine(renderer, roadX - roadSize/4, roadY, roadX + roadSize/4, roadY);
            break;

        case 'B': // Draw "B"
            SDL_RenderLine(renderer, roadX - roadSize/2, roadY - roadSize/2, roadX - roadSize/2, roadY + roadSize/2);
            SDL_RenderLine(renderer, roadX - roadSize/2, roadY - roadSize/2, roadX + roadSize/2, roadY - roadSize/2);
            SDL_RenderLine(renderer, roadX + roadSize/2, roadY - roadSize/2, roadX + roadSize/2, roadY);
            SDL_RenderLine(renderer, roadX + roadSize/2, roadY, roadX - roadSize/2, roadY);
            SDL_RenderLine(renderer, roadX - roadSize/2, roadY, roadX - roadSize/2, roadY + roadSize/2);
            SDL_RenderLine(renderer, roadX - roadSize/2, roadY + roadSize/2, roadX + roadSize/2, roadY + roadSize/2);
            SDL_RenderLine(renderer, roadX + roadSize/2, roadY + roadSize/2, roadX + roadSize/2, roadY);
            break;

        case 'C': // Draw "C"
            SDL_RenderLine(renderer, roadX + roadSize/2, roadY - roadSize/2, roadX - roadSize/2, roadY - roadSize/2);
            SDL_RenderLine(renderer, roadX - roadSize/2, roadY - roadSize/2, roadX - roadSize/2, roadY + roadSize/2);
            SDL_RenderLine(renderer, roadX - roadSize/2, roadY + roadSize/2, roadX + roadSize/2, roadY + roadSize/2);
            break;

        case 'D': // Draw "D"
            SDL_RenderLine(renderer, roadX - roadSize/2, roadY - roadSize/2, roadX - roadSize/2, roadY + roadSize/2);
            SDL_RenderLine(renderer, roadX - roadSize/2, roadY - roadSize/2, roadX, roadY - roadSize/2);
            SDL_RenderLine(renderer, roadX, roadY - roadSize/2, roadX + roadSize/2, roadY);
            SDL_RenderLine(renderer, roadX + roadSize/2, roadY, roadX, roadY + roadSize/2);
            SDL_RenderLine(renderer, roadX, roadY + roadSize/2, roadX - roadSize/2, roadY + roadSize/2);
            break;
    }
}
// Helper for drawing triangles (SDL3 compatible)
void Vehicle::SDL_RenderFillTriangleF(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float x3, float y3) {
    // Create vertices for rendering with SDL_RenderGeometry
    SDL_Vertex vertices[3];

    // Create color with normalized values (0.0-1.0)
    SDL_FColor fcolor = {
        1.0f,  // r (normalized to 0.0-1.0)
        1.0f,  // g
        1.0f,  // b
        1.0f   // a
    };

    // Set vertices
    vertices[0].position.x = x1;
    vertices[0].position.y = y1;
    vertices[0].color = fcolor;

    vertices[1].position.x = x2;
    vertices[1].position.y = y2;
    vertices[1].color = fcolor;

    vertices[2].position.x = x3;
    vertices[2].position.y = y3;
    vertices[2].color = fcolor;

    // Draw the triangle
    SDL_RenderGeometry(renderer, NULL, vertices, 3, NULL, 0);
}
