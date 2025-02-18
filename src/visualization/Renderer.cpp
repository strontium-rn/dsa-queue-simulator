// src/visualization/Renderer.cpp
#include "visualization/Renderer.h"
#include <cmath>
#include <iostream>

Renderer::Renderer() : window(nullptr), renderer(nullptr) {}

Renderer::~Renderer() { cleanup(); }

// Update Renderer.cpp to fix SDL flags:
bool Renderer::initialize() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
    return false;
  }

  window = SDL_CreateWindow("Traffic Simulator", WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_RESIZABLE);

  if (!window) {
    std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
    return false;
  }

  renderer = SDL_CreateRenderer(window, nullptr);
  if (!renderer) {
    std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
    return false;
  }

  return true;
}

void Renderer::renderBackground() {
  // Draw sky gradient
  for (int y = 0; y < WINDOW_HEIGHT; ++y) {
    float t = static_cast<float>(y) / WINDOW_HEIGHT;
    uint8_t skyR = static_cast<uint8_t>(135 * (1 - t) + 30 * t);
    uint8_t skyG = static_cast<uint8_t>(206 * (1 - t) + 30 * t);
    uint8_t skyB = static_cast<uint8_t>(235 * (1 - t) + 30 * t);
    SDL_SetRenderDrawColor(renderer, skyR, skyG, skyB, 255);
    SDL_RenderLine(renderer, 0, y, WINDOW_WIDTH, y);
  }

  // Draw grass areas with texture
  SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
  for (int y = 0; y < WINDOW_HEIGHT; y += 20) {
    for (int x = 0; x < WINDOW_WIDTH; x += 20) {
      SDL_SetRenderDrawColor(renderer, 34 + (rand() % 20), 139 + (rand() % 20),
                             34 + (rand() % 20), 255);
      SDL_FRect grassPatch = {static_cast<float>(x), static_cast<float>(y),
                              20.0f, 20.0f};
      SDL_RenderFillRect(renderer, &grassPatch);
    }
  }
}

void Renderer::renderRoads() {
  // Main road background
  SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);

  // Horizontal road
  SDL_FRect horizontalRoad = {static_cast<float>(CENTER_X - 450),
                              static_cast<float>(CENTER_Y - ROAD_WIDTH / 2),
                              900.0f, static_cast<float>(ROAD_WIDTH)};
  SDL_RenderFillRect(renderer, &horizontalRoad);

  // Vertical road
  SDL_FRect verticalRoad = {static_cast<float>(CENTER_X - ROAD_WIDTH / 2),
                            static_cast<float>(CENTER_Y - 450),
                            static_cast<float>(ROAD_WIDTH), 900.0f};
  SDL_RenderFillRect(renderer, &verticalRoad);

  // Draw curbs
  SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
  const float CURB_WIDTH = 3.0f;

  // Horizontal curbs
  SDL_FRect hCurbTop = {horizontalRoad.x, horizontalRoad.y, horizontalRoad.w,
                        CURB_WIDTH};
  SDL_FRect hCurbBottom = {horizontalRoad.x,
                           horizontalRoad.y + horizontalRoad.h - CURB_WIDTH,
                           horizontalRoad.w, CURB_WIDTH};
  SDL_RenderFillRect(renderer, &hCurbTop);
  SDL_RenderFillRect(renderer, &hCurbBottom);

  // Vertical curbs
  SDL_FRect vCurbLeft = {verticalRoad.x, verticalRoad.y, CURB_WIDTH,
                         verticalRoad.h};
  SDL_FRect vCurbRight = {verticalRoad.x + verticalRoad.w - CURB_WIDTH,
                          verticalRoad.y, CURB_WIDTH, verticalRoad.h};
  SDL_RenderFillRect(renderer, &vCurbLeft);
  SDL_RenderFillRect(renderer, &vCurbRight);
}

