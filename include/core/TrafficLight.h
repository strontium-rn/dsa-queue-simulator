// FILE: include/core/TrafficLight.h
#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <cstdint>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include "core/Lane.h"

class TrafficLight {
public:
    enum class State {
        ALL_RED = 0,
        A_GREEN = 1,
        B_GREEN = 2,
        C_GREEN = 3,
        D_GREEN = 4
    };

    TrafficLight();
    ~TrafficLight();

    // Updates the traffic light state based on lane priorities
    void update(const std::vector<Lane*>& lanes);

    // Renders the traffic lights
    void render(SDL_Renderer* renderer);

    // Returns the current traffic light state
    State getCurrentState() const { return currentState; }

    // Returns the next traffic light state
    State getNextState() const { return nextState; }

    // Sets the next traffic light state
    void setNextState(State state);

    // Checks if the specific lane gets green light
    bool isGreen(char lane) const;

private:
    State currentState;
    State nextState;

    // Timing for the green and red states
    const int allRedDuration = 2000; // 2 seconds for all red

    // Last state change time in milliseconds
    uint32_t lastStateChangeTime;

    // Priority mode flags
    bool isPriorityMode;
    bool shouldResumeNormalMode;
    bool forceAGreen;
    uint32_t priorityModeStartTime;

    // Helper function to calculate average vehicle count
    float calculateAverageVehicleCount(const std::vector<Lane*>& lanes);

    // Modern UI drawing functions
    void drawTrafficControlCenter(SDL_Renderer* renderer);
    void drawJunctionLight(SDL_Renderer* renderer, int x, int y, char roadId, bool isGreen);
    void drawStateTimer(SDL_Renderer* renderer);
    void drawHolographicLight(SDL_Renderer* renderer, int x, int y, int size, bool isActive);
    void drawPanelText(SDL_Renderer* renderer, const char* text, int x, int y);
    void drawPanelChar(SDL_Renderer* renderer, char c, int x, int y);
};

#endif // TRAFFIC_LIGHT_H
