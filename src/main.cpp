#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <SDL3/SDL.h>
#include <fstream>
#include <filesystem>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <cmath>

// Include the necessary headers
#include "core/Vehicle.h"
#include "core/Lane.h"
#include "core/TrafficLight.h"
#include "managers/TrafficManager.h"
#include "managers/FileHandler.h"
#include "visualization/Renderer.h"
#include "utils/DebugLogger.h"

namespace fs = std::filesystem;

// Constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const std::string DATA_DIR = "data/lanes";
const int PRIORITY_THRESHOLD_HIGH = 10;
const int PRIORITY_THRESHOLD_LOW = 5;

// Car colors for variety
const SDL_Color CAR_COLORS[] = {
    {200, 0, 0, 255},    // Red
    {0, 0, 200, 255},    // Blue
    {0, 150, 0, 255},    // Green
    {150, 150, 0, 255},  // Yellow
    {100, 100, 100, 255},// Gray
    {0, 150, 150, 255},  // Teal
    {150, 0, 150, 255},  // Purple
    {200, 100, 0, 255},  // Orange
    {0, 0, 0, 255}       // Black
};
const int NUM_CAR_COLORS = 9;

// Road colors
const SDL_Color ROAD_COLOR = {60, 60, 60, 255};
const SDL_Color LANE_MARKER_COLOR = {255, 255, 255, 255};
const SDL_Color YELLOW_MARKER_COLOR = {255, 255, 0, 255};
const SDL_Color INTERSECTION_COLOR = {50, 50, 50, 255};
const SDL_Color SIDEWALK_COLOR = {180, 180, 180, 255};
const SDL_Color GRASS_COLOR = {100, 200, 100, 255};

// Simple logging function
void log_message(const std::string& msg) {
    std::cout << "[Simulator] " << msg << std::endl;

    // Also log to file
    std::ofstream log("simulator_debug.log", std::ios::app);
    if (log.is_open()) {
        log << "[Simulator] " << msg << std::endl;
        log.close();
    }

    // Also use DebugLogger
    DebugLogger::log(msg);
}

// Ensure data directories exist
bool ensure_directories() {
    try {
        if (!fs::exists(DATA_DIR)) {
            fs::create_directories(DATA_DIR);
            log_message("Created directory: " + DATA_DIR);
        }
        return true;
    } catch (const std::exception& e) {
        log_message("Error creating directories: " + std::string(e.what()));
        return false;
    }
}

// Create a random number generator
std::mt19937 rng(std::random_device{}());

// Direction enum
enum class VehicleDirection {
    STRAIGHT = 0,
    LEFT = 1,
    RIGHT = 2
};

// The RenderSystem class for main visualization
class RenderSystem {
public:
    SDL_Window* window;
    SDL_Renderer* rendererSDL;
    int windowWidth;
    int windowHeight;
    bool active;
    bool showDebug;
    TrafficManager* trafficMgr;

    RenderSystem()
        : window(nullptr),
          rendererSDL(nullptr),
          windowWidth(800),
          windowHeight(800),
          active(false),
          showDebug(false), // Set to false to disable debug overlay
          trafficMgr(nullptr) {}

    ~RenderSystem() {
        cleanup();
    }

    // Initialize renderer
    bool initialize(int width, int height, const std::string& title) {
        windowWidth = width;
        windowHeight = height;

        // Create window
        window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL);
        if (!window) {
            log_message("Failed to create window: " + std::string(SDL_GetError()));
            return false;
        }

        // Create renderer
        rendererSDL = SDL_CreateRenderer(window, NULL);
        if (!rendererSDL) {
            log_message("Failed to create renderer: " + std::string(SDL_GetError()));
            return false;
        }