void Renderer::renderLanes() {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  // Draw lane dividers
  const float DASH_LENGTH = 30.0f;
  const float GAP_LENGTH = 20.0f;

  // Horizontal lanes
  for (int i = -1; i <= 1; i++) {
    float y = static_cast<float>(CENTER_Y + i * LANE_WIDTH);
    drawDashedLine(CENTER_X - 450.0f, y, CENTER_X + 450.0f, y, DASH_LENGTH,
                   GAP_LENGTH);
  }

  // Vertical lanes
  for (int i = -1; i <= 1; i++) {
    float x = static_cast<float>(CENTER_X + i * LANE_WIDTH);
    drawDashedLine(x, CENTER_Y - 450.0f, x, CENTER_Y + 450.0f, DASH_LENGTH,
                   GAP_LENGTH);
  }
}

void Renderer::renderIntersection() {
  // Draw intersection box
  SDL_SetRenderDrawColor(renderer, 45, 45, 45, 255);
  SDL_FRect intersection = {static_cast<float>(CENTER_X - ROAD_WIDTH / 2),
                            static_cast<float>(CENTER_Y - ROAD_WIDTH / 2),
                            static_cast<float>(ROAD_WIDTH),
                            static_cast<float>(ROAD_WIDTH)};
  SDL_RenderFillRect(renderer, &intersection);

  // Draw intersection guide lines
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);

  // Dotted lines through intersection
  drawDashedLine(CENTER_X - ROAD_WIDTH / 2, CENTER_Y, CENTER_X + ROAD_WIDTH / 2,
                 CENTER_Y, 10.0f, 10.0f);

  drawDashedLine(CENTER_X, CENTER_Y - ROAD_WIDTH / 2, CENTER_X,
                 CENTER_Y + ROAD_WIDTH / 2, 10.0f, 10.0f);
}

void Renderer::renderCrosswalks() {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  const float STRIPE_WIDTH = 5.0f;
  const float STRIPE_LENGTH = 30.0f;
  const float STRIPE_GAP = 5.0f;
  const float CROSSWALK_WIDTH = 20.0f;

  // Render crosswalks on all four sides of the intersection
  for (int i = 0; i < 4; ++i) {
    float angle = i * 90.0f * M_PI / 180.0f;
    float baseX = CENTER_X + cos(angle) * (ROAD_WIDTH / 2 - CROSSWALK_WIDTH);
    float baseY = CENTER_Y + sin(angle) * (ROAD_WIDTH / 2 - CROSSWALK_WIDTH);

    for (float offset = 0; offset < ROAD_WIDTH;
         offset += STRIPE_WIDTH + STRIPE_GAP) {
      SDL_FPoint p1 =
          rotatePoint(baseX, baseY + offset, CENTER_X, CENTER_Y, angle);
      SDL_FPoint p2 = rotatePoint(baseX + STRIPE_LENGTH, baseY + offset,
                                  CENTER_X, CENTER_Y, angle);

      SDL_FRect stripe = {p1.x, p1.y, STRIPE_WIDTH, p2.y - p1.y};
      SDL_RenderFillRect(renderer, &stripe);
    }
  }
}

SDL_FPoint Renderer::rotatePoint(float x, float y, float cx, float cy,
                                 float angle) {
  float s = sin(angle);
  float c = cos(angle);

  // Translate point back to origin
  x -= cx;
  y -= cy;

  // Rotate point
  float xnew = x * c - y * s;
  float ynew = x * s + y * c;

  // Translate point back
  x = xnew + cx;
  y = ynew + cy;

  return {x, y};
}

void Renderer::drawDashedLine(float x1, float y1, float x2, float y2,
                              float dashLength, float gapLength) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  float distance = sqrt(dx * dx + dy * dy);
  float dashX = dx * dashLength / distance;
  float dashY = dy * dashLength / distance;
  float gapX = dx * gapLength / distance;
  float gapY = dy * gapLength / distance;

  float currentX = x1;
  float currentY = y1;

  while (distance > 0) {
    float nextX = currentX + dashX;
    float nextY = currentY + dashY;
    SDL_RenderLine(renderer, currentX, currentY, nextX, nextY);

    currentX = nextX + gapX;
    currentY = nextY + gapY;
    distance -= (dashLength + gapLength);
  }
}

