
// src/managers/TrafficManager.cpp
#include "managers/TrafficManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>

TrafficManager::TrafficManager()
    : inPriorityMode(false), stateTimer(0.0f), lastUpdateTime(0.0f),
      vehicleProcessInterval(2.0f) {

  // Initialize lanes
  lanes.push_back(std::make_unique<Lane>(LaneId::AL1_INCOMING, false));
  lanes.push_back(std::make_unique<Lane>(LaneId::AL2_PRIORITY, true));
  lanes.push_back(std::make_unique<Lane>(LaneId::AL3_FREELANE, false));
  lanes.push_back(std::make_unique<Lane>(LaneId::BL1_INCOMING, false));
  lanes.push_back(std::make_unique<Lane>(LaneId::BL2_NORMAL, false));
  lanes.push_back(std::make_unique<Lane>(LaneId::BL3_FREELANE, false));
  lanes.push_back(std::make_unique<Lane>(LaneId::CL1_INCOMING, false));
  lanes.push_back(std::make_unique<Lane>(LaneId::CL2_NORMAL, false));
  lanes.push_back(std::make_unique<Lane>(LaneId::CL3_FREELANE, false));
  lanes.push_back(std::make_unique<Lane>(LaneId::DL1_INCOMING, false));
  lanes.push_back(std::make_unique<Lane>(LaneId::DL2_NORMAL, false));
  lanes.push_back(std::make_unique<Lane>(LaneId::DL3_FREELANE, false));

  // Initialize traffic lights
  trafficLights[LaneId::AL2_PRIORITY] = TrafficLight();
  trafficLights[LaneId::BL2_NORMAL] = TrafficLight();
  trafficLights[LaneId::CL2_NORMAL] = TrafficLight();
  trafficLights[LaneId::DL2_NORMAL] = TrafficLight();

  synchronizeTrafficLights();
}

void TrafficManager::update(float deltaTime) {
  stateTimer += deltaTime;
  lastUpdateTime += deltaTime;

  // Read new vehicles from files
  static float fileCheckTimer = 0.0f;
  fileCheckTimer += deltaTime;

  if (fileCheckTimer >= 0.1f) {
    FileHandler fileHandler;
    auto newVehicles = fileHandler.readNewVehicles();

    for (const auto &[laneId, vehicle] : newVehicles) {
      addVehicleToLane(laneId, vehicle);
      addNewVehicleToState(vehicle, laneId);
    }
    fileCheckTimer = 0.0f;
  }

  // Update traffic lights
  updateTrafficLights(deltaTime);

  // Process vehicles based on traffic rules
  if (checkPriorityConditions()) {
    if (!inPriorityMode) {
      synchronizeTrafficLights();
    }
    inPriorityMode = true;
    processPriorityLane();
  } else {
    if (inPriorityMode) {
      synchronizeTrafficLights();
    }
    inPriorityMode = false;
    if (lastUpdateTime >= vehicleProcessInterval) {
      size_t vehiclesToProcess = calculateVehiclesToProcess();
      processNormalLanes(vehiclesToProcess);
      lastUpdateTime = 0.0f;
    }
  }

  // Update vehicle positions
  updateVehiclePositions(deltaTime);

  // Process free lanes
  for (auto &lane : lanes) {
    if (lane->getId() == LaneId::AL3_FREELANE ||
        lane->getId() == LaneId::BL3_FREELANE ||
        lane->getId() == LaneId::CL3_FREELANE ||
        lane->getId() == LaneId::DL3_FREELANE) {

      if (lane->getQueueSize() > 0 && canProcessVehicle(lane->getId())) {
        auto vehicle = lane->removeVehicle();
        if (vehicle) {
          auto it = activeVehicles.find(vehicle->getId());
          if (it != activeVehicles.end()) {
            it->second.isMoving = true;
          }
        }
      }
    }
  }
}

void TrafficManager::updateTrafficLights(float deltaTime) {
  for (auto &[_, light] : trafficLights) {
    light.update(deltaTime);
  }
}