        active = true;
        log_message("Renderer initialized successfully");
        return true;
    }

    // Set traffic manager
    void setTrafficManager(TrafficManager* manager) {
        trafficMgr = manager;
    }

    // Draw a realistic road layout
    void drawRoadLayout() {
        const int ROAD_WIDTH = 150;
        const int LANE_WIDTH = 50;
        const int SIDEWALK_WIDTH = 20;

        // Draw grass background
        SDL_SetRenderDrawColor(rendererSDL, GRASS_COLOR.r, GRASS_COLOR.g, GRASS_COLOR.b, GRASS_COLOR.a);
        SDL_RenderClear(rendererSDL);

        // Draw sidewalks
        SDL_SetRenderDrawColor(rendererSDL, SIDEWALK_COLOR.r, SIDEWALK_COLOR.g, SIDEWALK_COLOR.b, SIDEWALK_COLOR.a);

        // Horizontal sidewalks
        SDL_FRect hSidewalk1 = {0, (float)(windowHeight/2 - ROAD_WIDTH/2 - SIDEWALK_WIDTH),
                               (float)windowWidth, (float)SIDEWALK_WIDTH};
        SDL_FRect hSidewalk2 = {0, (float)(windowHeight/2 + ROAD_WIDTH/2),
                               (float)windowWidth, (float)SIDEWALK_WIDTH};
        SDL_RenderFillRect(rendererSDL, &hSidewalk1);
        SDL_RenderFillRect(rendererSDL, &hSidewalk2);

        // Vertical sidewalks
        SDL_FRect vSidewalk1 = {(float)(windowWidth/2 - ROAD_WIDTH/2 - SIDEWALK_WIDTH), 0,
                               (float)SIDEWALK_WIDTH, (float)windowHeight};
        SDL_FRect vSidewalk2 = {(float)(windowWidth/2 + ROAD_WIDTH/2), 0,
                               (float)SIDEWALK_WIDTH, (float)windowHeight};
        SDL_RenderFillRect(rendererSDL, &vSidewalk1);
        SDL_RenderFillRect(rendererSDL, &vSidewalk2);

        // Draw main roads (dark gray)
        SDL_SetRenderDrawColor(rendererSDL, ROAD_COLOR.r, ROAD_COLOR.g, ROAD_COLOR.b, ROAD_COLOR.a);

        // Horizontal road
        SDL_FRect hRoad = {0, (float)(windowHeight/2 - ROAD_WIDTH/2),
                          (float)windowWidth, (float)ROAD_WIDTH};
        SDL_RenderFillRect(rendererSDL, &hRoad);

        // Vertical road
        SDL_FRect vRoad = {(float)(windowWidth/2 - ROAD_WIDTH/2), 0,
                          (float)ROAD_WIDTH, (float)windowHeight};
        SDL_RenderFillRect(rendererSDL, &vRoad);

        // Draw intersection (slightly darker)
        SDL_SetRenderDrawColor(rendererSDL, INTERSECTION_COLOR.r, INTERSECTION_COLOR.g, INTERSECTION_COLOR.b, INTERSECTION_COLOR.a);
        SDL_FRect intersection = {(float)(windowWidth/2 - ROAD_WIDTH/2), (float)(windowHeight/2 - ROAD_WIDTH/2),
                                 (float)ROAD_WIDTH, (float)ROAD_WIDTH};
        SDL_RenderFillRect(rendererSDL, &intersection);

        // Draw lane dividers
        // Horizontal lane dividers
        for (int i = 1; i < 3; i++) {
            int y = windowHeight/2 - ROAD_WIDTH/2 + i * LANE_WIDTH;

            if (i == 1) {
                // Center line (yellow)
                SDL_SetRenderDrawColor(rendererSDL, YELLOW_MARKER_COLOR.r, YELLOW_MARKER_COLOR.g,
                                     YELLOW_MARKER_COLOR.b, YELLOW_MARKER_COLOR.a);
            } else {
                // Other lane dividers (white)
                SDL_SetRenderDrawColor(rendererSDL, LANE_MARKER_COLOR.r, LANE_MARKER_COLOR.g,
                                     LANE_MARKER_COLOR.b, LANE_MARKER_COLOR.a);
            }

            // Draw dashed lines outside intersection
            for (int x = 0; x < windowWidth; x += 30) {
                if (x < windowWidth/2 - ROAD_WIDTH/2 || x > windowWidth/2 + ROAD_WIDTH/2) {
                    SDL_FRect line = {(float)x, (float)y - 2, 15, 4};
                    SDL_RenderFillRect(rendererSDL, &line);
                }
            }
        }

        // Vertical lane dividers
        for (int i = 1; i < 3; i++) {
            int x = windowWidth/2 - ROAD_WIDTH/2 + i * LANE_WIDTH;

            if (i == 1) {
                // Center line (yellow)
                SDL_SetRenderDrawColor(rendererSDL, YELLOW_MARKER_COLOR.r, YELLOW_MARKER_COLOR.g,
                                     YELLOW_MARKER_COLOR.b, YELLOW_MARKER_COLOR.a);
            } else {
                // Other lane dividers (white)
                SDL_SetRenderDrawColor(rendererSDL, LANE_MARKER_COLOR.r, LANE_MARKER_COLOR.g,
                                     LANE_MARKER_COLOR.b, LANE_MARKER_COLOR.a);
            }

            // Draw dashed lines outside intersection
            for (int y = 0; y < windowHeight; y += 30) {
                if (y < windowHeight/2 - ROAD_WIDTH/2 || y > windowHeight/2 + ROAD_WIDTH/2) {
                    SDL_FRect line = {(float)x - 2, (float)y, 4, 15};
                    SDL_RenderFillRect(rendererSDL, &line);
                }
            }
        }

        // Draw crosswalks
        SDL_SetRenderDrawColor(rendererSDL, 255, 255, 255, 255);

        // North crosswalk
        for (int i = 0; i < 10; i++) {
            SDL_FRect stripe = {(float)(windowWidth/2 - ROAD_WIDTH/2 + 15*i),
                               (float)(windowHeight/2 - ROAD_WIDTH/2 - 15), 10, 15};
            SDL_RenderFillRect(rendererSDL, &stripe);
        }

        // South crosswalk
        for (int i = 0; i < 10; i++) {
            SDL_FRect stripe = {(float)(windowWidth/2 - ROAD_WIDTH/2 + 15*i),
                               (float)(windowHeight/2 + ROAD_WIDTH/2), 10, 15};
            SDL_RenderFillRect(rendererSDL, &stripe);
        }

        // East crosswalk
        for (int i = 0; i < 10; i++) {
            SDL_FRect stripe = {(float)(windowWidth/2 + ROAD_WIDTH/2),
                               (float)(windowHeight/2 - ROAD_WIDTH/2 + 15*i), 15, 10};
            SDL_RenderFillRect(rendererSDL, &stripe);
        }

        // West crosswalk
        for (int i = 0; i < 10; i++) {
            SDL_FRect stripe = {(float)(windowWidth/2 - ROAD_WIDTH/2 - 15),
                               (float)(windowHeight/2 - ROAD_WIDTH/2 + 15*i), 15, 10};
            SDL_RenderFillRect(rendererSDL, &stripe);
        }
    }

    // Render a frame
    void renderFrame() {
        if (!active || !rendererSDL || !trafficMgr) {
            return;
        }

        // Draw realistic road layout
        drawRoadLayout();

        // Draw traffic lights
        if (trafficMgr->getTrafficLight()) {
            trafficMgr->getTrafficLight()->render(rendererSDL);
        }

        // Draw vehicles
        for (auto* lane : trafficMgr->getLanes()) {
            for (auto* vehicle : lane->getVehicles()) {
                if (vehicle) {
                    // Create default parameters for vehicle rendering
                    int queuePos = 0; // Not important for this call
                    vehicle->render(rendererSDL, nullptr, queuePos);
                }
            }
        }

        // Draw debug overlay only if enabled
        if (showDebug) {
            drawDebugOverlay();
        }

        // Present render
        SDL_RenderPresent(rendererSDL);
    }

    // Start render loop
    void startRenderLoop() {
        if (!active) {
            return;
        }

        log_message("Starting render loop");

        bool running = true;
        uint32_t lastUpdateTime = SDL_GetTicks();

        while (running) {
            // Process events
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (event.type == SDL_EVENT_KEY_DOWN) {
                    // Fixed SDL3 key handling
                    int key = event.key.which;

                    if (key == SDL_SCANCODE_D) {
                        showDebug = !showDebug;
                        log_message("Debug overlay " + std::string(showDebug ? "enabled" : "disabled"));
                    } else if (key == SDL_SCANCODE_ESCAPE) {
                        running = false;
                    }
                }
            }

            // Calculate delta time
            uint32_t currentTime = SDL_GetTicks();
            uint32_t deltaTime = currentTime - lastUpdateTime;

            // Update traffic manager
            if (trafficMgr) {
                trafficMgr->update(deltaTime);
            }

            // Render frame
            renderFrame();

            // Limit frame rate
            SDL_Delay(16); // ~60 FPS

            lastUpdateTime = currentTime;
        }
    }

    // Draw debug overlay - now much smaller and less intrusive
    void drawDebugOverlay() {
        if (!trafficMgr) return;

        // Draw semi-transparent background
        SDL_SetRenderDrawColor(rendererSDL, 0, 0, 0, 180);
        SDL_SetRenderDrawBlendMode(rendererSDL, SDL_BLENDMODE_BLEND);
        SDL_FRect overlayRect = {10, 10, 200, 100}; // Much smaller overlay
        SDL_RenderFillRect(rendererSDL, &overlayRect);
        SDL_SetRenderDrawBlendMode(rendererSDL, SDL_BLENDMODE_NONE);

        // Draw border
        SDL_SetRenderDrawColor(rendererSDL, 255, 255, 255, 255);
        SDL_RenderRect(rendererSDL, &overlayRect);

        // Function to draw text (simplified with rectangles)
        auto drawText = [this](const std::string& text, float x, float y, SDL_Color color) {
            SDL_SetRenderDrawColor(rendererSDL, color.r, color.g, color.b, color.a);
            SDL_FRect rect = {x, y, text.length() * 7.0f, 16.0f};
            SDL_RenderFillRect(rendererSDL, &rect);

            // Black border
            SDL_SetRenderDrawColor(rendererSDL, 0, 0, 0, 255);
            SDL_RenderRect(rendererSDL, &rect);
        };

        // Title
        drawText("Traffic Simulator", 20, 20, {255, 255, 255, 255});

        // Traffic light state
        std::string stateStr = "Light: ";
        SDL_Color stateColor = {255, 255, 255, 255};

        auto currentState = trafficMgr->getTrafficLight()->getCurrentState();

        switch (currentState) {
            case TrafficLight::State::ALL_RED:
                stateStr += "All Red";
                stateColor = {255, 100, 100, 255};
                break;
            case TrafficLight::State::A_GREEN:
                stateStr += "A Green";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::B_GREEN:
                stateStr += "B Green";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::C_GREEN:
                stateStr += "C Green";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::D_GREEN:
                stateStr += "D Green";
                stateColor = {100, 255, 100, 255};
                break;
        }

        drawText(stateStr, 20, 45, stateColor);

        // Priority status
        bool isPriorityMode = false;
        Lane* priorityLane = trafficMgr->getPriorityLane();
        if (priorityLane && priorityLane->getPriority() > 0) {
            isPriorityMode = true;
            drawText("PRIORITY MODE", 20, 70, {255, 165, 0, 255});
        }
    }

    // Clean up resources
    void cleanup() {
        if (rendererSDL) {
            SDL_DestroyRenderer(rendererSDL);
            rendererSDL = nullptr;
        }

        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        active = false;
    }
};

// Main function
int main(int argc, char* argv[]) {
    try {
        // Initialize debug logger
        DebugLogger::initialize();
        log_message("Starting Traffic Junction Simulator");


        // Create traffic manager
        TrafficManager trafficManager;
        if (!trafficManager.initialize()) {
            log_message("Failed to initialize traffic manager");
            SDL_Quit();
            return 1;
        }

        // Create renderer
        RenderSystem renderer;
        if (!renderer.initialize(WINDOW_WIDTH, WINDOW_HEIGHT, "Traffic Junction Simulator")) {
            log_message("Failed to initialize renderer");
            SDL_Quit();
            return 1;
        }

        // Connect traffic manager to renderer
        renderer.setTrafficManager(&trafficManager);

        // Start traffic manager
        trafficManager.start();

        // Start render loop
        renderer.startRenderLoop();

        // Cleanup
        trafficManager.stop();
        renderer.cleanup();
        SDL_Quit();

        log_message("Simulator shutdown complete");
        return 0;
    }
    catch (const std::exception& e) {
        log_message("Unhandled exception: " + std::string(e.what()));
        SDL_Quit();
        return 1;
    }
}