void Renderer::renderVehicle(float x, float y, Direction dir, bool isPriority,
                             float angle, bool isMoving) {
  const float halfWidth = VEHICLE_WIDTH / 2.0f;
  const float halfHeight = VEHICLE_HEIGHT / 2.0f;

  float cosAngle = cosf(angle);
  float sinAngle = sinf(angle);

  // Vehicle corners
  SDL_FPoint vertices[4] = {
      {x + (-halfWidth * cosAngle - halfHeight * sinAngle),
       y + (-halfWidth * sinAngle + halfHeight * cosAngle)},
      {x + (halfWidth * cosAngle - halfHeight * sinAngle),
       y + (halfWidth * sinAngle + halfHeight * cosAngle)},
      {x + (halfWidth * cosAngle + halfHeight * sinAngle),
       y + (halfWidth * sinAngle - halfHeight * cosAngle)},
      {x + (-halfWidth * cosAngle + halfHeight * sinAngle),
       y + (-halfWidth * sinAngle - halfHeight * cosAngle)}};

  // Draw shadow
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
  for (int i = 0; i < 4; i++) {
    SDL_RenderLine(renderer, vertices[i].x + 2, vertices[i].y + 2,
                   vertices[(i + 1) % 4].x + 2, vertices[(i + 1) % 4].y + 2);
  }

  // Draw vehicle body
  if (isPriority) {
    SDL_SetRenderDrawColor(renderer, 255, 69, 0, 255); // Priority vehicles
  } else {
    SDL_SetRenderDrawColor(renderer, 65, 105, 225, 255); // Normal vehicles
  }

  for (int i = 0; i < 4; i++) {
    SDL_RenderLine(renderer, vertices[i].x, vertices[i].y,
                   vertices[(i + 1) % 4].x, vertices[(i + 1) % 4].y);
  }

  // Draw direction indicator
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  float arrowAngle = angle;

  switch (dir) {
  case Direction::LEFT:
    arrowAngle -= 0.785f;
    break;
  case Direction::RIGHT:
    arrowAngle += 0.785f;
    break;
  default:
    break;
  }

  float arrowLength = VEHICLE_WIDTH * 0.4f;
  float endX = x + arrowLength * cosf(arrowAngle);
  float endY = y + arrowLength * sinf(arrowAngle);

  SDL_RenderLine(renderer, x, y, endX, endY);

  // Add movement indicator if vehicle is moving
  if (isMoving) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    float speed = 2.0f;
    float t = static_cast<float>(SDL_GetTicks()) / 1000.0f;
    float offset = (sinf(t * speed) + 1.0f) * 5.0f;

    for (int i = 0; i < 3; i++) {
      float tailX = x - (i + 1) * offset * cosf(angle);
      float tailY = y - (i + 1) * offset * sinf(angle);
      SDL_RenderPoint(renderer, tailX, tailY);
    }
  }
}

