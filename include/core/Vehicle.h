// FILE: include/c// FILE: include/core/Vehicle.h
#ifndef VEHICLE_H
#define VEHICLE_H

#include <string>
#include <SDL3/SDL.h>
#include <ctime>
#include <vector>
#include <sstream>
#include "utils/DebugLogger.h"

// Define all enums here instead of just forward declaring them
enum class Destination {
    STRAIGHT,
    LEFT,
    RIGHT
};

enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

enum class VehicleState {
    APPROACHING,
    IN_INTERSECTION,
    EXITING,
    EXITED
};

// Point structure for waypoints
struct Point {
    float x;
    float y;
};

class Vehicle {
public:
    Vehicle(const std::string& id, char lane, int laneNumber, bool isEmergency = false);
    ~Vehicle();

    // Getters and setters
    std::string getId() const;
    char getLane() const;
    void setLane(char lane);
    int getLaneNumber() const;
    void setLaneNumber(int number);
    bool isEmergencyVehicle() const;
    time_t getArrivalTime() const;

    // Destination control
    void setDestination(Destination dest);
    Destination getDestination() const;

    // Animation related
    float getAnimationPos() const;
    void setAnimationPos(float pos);
    bool isTurning() const;
    void setTurning(bool turning);
    float getTurnProgress() const;
    void setTurnProgress(float progress);
    float getTurnPosX() const;
    void setTurnPosX(float x);
    float getTurnPosY() const;
    void setTurnPosY(float y);

    // Update vehicle position
    void update(uint32_t delta, bool isGreenLight, float targetPos);

    // Render vehicle
    void render(SDL_Renderer* renderer, SDL_Texture* vehicleTexture, int queuePos);

    // Calculate turn path
    void calculateTurnPath(float startX, float startY, float controlX, float controlY,
                          float endX, float endY, float progress);

    // Initialize waypoints for movement path
    void initializeWaypoints();

    // Check if vehicle has exited the screen
    bool hasExited() const { return state == VehicleState::EXITED; }

private:
    std::string id;
    char lane;
    int laneNumber;
    bool isEmergency;
    time_t arrivalTime;

    // Animation properties
    float animPos;
    bool turning;
    float turnProgress;
    float turnPosX;
    float turnPosY;
    int queuePos; // Position in the queue for proper spacing

    // Destination (where the vehicle is heading)
    Destination destination;

    // Current direction of travel
    Direction currentDirection;

    // Vehicle state
    VehicleState state;

    // Waypoints for movement
    std::vector<Point> waypoints;
    size_t currentWaypoint;

    // Helper methods
    float easeInOutQuad(float t) const;

    // Helper for drawing triangles (SDL3 compatible)
    void SDL_RenderFillTriangleF(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float x3, float y3);
};

#endif // VEHICLE_H
