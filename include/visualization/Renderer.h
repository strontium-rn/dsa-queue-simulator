// include/visualization/Renderer.h
#pragma once
#include <SDL3/SDL.h>
#include "managers/TrafficManager.h"
#include "visualization/DebugOverlay.h"
#include <memory>
#include <map>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Renderer {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    DebugOverlay debugOverlay;

    // Constants for visualization
    static constexpr int WINDOW_WIDTH = 1024;
    static constexpr int WINDOW_HEIGHT = 768;
    static constexpr int ROAD_WIDTH = 180;
    static constexpr int LANE_WIDTH = 60;
    static constexpr int CENTER_X = WINDOW_WIDTH / 2;
    static constexpr int CENTER_Y = WINDOW_HEIGHT / 2;

    static constexpr float VEHICLE_WIDTH = 30.0f;
    static constexpr float VEHICLE_HEIGHT = 20.0f;
    static constexpr float VEHICLE_SPACING = 45.0f;

    // Helper functions
    void renderBackground();
    void renderRoads();
    void renderLanes();
    void renderIntersection();
    void renderCrosswalks();
    void renderTrafficLights(const std::map<LaneId, TrafficLight>& lights);
    void renderVehicles(const std::map<uint32_t, VehicleState>& vehicles);
    void renderVehicle(float x, float y, Direction dir, bool isPriority, float angle, bool isMoving);
    void renderLaneIdentifiers();
    void renderVehicleCount(const TrafficManager& trafficManager);
    void renderPriorityIndicator(bool isInPriorityMode);
    void renderStopLines();
    void renderArrows();

    // Utility functions
    void drawDashedLine(float x1, float y1, float x2, float y2, float dashLength, float gapLength);
    void drawArrow(float x, float y, float angle, Direction dir);
    SDL_FPoint rotatePoint(float x, float y, float cx, float cy, float angle);
    void renderRoundedRect(float x, float y, float w, float h, float radius);

public:
    Renderer();
    ~Renderer();

    bool initialize();
    void render(const TrafficManager& trafficManager);
    void cleanup();
    bool isInitialized() const { return window != nullptr && renderer != nullptr; }
};