void Renderer::renderTrafficLights(
    const std::map<LaneId, TrafficLight> &lights) {
  const float LIGHT_SIZE = 20.0f;
  const float HOUSING_PADDING = 5.0f;
  const float HOUSING_HEIGHT = 3 * LIGHT_SIZE + 4 * HOUSING_PADDING;
  const float HOUSING_WIDTH = LIGHT_SIZE + 2 * HOUSING_PADDING;

  for (const auto &[laneId, light] : lights) {
    float x = 0.0f, y = 0.0f;
    float rotation = 0.0f;

    // Position lights based on lane
    switch (laneId) {
    case LaneId::AL2_PRIORITY:
      x = static_cast<float>(CENTER_X - ROAD_WIDTH / 2 - HOUSING_WIDTH - 10);
      y = static_cast<float>(CENTER_Y - LANE_WIDTH);
      break;
    case LaneId::BL2_NORMAL:
      x = static_cast<float>(CENTER_X - LANE_WIDTH);
      y = static_cast<float>(CENTER_Y - ROAD_WIDTH / 2 - HOUSING_HEIGHT - 10);
      rotation = 90.0f;
      break;
    case LaneId::CL2_NORMAL:
      x = static_cast<float>(CENTER_X + ROAD_WIDTH / 2 + 10);
      y = static_cast<float>(CENTER_Y - LANE_WIDTH);
      break;
    case LaneId::DL2_NORMAL:
      x = static_cast<float>(CENTER_X - LANE_WIDTH);
      y = static_cast<float>(CENTER_Y + ROAD_WIDTH / 2 + 10);
      rotation = 90.0f;
      break;
    default:
      continue;
    }

    // Draw light housing (with rounded corners)
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    renderRoundedRect(x, y, HOUSING_WIDTH, HOUSING_HEIGHT, 5.0f);

    // Draw shadow
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
    renderRoundedRect(x + 2, y + 2, HOUSING_WIDTH, HOUSING_HEIGHT, 5.0f);

    // Calculate light positions
    float lightX = x + HOUSING_PADDING;
    float lightY = y + HOUSING_PADDING;

    // Draw red light
    if (light.getState() == LightState::RED) {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    } else {
      SDL_SetRenderDrawColor(renderer, 64, 0, 0, 255);
    }
    SDL_FRect redLight = {lightX, lightY, LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &redLight);

    // Draw yellow light
    SDL_SetRenderDrawColor(renderer, 64, 64, 0, 255);
    SDL_FRect yellowLight = {lightX, lightY + LIGHT_SIZE + HOUSING_PADDING,
                             LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &yellowLight);

    // Draw green light
    if (light.getState() == LightState::GREEN) {
      SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    } else {
      SDL_SetRenderDrawColor(renderer, 0, 64, 0, 255);
    }
    SDL_FRect greenLight = {lightX, lightY + 2 * (LIGHT_SIZE + HOUSING_PADDING),
                            LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &greenLight);
  }
}

void Renderer::renderRoundedRect(float x, float y, float w, float h,
                                 float radius) {
  const int segments = 8;

  // Draw the corners
  for (int corner = 0; corner < 4; corner++) {
    float centerX = (corner & 1) ? x + w - radius : x + radius;
    float centerY = (corner & 2) ? y + h - radius : y + radius;

    for (int i = 0; i <= segments; i++) {
      float angle1 = (corner * 90 + i * 90.0f / segments) * M_PI / 180.0f;
      float angle2 = (corner * 90 + (i + 1) * 90.0f / segments) * M_PI / 180.0f;

      SDL_RenderLine(renderer, centerX + radius * cosf(angle1),
                     centerY + radius * sinf(angle1),
                     centerX + radius * cosf(angle2),
                     centerY + radius * sinf(angle2));
    }
  }

  // Draw the straight edges
  SDL_RenderLine(renderer, x + radius, y, x + w - radius, y);
  SDL_RenderLine(renderer, x + radius, y + h, x + w - radius, y + h);
  SDL_RenderLine(renderer, x, y + radius, x, y + h - radius);
  SDL_RenderLine(renderer, x + w, y + radius, x + w, y + h - radius);
}

void Renderer::renderStopLines() {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  const float STOP_LINE_WIDTH = 8.0f;
  const float OFFSET = ROAD_WIDTH / 2 - 20.0f;

  // Draw stop lines at each intersection entry
  SDL_FRect stopLines[] = {
      // Left approach
      {static_cast<float>(CENTER_X - OFFSET - STOP_LINE_WIDTH),
       static_cast<float>(CENTER_Y - LANE_WIDTH), STOP_LINE_WIDTH,
       LANE_WIDTH * 2},

      // Top approach
      {static_cast<float>(CENTER_X - LANE_WIDTH),
       static_cast<float>(CENTER_Y - OFFSET - STOP_LINE_WIDTH), LANE_WIDTH * 2,
       STOP_LINE_WIDTH},

      // Right approach
      {static_cast<float>(CENTER_X + OFFSET),
       static_cast<float>(CENTER_Y - LANE_WIDTH), STOP_LINE_WIDTH,
       LANE_WIDTH * 2},

      // Bottom approach
      {static_cast<float>(CENTER_X - LANE_WIDTH),
       static_cast<float>(CENTER_Y + OFFSET), LANE_WIDTH * 2, STOP_LINE_WIDTH}};

  for (const auto &line : stopLines) {
    SDL_RenderFillRect(renderer, &line);
  }
}