void TrafficManager::synchronizeTrafficLights() {
  if (inPriorityMode) {
    trafficLights[LaneId::AL2_PRIORITY].setState(LightState::GREEN);
    trafficLights[LaneId::BL2_NORMAL].setState(LightState::RED);
    trafficLights[LaneId::CL2_NORMAL].setState(LightState::RED);
    trafficLights[LaneId::DL2_NORMAL].setState(LightState::RED);
  } else {
    trafficLights[LaneId::AL2_PRIORITY].setState(LightState::RED);
    trafficLights[LaneId::BL2_NORMAL].setState(LightState::GREEN);
    trafficLights[LaneId::CL2_NORMAL].setState(LightState::RED);
    trafficLights[LaneId::DL2_NORMAL].setState(LightState::GREEN);
  }
}

void TrafficManager::addVehicleToLane(LaneId laneId,
                                      std::shared_ptr<Vehicle> vehicle) {
  auto it =
      std::find_if(lanes.begin(), lanes.end(), [laneId](const auto &lane) {
        return lane->getId() == laneId;
      });
  if (it != lanes.end()) {
    (*it)->addVehicle(vehicle);
  }
}

void TrafficManager::addNewVehicleToState(std::shared_ptr<Vehicle> vehicle,
                                          LaneId laneId) {
  VehicleState state;
  state.vehicle = vehicle;
  state.speed = 50.0f; // Reduced speed for better visualization
  state.isMoving = false;
  state.direction = vehicle->getDirection();
  state.hasStartedTurn = false;
  state.turnProgress = 0.0f;
  state.waitTime = 0.0f;

  // Calculate initial queue position
  const float QUEUE_SPACING = 40.0f;
  const float CENTER_X = 400.0f;
  const float CENTER_Y = 300.0f;
  const float LANE_WIDTH = 60.0f;
  const float QUEUE_START_OFFSET = 250.0f;

  size_t queuePosition = getLaneSize(laneId);

  // Set initial position based on lane
  switch (laneId) {
  case LaneId::AL1_INCOMING:
  case LaneId::AL2_PRIORITY:
  case LaneId::AL3_FREELANE: {
    float laneOffset =
        static_cast<float>((static_cast<int>(laneId) -
                            static_cast<int>(LaneId::AL1_INCOMING))) *
        LANE_WIDTH;
    state.x = CENTER_X - QUEUE_START_OFFSET - (queuePosition * QUEUE_SPACING);
    state.y = CENTER_Y - LANE_WIDTH + laneOffset;
    break;
  }
  case LaneId::BL1_INCOMING:
  case LaneId::BL2_NORMAL:
  case LaneId::BL3_FREELANE: {
    float laneOffset =
        static_cast<float>((static_cast<int>(laneId) -
                            static_cast<int>(LaneId::BL1_INCOMING))) *
        LANE_WIDTH;
    state.x = CENTER_X - LANE_WIDTH + laneOffset;
    state.y = CENTER_Y - QUEUE_START_OFFSET - (queuePosition * QUEUE_SPACING);
    break;
  }
  case LaneId::CL1_INCOMING:
  case LaneId::CL2_NORMAL:
  case LaneId::CL3_FREELANE: {
    float laneOffset =
        static_cast<float>((static_cast<int>(laneId) -
                            static_cast<int>(LaneId::CL1_INCOMING))) *
        LANE_WIDTH;
    state.x = CENTER_X + QUEUE_START_OFFSET + (queuePosition * QUEUE_SPACING);
    state.y = CENTER_Y - LANE_WIDTH + laneOffset;
    break;
  }
  case LaneId::DL1_INCOMING:
  case LaneId::DL2_NORMAL:
  case LaneId::DL3_FREELANE: {
    float laneOffset =
        static_cast<float>((static_cast<int>(laneId) -
                            static_cast<int>(LaneId::DL1_INCOMING))) *
        LANE_WIDTH;
    state.x = CENTER_X - LANE_WIDTH + laneOffset;
    state.y = CENTER_Y + QUEUE_START_OFFSET + (queuePosition * QUEUE_SPACING);
    break;
  }
  }

  // Set initial state to moving for free lanes
  if (laneId == LaneId::AL3_FREELANE || laneId == LaneId::BL3_FREELANE ||
      laneId == LaneId::CL3_FREELANE || laneId == LaneId::DL3_FREELANE) {
    state.isMoving = true;
  }

  // Calculate target position
  calculateTargetPosition(state, laneId);
  activeVehicles[vehicle->getId()] = state;
}

