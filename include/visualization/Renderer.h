// F// FILE: include/visualization/Renderer.h
#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <cmath>
#include "core/Vehicle.h" // For Direction enumerator

class Lane;
class TrafficLight;
class TrafficManager;

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Initialize renderer with window dimensions
    bool initialize(int width, int height, const std::string& title);

    // Start rendering loop
    void startRenderLoop();

    // Set traffic manager to render
    void setTrafficManager(TrafficManager* manager);

    // Render a single frame
    void renderFrame();

    // Clean up resource
    void cleanup();

    // Check if rendering is active
    bool isActive() const;

    // Toggle debug overlay
    void toggleDebugOverlay();

    // Set frame rate limiter
    void setFrameRateLimit(int fps);

private:
    // SDL components
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* carTexture;
    SDL_Surface* surface;

    // Rendering state
    bool active;
    bool showDebugOverlay;
    int frameRateLimit;
    uint32_t lastFrameTime;

    // Window dimensions
    int windowWidth;
    int windowHeight;

    // Traffic manager
    TrafficManager* trafficManager;

    // Helper drawing functions
    void drawRoadsAndLanes();
    void drawTrafficLights();
    void drawVehicles();
    void drawDebugOverlay();
    void drawLaneLabels();
    void drawStatistics();
    void drawAlertIcon(int x, int y);

    // Modern UI drawing functions
    void drawCityBlocks();
    void drawBuildingWindows(int buildingX, int buildingY, int buildingWidth, int buildingHeight);
    void drawRoadTexture();
    void drawLaneDividers();
    void drawLaneIndicators();
    void drawCrosswalks();
    void drawStopLines();
    void drawLaneMarker(int x, int y, const std::string& label, SDL_Color color, bool isVertical);
    void drawNeonSign(int x, int y, const std::string& text, SDL_Color color, bool isHorizontal);
    void drawNeonChar(float x, float y, char c, SDL_Color color, bool isVertical);
    void drawLaneLegend();
    void drawLaneFlowArrow(int x, int y, Direction dir);

    // Text and character rendering
    void drawText(const std::string& text, int x, int y, SDL_Color color);
    void drawSimpleChar(char c, int x, int y);

    // Vehicle rendering
    void renderModernVehicle(Vehicle* vehicle, int queuePos);
    void drawVehicleLights(float x, float y, int laneNumber, char laneChar,
                          Direction dir, bool isTurning, Destination destination);

    // Load textures
    bool loadTextures();

    // Process SDL events
    bool processEvents();

    // Helper to draw a filled arrow
    void drawArrow(int x1, int y1, int x2, int y2, int x3, int y3, SDL_Color color);
    void drawDirectionArrow(int x, int y, Direction dir, SDL_Color color);
};

#endif // RENDERER_Hf // RENDERER_Hendif // RENDERER_Hendif // RENDERER_Hendif // RENDERER_Hendif // RENDERER_H
