// TrafficLight.h
#pragma once
#include <SDL3/SDL.h>
#include "Constants.h"
#include <cstdint>

class TrafficLight {
private:
    LightState state;
    LightState nextState;
    float transitionProgress;
    float transitionDuration;
    float stateTimer;
    bool isTransitioning;

public:
    TrafficLight();  // Just declare the constructor here
    void update(float deltaTime);
    void setState(LightState newState);
    LightState getState() const;
    void render(SDL_Renderer* renderer, float x, float y) const;
};