void TrafficManager::updateVehiclePositions(float deltaTime) {
  auto it = activeVehicles.begin();
  while (it != activeVehicles.end()) {
    auto &state = it->second;

    if (state.isMoving) {
      float dx = state.targetX - state.x;
      float dy = state.targetY - state.y;
      float distance = std::sqrt(dx * dx + dy * dy);

      if (distance < 1.0f) {
        it = activeVehicles.erase(it);
        continue;
      }

      float speed = state.speed * (1.0f - std::exp(-distance / 100.0f));
      float moveX = (dx / distance) * speed * deltaTime;
      float moveY = (dy / distance) * speed * deltaTime;

      if (!checkCollision(state, state.x + moveX, state.y + moveY)) {
        state.x += moveX;
        state.y += moveY;
      }
      ++it;
    } else {
      updateVehicleQueuePosition(state, state.vehicle->getCurrentLane(),
                                 getLaneSize(state.vehicle->getCurrentLane()));
      ++it;
    }
  }
}

// Add this function to your TrafficManager.cpp file
void TrafficManager::updateVehicleQueuePosition(VehicleState &state,
                                                LaneId laneId,
                                                size_t queuePosition) {
  const float QUEUE_SPACING = 40.0f;
  const float CENTER_X = 400.0f;
  const float CENTER_Y = 300.0f;
  const float LANE_WIDTH = 60.0f;
  const float QUEUE_START_OFFSET = 250.0f;

  // Calculate position based on lane
  switch (laneId) {
  // Left lanes (A)
  case LaneId::AL1_INCOMING:
  case LaneId::AL2_PRIORITY:
  case LaneId::AL3_FREELANE: {
    float laneOffset =
        static_cast<float>((static_cast<int>(laneId) -
                            static_cast<int>(LaneId::AL1_INCOMING))) *
        LANE_WIDTH;
    state.x = CENTER_X - QUEUE_START_OFFSET -
              (static_cast<float>(queuePosition) * QUEUE_SPACING);
    state.y = CENTER_Y - LANE_WIDTH + laneOffset;
    break;
  }

  // Top lanes (B)
  case LaneId::BL1_INCOMING:
  case LaneId::BL2_NORMAL:
  case LaneId::BL3_FREELANE: {
    float laneOffset =
        static_cast<float>((static_cast<int>(laneId) -
                            static_cast<int>(LaneId::BL1_INCOMING))) *
        LANE_WIDTH;
    state.x = CENTER_X - LANE_WIDTH + laneOffset;
    state.y = CENTER_Y - QUEUE_START_OFFSET -
              (static_cast<float>(queuePosition) * QUEUE_SPACING);
    break;
  }

  // Right lanes (C)
  case LaneId::CL1_INCOMING:
  case LaneId::CL2_NORMAL:
  case LaneId::CL3_FREELANE: {
    float laneOffset =
        static_cast<float>((static_cast<int>(laneId) -
                            static_cast<int>(LaneId::CL1_INCOMING))) *
        LANE_WIDTH;
    state.x = CENTER_X + QUEUE_START_OFFSET +
              (static_cast<float>(queuePosition) * QUEUE_SPACING);
    state.y = CENTER_Y - LANE_WIDTH + laneOffset;
    break;
  }

  // Bottom lanes (D)
  case LaneId::DL1_INCOMING:
  case LaneId::DL2_NORMAL:
  case LaneId::DL3_FREELANE: {
    float laneOffset =
        static_cast<float>((static_cast<int>(laneId) -
                            static_cast<int>(LaneId::DL1_INCOMING))) *
        LANE_WIDTH;
    state.x = CENTER_X - LANE_WIDTH + laneOffset;
    state.y = CENTER_Y + QUEUE_START_OFFSET +
              (static_cast<float>(queuePosition) * QUEUE_SPACING);
    break;
  }
  }

  // Calculate target position based on whether vehicle is in free lane
  if (laneId == LaneId::AL3_FREELANE || laneId == LaneId::BL3_FREELANE ||
      laneId == LaneId::CL3_FREELANE || laneId == LaneId::DL3_FREELANE) {
    calculateTargetPosition(state, laneId);
  }
}

