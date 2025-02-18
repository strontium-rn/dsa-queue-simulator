// src/core/TrafficLight.cpp
#include "core/TrafficLight.h"

TrafficLight::TrafficLight()
    : state(LightState::RED)
    , nextState(LightState::RED)
    , transitionProgress(0.0f)
    , transitionDuration(1.0f)
    , stateTimer(0.0f)
    , isTransitioning(false) {}

void TrafficLight::update(float deltaTime) {
    stateTimer += deltaTime;

    if (isTransitioning) {
        transitionProgress += deltaTime / transitionDuration;
        if (transitionProgress >= 1.0f) {
            transitionProgress = 0.0f;
            isTransitioning = false;
            state = nextState;
        }
    }
    else if (stateTimer >= 5.0f) {  // 5 seconds per state
        stateTimer = 0.0f;
        isTransitioning = true;
        nextState = (state == LightState::RED) ? LightState::GREEN : LightState::RED;
    }
}

void TrafficLight::setState(LightState newState) {
    state = newState;
    stateTimer = 0.0f;
}

LightState TrafficLight::getState() const {
    return state;
}

void TrafficLight::render(SDL_Renderer* renderer, float x, float y) const {
    const float SIZE = 30.0f;
    const float SPACING = 40.0f;

    // Draw light housing
    SDL_FRect housing = {x - 5, y - 5, SIZE + 10, (SIZE * 2) + SPACING + 10};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &housing);

    // Draw red light
    SDL_FRect redLight = {x, y, SIZE, SIZE};
    float redIntensity = (state == LightState::RED) ?
        (isTransitioning ? 1.0f - transitionProgress : 1.0f) :
        (isTransitioning ? transitionProgress : 0.0f);
    SDL_SetRenderDrawColor(renderer,
        static_cast<uint8_t>(255 * redIntensity),
        0,
        0,
        255);
    SDL_RenderFillRect(renderer, &redLight);

    // Draw green light
    SDL_FRect greenLight = {x, y + SIZE + SPACING, SIZE, SIZE};
    float greenIntensity = (state == LightState::GREEN) ?
        (isTransitioning ? 1.0f - transitionProgress : 1.0f) :
        (isTransitioning ? transitionProgress : 0.0f);
    SDL_SetRenderDrawColor(renderer,
        0,
        static_cast<uint8_t>(255 * greenIntensity),
        0,
        255);
    SDL_RenderFillRect(renderer, &greenLight);

    // Draw borders
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &redLight);
    SDL_RenderRect(renderer, &greenLight);
}