// Update the renderArrows function to use correct angle calculations
void Renderer::renderArrows() {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
  const float ARROW_DISTANCE = 150.0f;

  // Draw direction arrows in each lane
  for (int lane = -1; lane <= 1; lane++) {
    float laneOffset = static_cast<float>(lane * LANE_WIDTH);

    // Left approach
    drawArrow(CENTER_X - ARROW_DISTANCE, CENTER_Y + laneOffset, 0.0f,
              Direction::STRAIGHT);

    // Top approach
    drawArrow(CENTER_X + laneOffset, CENTER_Y - ARROW_DISTANCE,
              static_cast<float>(M_PI / 2.0), Direction::STRAIGHT);

    // Right approach
    drawArrow(CENTER_X + ARROW_DISTANCE, CENTER_Y + laneOffset,
              static_cast<float>(M_PI), Direction::STRAIGHT);

    // Bottom approach
    drawArrow(CENTER_X + laneOffset, CENTER_Y + ARROW_DISTANCE,
              static_cast<float>(-M_PI / 2.0), Direction::STRAIGHT);
  }
}

// Update the drawArrow function definition to match the declaration
void Renderer::drawArrow(float x, float y, float angle, Direction dir) {
  const float ARROW_LENGTH = 30.0f;
  const float HEAD_SIZE = 10.0f;
  const float HEAD_ANGLE = static_cast<float>(M_PI / 6.0);

  float cosA = cosf(angle);
  float sinA = sinf(angle);

  // Arrow shaft
  float endX = x + ARROW_LENGTH * cosA;
  float endY = y + ARROW_LENGTH * sinA;
  SDL_RenderLine(renderer, x, y, endX, endY);

  // Arrow head
  float leftX = endX - HEAD_SIZE * cosf(angle + HEAD_ANGLE);
  float leftY = endY - HEAD_SIZE * sinf(angle + HEAD_ANGLE);
  float rightX = endX - HEAD_SIZE * cosf(angle - HEAD_ANGLE);
  float rightY = endY - HEAD_SIZE * sinf(angle - HEAD_ANGLE);

  SDL_RenderLine(renderer, endX, endY, leftX, leftY);
  SDL_RenderLine(renderer, endX, endY, rightX, rightY);
}

void Renderer::renderVehicleCount(const TrafficManager &trafficManager) {
  // Render vehicle count display in top-left corner
  const float PADDING = 10.0f;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
  SDL_FRect countBox = {PADDING, PADDING, 150.0f, 80.0f};
  SDL_RenderFillRect(renderer, &countBox);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  // Count display would go here - requires text rendering which SDL doesn't
  // provide directly You might want to add a text rendering library like
  // SDL_ttf for this
}

void Renderer::renderPriorityIndicator(bool isInPriorityMode) {
  if (isInPriorityMode) {
    const float INDICATOR_SIZE = 20.0f;
    SDL_SetRenderDrawColor(renderer, 255, 69, 0, 255);
    SDL_FRect indicator = {10.0f, 10.0f, INDICATOR_SIZE, INDICATOR_SIZE};
    SDL_RenderFillRect(renderer, &indicator);
  }
}

// Update Renderer's render function:
void Renderer::render(const TrafficManager& trafficManager) {
    renderBackground();
    renderRoads();
    renderLanes();
    renderIntersection();
    renderCrosswalks();
    renderStopLines();
    renderArrows();
    renderTrafficLights(trafficManager.getTrafficLights());

    // Render active vehicles
    const auto& vehicles = trafficManager.getActiveVehicles();
    for (const auto& [vehicleId, state] : vehicles) {
        float angle = atan2f(state.targetY - state.y, state.targetX - state.x);
        renderVehicle(
            state.x,
            state.y,
            state.direction,
            state.vehicle->getCurrentLane() == LaneId::AL2_PRIORITY,
            angle,
            state.isMoving
        );
    }

    renderPriorityIndicator(trafficManager.isInPriorityMode());
    debugOverlay.render(renderer, trafficManager);

    SDL_RenderPresent(renderer);
}

void Renderer::cleanup() {
  if (renderer) {
    SDL_DestroyRenderer(renderer);
    renderer = nullptr;
  }
  if (window) {
    SDL_DestroyWindow(window);
    window = nullptr;
  }
  SDL_Quit();
}