bool TrafficManager::checkCollision(const VehicleState &state, float newX,
                                    float newY) const {
  const float MIN_DISTANCE = 40.0f;

  for (const auto &[otherId, otherState] : activeVehicles) {
    if (otherId != state.vehicle->getId()) {
      float dx = newX - otherState.x;
      float dy = newY - otherState.y;
      float dist2 = dx * dx + dy * dy;
      if (dist2 < MIN_DISTANCE * MIN_DISTANCE) {
        return true;
      }
    }
  }
  return false;
}

bool TrafficManager::canProcessVehicle(LaneId laneId) const {
  // Check if there's space in the target area
  const float PROCESS_RADIUS = 100.0f;
  float targetX = 400.0f, targetY = 300.0f;

  switch (laneId) {
  case LaneId::AL3_FREELANE:
    targetX += PROCESS_RADIUS;
    break;
  case LaneId::BL3_FREELANE:
    targetY += PROCESS_RADIUS;
    break;
  case LaneId::CL3_FREELANE:
    targetX -= PROCESS_RADIUS;
    break;
  case LaneId::DL3_FREELANE:
    targetY -= PROCESS_RADIUS;
    break;
  default:
    return false;
  }

  for (const auto &[_, state] : activeVehicles) {
    if (state.isMoving) {
      float dx = state.x - targetX;
      float dy = state.y - targetY;
      if (std::sqrt(dx * dx + dy * dy) < PROCESS_RADIUS) {
        return false;
      }
    }
  }
  return true;
}

float TrafficManager::calculateTurningRadius(Direction dir) const {
  return dir == Direction::LEFT ? 80.0f : 60.0f;
}

void TrafficManager::calculateTargetPosition(VehicleState &state,
                                             LaneId laneId) {
  const float EXIT_DISTANCE = 450.0f;
  const float CENTER_X = 400.0f;
  const float CENTER_Y = 300.0f;
  const float TURN_RADIUS = 100.0f;

  switch (state.direction) {
  case Direction::STRAIGHT: {
    switch (laneId) {
    case LaneId::AL1_INCOMING:
    case LaneId::AL2_PRIORITY:
    case LaneId::AL3_FREELANE:
      state.targetX = CENTER_X + EXIT_DISTANCE;
      state.targetY = state.y;
      break;
    case LaneId::BL1_INCOMING:
    case LaneId::BL2_NORMAL:
    case LaneId::BL3_FREELANE:
      state.targetX = state.x;
      state.targetY = CENTER_Y + EXIT_DISTANCE;
      break;
    case LaneId::CL1_INCOMING:
    case LaneId::CL2_NORMAL:
    case LaneId::CL3_FREELANE:
      state.targetX = CENTER_X - EXIT_DISTANCE;
      state.targetY = state.y;
      break;
    case LaneId::DL1_INCOMING:
    case LaneId::DL2_NORMAL:
    case LaneId::DL3_FREELANE:
      state.targetX = state.x;
      state.targetY = CENTER_Y - EXIT_DISTANCE;
      break;
    }
    break;
  }
  case Direction::LEFT: {
    switch (laneId) {
    case LaneId::AL1_INCOMING:
    case LaneId::AL2_PRIORITY:
    case LaneId::AL3_FREELANE:
      state.targetX = state.x + TURN_RADIUS;
      state.targetY = CENTER_Y - EXIT_DISTANCE;
      break;
    case LaneId::BL1_INCOMING:
    case LaneId::BL2_NORMAL:
    case LaneId::BL3_FREELANE:
      state.targetX = CENTER_X - EXIT_DISTANCE;
      state.targetY = state.y + TURN_RADIUS;
      break;
    case LaneId::CL1_INCOMING:
    case LaneId::CL2_NORMAL:
    case LaneId::CL3_FREELANE:
      state.targetX = state.x - TURN_RADIUS;
      state.targetY = CENTER_Y + EXIT_DISTANCE;
      break;
    case LaneId::DL1_INCOMING:
    case LaneId::DL2_NORMAL:
    case LaneId::DL3_FREELANE:
      state.targetX = CENTER_X + EXIT_DISTANCE;
      state.targetY = state.y - TURN_RADIUS;
      break;
    }
    break;
  }
  case Direction::RIGHT: {
    switch (laneId) {
    case LaneId::AL1_INCOMING:
    case LaneId::AL2_PRIORITY:
    case LaneId::AL3_FREELANE:
      state.targetX = state.x + TURN_RADIUS;
      state.targetY = CENTER_Y + EXIT_DISTANCE;
      break;
    case LaneId::BL1_INCOMING:
    case LaneId::BL2_NORMAL:
    case LaneId::BL3_FREELANE:
      state.targetX = CENTER_X + EXIT_DISTANCE;
      state.targetY = state.y + TURN_RADIUS;
      break;
    case LaneId::CL1_INCOMING:
    case LaneId::CL2_NORMAL:
    case LaneId::CL3_FREELANE:
      state.targetX = state.x - TURN_RADIUS;
      state.targetY = CENTER_Y - EXIT_DISTANCE;
      break;
    case LaneId::DL1_INCOMING:
    case LaneId::DL2_NORMAL:
    case LaneId::DL3_FREELANE:
      state.targetX = CENTER_X - EXIT_DISTANCE;
      state.targetY = state.y - TURN_RADIUS;
      break;
    }
    break;
  }
  }
}

