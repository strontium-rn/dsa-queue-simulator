// src/visualization/DebugOverlay.cpp
#define _USE_MATH_DEFINES
#include <cmath>
#include "visualization/DebugOverlay.h"

void DebugOverlay::render(SDL_Renderer* renderer, const TrafficManager& trafficManager) {
    // Draw background for debug panel
    SDL_FRect debugPanel = {10.0f, 10.0f, 200.0f, 300.0f};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, &debugPanel);

    // Priority mode indicator
    if (trafficManager.isInPriorityMode()) {
        SDL_FRect priorityIndicator = {20.0f, 20.0f, 20.0f, 20.0f};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &priorityIndicator);
    }

    // Queue lengths visualization
    int yOffset = 50;
    for (const auto& lane : trafficManager.getLanes()) {
        SDL_FRect queueBar = {
            20.0f,
            static_cast<float>(yOffset),
            static_cast<float>(lane->getQueueSize() * 5),
            15.0f
        };

        // Color based on lane type
        if (lane->isPriorityLane()) {
            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
        } else if (lane->getId() == LaneId::AL3_FREELANE ||
                   lane->getId() == LaneId::BL3_FREELANE ||
                   lane->getId() == LaneId::CL3_FREELANE ||
                   lane->getId() == LaneId::DL3_FREELANE) {
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
        }

        SDL_RenderFillRect(renderer, &queueBar);
        yOffset += 20;
    }
}

void DebugOverlay::updateStatistics(const TrafficManager& trafficManager) {
    for (const auto& lane : trafficManager.getLanes()) {
        LaneStatistics& laneStat = stats[lane->getId()];
        laneStat.vehicleCount = static_cast<int>(lane->getQueueSize());
    }
}

void DebugOverlay::renderQueueStats(SDL_Renderer* renderer, int x, int y) {
    int yOffset = y;
    for (const auto& [laneId, stat] : stats) {
        SDL_FRect bar = {
            static_cast<float>(x + 10),
            static_cast<float>(yOffset),
            static_cast<float>(stat.vehicleCount * 5),
            15.0f
        };

        if (stat.vehicleCount > 10) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        } else if (stat.vehicleCount > 5) {
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        }

        SDL_RenderFillRect(renderer, &bar);
        yOffset += 20;
    }
}

void DebugOverlay::renderLaneLoadIndicator(SDL_Renderer* renderer, int x, int y) {
    const float RADIUS = 50.0f;
    const int SEGMENTS = 12;
    const float TWO_PI = static_cast<float>(2.0 * M_PI);

    for (int i = 0; i < SEGMENTS; i++) {
        float startAngle = (TWO_PI * i) / SEGMENTS;
        float endAngle = (TWO_PI * (i + 1)) / SEGMENTS;

        float startX = x + RADIUS * cosf(startAngle);
        float startY = y + RADIUS * sinf(startAngle);
        float endX = x + RADIUS * cosf(endAngle);
        float endY = y + RADIUS * sinf(endAngle);

        // Color based on load
        int laneIndex = i % 4;
        LaneId laneId = static_cast<LaneId>(laneIndex);
        int load = stats[laneId].vehicleCount;

        if (load > 10) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        } else if (load > 5) {
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        }

        SDL_RenderLine(renderer, startX, startY, endX, endY);
    }
}

void DebugOverlay::renderSystemStatus(SDL_Renderer* renderer, int x, int y) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    // TODO: Add text rendering for system status when needed
}