bool TrafficManager::checkPriorityConditions() const {
  auto priorityLane =
      std::find_if(lanes.begin(), lanes.end(), [](const auto &lane) {
        return lane->isPriorityLane() && lane->getQueueSize() > 10;
      });
  return priorityLane != lanes.end();
}

void TrafficManager::processPriorityLane() {
  for (auto &lane : lanes) {
    if (lane->isPriorityLane() && lane->getQueueSize() > 5) {
      while (lane->getQueueSize() > 5) {
        auto vehicle = lane->removeVehicle();
        if (vehicle) {
          auto it = activeVehicles.find(vehicle->getId());
          if (it != activeVehicles.end()) {
            it->second.isMoving = true;
          }
        }
      }
      break;
    }
  }
}

void TrafficManager::processNormalLanes(size_t vehicleCount) {
  if (vehicleCount == 0)
    return;

  for (auto &lane : lanes) {
    if (!lane->isPriorityLane() && lane->getId() != LaneId::AL3_FREELANE &&
        lane->getId() != LaneId::BL3_FREELANE &&
        lane->getId() != LaneId::CL3_FREELANE &&
        lane->getId() != LaneId::DL3_FREELANE) {

      for (size_t i = 0; i < vehicleCount && lane->getQueueSize() > 0; ++i) {
        auto vehicle = lane->removeVehicle();
        if (vehicle) {
          auto it = activeVehicles.find(vehicle->getId());
          if (it != activeVehicles.end()) {
            it->second.isMoving = true;
          }
        }
      }
    }
  }
}

size_t TrafficManager::calculateVehiclesToProcess() const {
  size_t totalVehicles = 0;
  size_t normalLaneCount = 0;

  for (const auto &lane : lanes) {
    if (!lane->isPriorityLane() && lane->getId() != LaneId::AL3_FREELANE &&
        lane->getId() != LaneId::BL3_FREELANE &&
        lane->getId() != LaneId::CL3_FREELANE &&
        lane->getId() != LaneId::DL3_FREELANE) {
      totalVehicles += lane->getQueueSize();
      normalLaneCount++;
    }
  }

  if (normalLaneCount == 0)
    return 0;

  float avgVehicles =
      static_cast<float>(totalVehicles) / static_cast<float>(normalLaneCount);
  return static_cast<size_t>(
      std::ceil(avgVehicles * 0.3f)); // Process 30% of average
}

size_t TrafficManager::getLaneSize(LaneId laneId) const {
  auto it =
      std::find_if(lanes.begin(), lanes.end(), [laneId](const auto &lane) {
        return lane->getId() == laneId;
      });
  if (it != lanes.end()) {
    return (*it)->getQueueSize();
  }
  return 0;
}
