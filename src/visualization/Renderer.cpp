// FILE: src/visualization/Renderer.cpp workswell
#include "visualization/Renderer.h"
#include "core/Lane.h"
#include "core/Vehicle.h"
#include "core/TrafficLight.h"
#include "managers/TrafficManager.h"
#include "utils/DebugLogger.h"
#include "core/Constants.h"

#include <sstream>
#include <algorithm>
#include <cmath>

Renderer::Renderer()
    : window(nullptr),
      renderer(nullptr),
      carTexture(nullptr),
      surface(nullptr),
      active(false),
      showDebugOverlay(true),
      frameRateLimit(60),
      lastFrameTime(0),
      windowWidth(800),
      windowHeight(800),
      trafficManager(nullptr) {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::initialize(int width, int height, const std::string& title) {
    windowWidth = width;
    windowHeight = height;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        DebugLogger::log("Failed to initialize SDL: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Create window
    window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL);
    if (!window) {
        DebugLogger::log("Failed to create window: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        DebugLogger::log("Failed to create renderer: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Load textures
    if (!loadTextures()) {
        DebugLogger::log("Failed to load textures", DebugLogger::LogLevel::ERROR);
        return false;
    }

    active = true;
    DebugLogger::log("Renderer initialized successfully");

    return true;
}

bool Renderer::loadTextures() {
    // Create a simple surface directly with a solid color to avoid SDL_MapRGB issues
    surface = SDL_CreateSurface(20, 10, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        DebugLogger::log("Failed to create surface: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Fill with blue color using a simpler approach
    // Create a color value manually
    Uint32 blueColor = 0x0000FFFF; // RGBA format: blue with full alpha

    // Fill the entire surface with this color
    SDL_FillSurfaceRect(surface, NULL, blueColor);

    carTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    surface = nullptr;

    if (!carTexture) {
        DebugLogger::log("Failed to create car texture: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    return true;
}

void Renderer::startRenderLoop() {
    if (!active || !trafficManager) {
        DebugLogger::log("Cannot start render loop - renderer not active or trafficManager not set", DebugLogger::LogLevel::ERROR);
        return;
    }

    DebugLogger::log("Starting render loop");

    uint32_t lastUpdate = SDL_GetTicks();
    const int updateInterval = 16; // ~60 FPS

    while (active) {
        uint32_t currentTime = SDL_GetTicks();
        uint32_t deltaTime = currentTime - lastUpdate;

        if (deltaTime >= updateInterval) {
            // Process events
            active = processEvents();

            // Update traffic manager
            trafficManager->update(deltaTime);

            // Render frame
            renderFrame();

            lastUpdate = currentTime;
        }

        // Delay to maintain frame rate
        uint32_t frameDuration = SDL_GetTicks() - currentTime;
        if (frameRateLimit > 0) {
            uint32_t targetFrameTime = 1000 / frameRateLimit;
            if (frameDuration < targetFrameTime) {
                SDL_Delay(targetFrameTime - frameDuration);
            }
        }
    }
}

bool Renderer::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                return false;

            case SDL_EVENT_KEY_DOWN: {
                // Check based on the key scancode instead of using SDLK constants
                SDL_Scancode scancode = event.key.scancode;

                // D key scancode is usually 7 (for SDL_SCANCODE_D)
                if (scancode == SDL_SCANCODE_D) {
                    toggleDebugOverlay();
                }
                // Escape key scancode is usually 41 (for SDL_SCANCODE_ESCAPE)
                else if (scancode == SDL_SCANCODE_ESCAPE) {
                    return false;
                }
                break;
            }
        }
    }

    return true;
}

void Renderer::renderFrame() {
    if (!active || !renderer || !trafficManager) {
        return;
    }

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255); // Darker background
    SDL_RenderClear(renderer);

    // Draw roads and lanes
    drawRoadsAndLanes();

    // Draw traffic lights
    drawTrafficLights();

    // Draw vehicles
    drawVehicles();

    // Draw lane labels and direction indicators
    drawLaneLabels();

    // Draw debug overlay if enabled
    if (showDebugOverlay) {
        drawDebugOverlay();
    }

    // Present render
    SDL_RenderPresent(renderer);

    // Update frame time
    lastFrameTime = SDL_GetTicks();
}


void Renderer::drawRoadsAndLanes() {
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;
    const int LANE_WIDTH = Constants::LANE_WIDTH;
    const int CENTER_X = windowWidth / 2;
    const int CENTER_Y = windowHeight / 2;

    // ---------- STEP 1: BACKGROUND ----------
    // Draw dark gray background for the entire window
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderClear(renderer);

    // Draw grass areas in corners (to highlight road areas)
    SDL_SetRenderDrawColor(renderer, 30, 100, 30, 255);  // Dark green grass

    // Top-left grass
    SDL_FRect grassTL = {
        0, 0,
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2)
    };
    SDL_RenderFillRect(renderer, &grassTL);

    // Top-right grass
    SDL_FRect grassTR = {
        static_cast<float>(CENTER_X + ROAD_WIDTH/2), 0,
        static_cast<float>(windowWidth - (CENTER_X + ROAD_WIDTH/2)),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2)
    };
    SDL_RenderFillRect(renderer, &grassTR);

    // Bottom-left grass
    SDL_FRect grassBL = {
        0, static_cast<float>(CENTER_Y + ROAD_WIDTH/2),
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(windowHeight - (CENTER_Y + ROAD_WIDTH/2))
    };
    SDL_RenderFillRect(renderer, &grassBL);

    // Bottom-right grass
    SDL_FRect grassBR = {
        static_cast<float>(CENTER_X + ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y + ROAD_WIDTH/2),
        static_cast<float>(windowWidth - (CENTER_X + ROAD_WIDTH/2)),
        static_cast<float>(windowHeight - (CENTER_Y + ROAD_WIDTH/2))
    };
    SDL_RenderFillRect(renderer, &grassBR);

    // ---------- STEP 2: DRAW BASE ROADS ----------
    // Draw horizontal road (mid-gray)
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_FRect horizontalRoad = {
        0, static_cast<float>(CENTER_Y - ROAD_WIDTH/2),
        static_cast<float>(windowWidth), static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &horizontalRoad);

    // Draw vertical road (mid-gray)
    SDL_FRect verticalRoad = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2), 0,
        static_cast<float>(ROAD_WIDTH), static_cast<float>(windowHeight)
    };
    SDL_RenderFillRect(renderer, &verticalRoad);

    // Draw intersection (slightly darker)
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_FRect intersection = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2),
        static_cast<float>(ROAD_WIDTH),
        static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &intersection);

    // ---------- STEP 3: DRAW LANES WITH DISTINCT COLORS ----------
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // --- ROAD A (NORTH) ---
    // A1 - Incoming lane (light blue with direction indicator)
    SDL_SetRenderDrawColor(renderer, 30, 144, 255, 80); // Light blue transparent
    SDL_FRect laneA1 = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        0,
        static_cast<float>(LANE_WIDTH),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2)
    };
    SDL_RenderFillRect(renderer, &laneA1);

    // A2 - Priority lane (orange with priority indicator)
    SDL_SetRenderDrawColor(renderer, 255, 140, 0, 80); // Orange transparent
    SDL_FRect laneA2 = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH),
        0,
        static_cast<float>(LANE_WIDTH),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2)
    };
    SDL_RenderFillRect(renderer, &laneA2);

    // A3 - Free lane (green with free lane indicator)
    SDL_SetRenderDrawColor(renderer, 50, 205, 50, 80); // Lime green transparent
    SDL_FRect laneA3 = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2 + 2*LANE_WIDTH),
        0,
        static_cast<float>(LANE_WIDTH),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2)
    };
    SDL_RenderFillRect(renderer, &laneA3);

    // --- ROAD B (EAST) ---
    // B1 - Incoming lane (light blue)
    SDL_SetRenderDrawColor(renderer, 30, 144, 255, 80);
    SDL_FRect laneB1 = {
        static_cast<float>(CENTER_X + ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2),
        static_cast<float>(windowWidth - (CENTER_X + ROAD_WIDTH/2)),
        static_cast<float>(LANE_WIDTH)
    };
    SDL_RenderFillRect(renderer, &laneB1);

    // B2 - Normal lane (light yellow)
    SDL_SetRenderDrawColor(renderer, 218, 165, 32, 80); // Goldenrod transparent
    SDL_FRect laneB2 = {
        static_cast<float>(CENTER_X + ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2 + LANE_WIDTH),
        static_cast<float>(windowWidth - (CENTER_X + ROAD_WIDTH/2)),
        static_cast<float>(LANE_WIDTH)
    };
    SDL_RenderFillRect(renderer, &laneB2);

    // B3 - Free lane (green)
    SDL_SetRenderDrawColor(renderer, 34, 139, 34, 80); // Forest green transparent
    SDL_FRect laneB3 = {
        static_cast<float>(CENTER_X + ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2 + 2*LANE_WIDTH),
        static_cast<float>(windowWidth - (CENTER_X + ROAD_WIDTH/2)),
        static_cast<float>(LANE_WIDTH)
    };
    SDL_RenderFillRect(renderer, &laneB3);

    // --- ROAD C (SOUTH) ---
    // C1 - Incoming lane (light blue)
    SDL_SetRenderDrawColor(renderer, 30, 144, 255, 80);
    SDL_FRect laneC1 = {
        static_cast<float>(CENTER_X + LANE_WIDTH),
        static_cast<float>(CENTER_Y + ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH),
        static_cast<float>(windowHeight - (CENTER_Y + ROAD_WIDTH/2))
    };
    SDL_RenderFillRect(renderer, &laneC1);

    // C2 - Normal lane (light brown)
    SDL_SetRenderDrawColor(renderer, 210, 105, 30, 80); // Chocolate transparent
    SDL_FRect laneC2 = {
        static_cast<float>(CENTER_X),
        static_cast<float>(CENTER_Y + ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH),
        static_cast<float>(windowHeight - (CENTER_Y + ROAD_WIDTH/2))
    };
    SDL_RenderFillRect(renderer, &laneC2);

    // C3 - Free lane (green)
    SDL_SetRenderDrawColor(renderer, 60, 179, 113, 80); // Medium sea green transparent
    SDL_FRect laneC3 = {
        static_cast<float>(CENTER_X - LANE_WIDTH),
        static_cast<float>(CENTER_Y + ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH),
        static_cast<float>(windowHeight - (CENTER_Y + ROAD_WIDTH/2))
    };
    SDL_RenderFillRect(renderer, &laneC3);

    // --- ROAD D (WEST) ---
    // D1 - Incoming lane (light blue)
    SDL_SetRenderDrawColor(renderer, 30, 144, 255, 80);
    SDL_FRect laneD1 = {
        0,
        static_cast<float>(CENTER_Y + LANE_WIDTH),
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH)
    };
    SDL_RenderFillRect(renderer, &laneD1);

    // D2 - Normal lane (light brown)
    SDL_SetRenderDrawColor(renderer, 205, 133, 63, 80); // Peru transparent
    SDL_FRect laneD2 = {
        0,
        static_cast<float>(CENTER_Y),
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH)
    };
    SDL_RenderFillRect(renderer, &laneD2);

    // D3 - Free lane (green)
    SDL_SetRenderDrawColor(renderer, 46, 139, 87, 80); // Sea green transparent
    SDL_FRect laneD3 = {
        0,
        static_cast<float>(CENTER_Y - LANE_WIDTH),
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH)
    };
    SDL_RenderFillRect(renderer, &laneD3);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // ---------- STEP 4: DRAW LANE DIVIDERS ----------
    // Draw the center double-yellow lines
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow

    // Horizontal center double line
    SDL_FRect hCenterLine1 = {
        0, static_cast<float>(CENTER_Y - 1),
        static_cast<float>(windowWidth), 2.0f
    };
    SDL_FRect hCenterLine2 = {
        0, static_cast<float>(CENTER_Y - 5),
        static_cast<float>(windowWidth), 2.0f
    };
    SDL_RenderFillRect(renderer, &hCenterLine1);
    SDL_RenderFillRect(renderer, &hCenterLine2);

    // Vertical center double line
    SDL_FRect vCenterLine1 = {
        static_cast<float>(CENTER_X - 1), 0,
        2.0f, static_cast<float>(windowHeight)
    };
    SDL_FRect vCenterLine2 = {
        static_cast<float>(CENTER_X - 5), 0,
        2.0f, static_cast<float>(windowHeight)
    };
    SDL_RenderFillRect(renderer, &vCenterLine1);
    SDL_RenderFillRect(renderer, &vCenterLine2);

    // Draw white lane dividers (dashed lines)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Horizontal lane dividers
    for (int i = 1; i < 3; i++) {
        if (i == 1) continue; // Skip the center line (already drawn as yellow)

        float y1 = CENTER_Y - ROAD_WIDTH/2 + i * LANE_WIDTH;
        float y2 = CENTER_Y + i * LANE_WIDTH;

        // Top road lanes (going down)
        for (int x = 0; x < CENTER_X - ROAD_WIDTH/2; x += 30) {
            SDL_RenderLine(renderer, x, y1, x + 15, y1);
        }

        // Bottom road lanes (going up)
        for (int x = CENTER_X + ROAD_WIDTH/2; x < windowWidth; x += 30) {
            SDL_RenderLine(renderer, x, y2, x + 15, y2);
        }
    }

    // Vertical lane dividers
    for (int i = 1; i < 3; i++) {
        if (i == 1) continue; // Skip the center line

        float x1 = CENTER_X - ROAD_WIDTH/2 + i * LANE_WIDTH;
        float x2 = CENTER_X + i * LANE_WIDTH;

        // Left road lanes (going right)
        for (int y = 0; y < CENTER_Y - ROAD_WIDTH/2; y += 30) {
            SDL_RenderLine(renderer, x1, y, x1, y + 15);
        }

        // Right road lanes (going left)
        for (int y = CENTER_Y + ROAD_WIDTH/2; y < windowHeight; y += 30) {
            SDL_RenderLine(renderer, x2, y, x2, y + 15);
        }
    }

    // ---------- STEP 5: DRAW DISTINCTIVE LANE IDENTIFIERS ----------
    // Lane identifiers using shapes and patterns (no text)

    // --- A1 Lane Identifier (North, Incoming) ---
    SDL_SetRenderDrawColor(renderer, 200, 200, 255, 255); // Light blue
    SDL_FRect a1Marker = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH/2 - 15),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2 - 30),
        30.0f, 20.0f
    };
    SDL_RenderFillRect(renderer, &a1Marker);

    // Blue A1 label - draw a rectangle with "1" in it
    SDL_SetRenderDrawColor(renderer, 0, 0, 180, 255);
    // Draw "1" using two lines
    SDL_RenderLine(renderer,
        CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH/2, CENTER_Y - ROAD_WIDTH/2 - 25,
        CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH/2, CENTER_Y - ROAD_WIDTH/2 - 15);

    // --- A2 Lane Identifier (North, Priority) ---
    // Orange A2 marker (priority lane)
    SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);
    SDL_FRect a2Marker = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH + LANE_WIDTH/2 - 15),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2 - 30),
        30.0f, 20.0f
    };
    SDL_RenderFillRect(renderer, &a2Marker);

    // Draw "2" using three lines
    SDL_SetRenderDrawColor(renderer, 180, 0, 0, 255);
    SDL_RenderLine(renderer,
        CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH + LANE_WIDTH/2 - 5, CENTER_Y - ROAD_WIDTH/2 - 25,
        CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH + LANE_WIDTH/2 + 5, CENTER_Y - ROAD_WIDTH/2 - 25);
    SDL_RenderLine(renderer,
        CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH + LANE_WIDTH/2 + 5, CENTER_Y - ROAD_WIDTH/2 - 25,
        CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH + LANE_WIDTH/2 - 5, CENTER_Y - ROAD_WIDTH/2 - 20);
    SDL_RenderLine(renderer,
        CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH + LANE_WIDTH/2 - 5, CENTER_Y - ROAD_WIDTH/2 - 20,
        CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH + LANE_WIDTH/2 + 5, CENTER_Y - ROAD_WIDTH/2 - 15);

    // A2 Priority indicator (star/asterisk shape)
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
    int px = CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH + LANE_WIDTH/2;
    int py = CENTER_Y - ROAD_WIDTH/2 - 40;
    int r = 8; // size
    for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159f / 4; // 8 directions
        SDL_RenderLine(renderer, px, py,
                      px + r * cos(angle), py + r * sin(angle));
    }

    // --- A3 Lane Identifier (North, Free) ---
    // Green A3 marker (free lane)
    SDL_SetRenderDrawColor(renderer, 50, 205, 50, 255);
    SDL_FRect a3Marker = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2 + 2*LANE_WIDTH + LANE_WIDTH/2 - 15),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2 - 30),
        30.0f, 20.0f
    };
    SDL_RenderFillRect(renderer, &a3Marker);

    // Draw "3" using three lines
    SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
    SDL_RenderLine(renderer,
        CENTER_X - ROAD_WIDTH/2 + 2*LANE_WIDTH + LANE_WIDTH/2 - 5, CENTER_Y - ROAD_WIDTH/2 - 25,
        CENTER_X - ROAD_WIDTH/2 + 2*LANE_WIDTH + LANE_WIDTH/2 + 5, CENTER_Y - ROAD_WIDTH/2 - 25);
    SDL_RenderLine(renderer,
        CENTER_X - ROAD_WIDTH/2 + 2*LANE_WIDTH + LANE_WIDTH/2 + 5, CENTER_Y - ROAD_WIDTH/2 - 25,
        CENTER_X - ROAD_WIDTH/2 + 2*LANE_WIDTH + LANE_WIDTH/2 + 5, CENTER_Y - ROAD_WIDTH/2 - 20);
    SDL_RenderLine(renderer,
        CENTER_X - ROAD_WIDTH/2 + 2*LANE_WIDTH + LANE_WIDTH/2 - 5, CENTER_Y - ROAD_WIDTH/2 - 20,
        CENTER_X - ROAD_WIDTH/2 + 2*LANE_WIDTH + LANE_WIDTH/2 + 5, CENTER_Y - ROAD_WIDTH/2 - 15);

    // Free lane indicator (curved left arrow)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    int fx = CENTER_X - ROAD_WIDTH/2 + 2*LANE_WIDTH + LANE_WIDTH/2;
    int fy = CENTER_Y - ROAD_WIDTH/2 - 45;
    // Draw arrow curve using lines approximating an arc
    for (int i = 0; i < 10; i++) {
        float angle1 = (i * 0.1f + 0.25f) * M_PI;
        float angle2 = ((i+1) * 0.1f + 0.25f) * M_PI;
        SDL_RenderLine(renderer,
            fx + 12 * cos(angle1), fy + 12 * sin(angle1),
            fx + 12 * cos(angle2), fy + 12 * sin(angle2));
    }
    // Arrow head
    SDL_RenderLine(renderer, fx, fy - 12, fx - 5, fy - 8);
    SDL_RenderLine(renderer, fx, fy - 12, fx + 5, fy - 8);

    // --- B LANE MARKERS (Similar pattern) ---
    // B1 Lane Identifier (East, Incoming)
    SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255);
    SDL_FRect b1Marker = {
        static_cast<float>(CENTER_X + ROAD_WIDTH/2 + 30),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2 + LANE_WIDTH/2 - 10),
        20.0f, 20.0f
    };
    SDL_RenderFillRect(renderer, &b1Marker);

    // Draw "B1" inside
    SDL_SetRenderDrawColor(renderer, 0, 0, 180, 255);
    // Draw "1"
    SDL_RenderLine(renderer,
        CENTER_X + ROAD_WIDTH/2 + 40, CENTER_Y - ROAD_WIDTH/2 + LANE_WIDTH/2 - 5,
        CENTER_X + ROAD_WIDTH/2 + 40, CENTER_Y - ROAD_WIDTH/2 + LANE_WIDTH/2 + 5);

    // --- C LANE MARKERS ---
    // C1 Lane Identifier (South, Incoming)
    SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255);
    SDL_FRect c1Marker = {
        static_cast<float>(CENTER_X + LANE_WIDTH + LANE_WIDTH/2 - 15),
        static_cast<float>(CENTER_Y + ROAD_WIDTH/2 + 10),
        30.0f, 20.0f
    };
    SDL_RenderFillRect(renderer, &c1Marker);

    // Draw "C1" inside
    SDL_SetRenderDrawColor(renderer, 0, 0, 180, 255);
    // Draw "1"
    SDL_RenderLine(renderer,
        CENTER_X + LANE_WIDTH + LANE_WIDTH/2, CENTER_Y + ROAD_WIDTH/2 + 15,
        CENTER_X + LANE_WIDTH + LANE_WIDTH/2, CENTER_Y + ROAD_WIDTH/2 + 25);

    // --- D LANE MARKERS ---
    // D1 Lane Identifier (West, Incoming)
    SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255);
    SDL_FRect d1Marker = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2 - 50),
        static_cast<float>(CENTER_Y + LANE_WIDTH + LANE_WIDTH/2 - 10),
        20.0f, 20.0f
    };
    SDL_RenderFillRect(renderer, &d1Marker);

    // Draw "D1" inside
    SDL_SetRenderDrawColor(renderer, 0, 0, 180, 255);
    // Draw "1"
    SDL_RenderLine(renderer,
        CENTER_X - ROAD_WIDTH/2 - 40, CENTER_Y + LANE_WIDTH + LANE_WIDTH/2 - 5,
        CENTER_X - ROAD_WIDTH/2 - 40, CENTER_Y + LANE_WIDTH + LANE_WIDTH/2 + 5);

    // ---------- STEP 6: DRAW LARGE ROAD IDENTIFIERS ----------
    // Draw a large "A" at the top (North Road)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    int aX = CENTER_X;
    int aY = 40;
    int aSize = 30;
    // Draw "A" using lines
    SDL_RenderLine(renderer, aX - aSize/2, aY + aSize/2, aX, aY - aSize/2); // Left diagonal
    SDL_RenderLine(renderer, aX, aY - aSize/2, aX + aSize/2, aY + aSize/2); // Right diagonal
    SDL_RenderLine(renderer, aX - aSize/4, aY, aX + aSize/4, aY); // Middle bar

    // Draw a large "B" at the right (East Road)
    int bX = windowWidth - 40;
    int bY = CENTER_Y;
    int bSize = 30;
    // Draw "B" using lines
    SDL_RenderLine(renderer, bX - bSize/2, bY - bSize/2, bX - bSize/2, bY + bSize/2); // Vertical
    SDL_RenderLine(renderer, bX - bSize/2, bY - bSize/2, bX + bSize/2, bY - bSize/4); // Top curve
    SDL_RenderLine(renderer, bX + bSize/2, bY - bSize/4, bX, bY); // Top to middle
    SDL_RenderLine(renderer, bX, bY, bX + bSize/2, bY + bSize/4); // Middle to bottom
    SDL_RenderLine(renderer, bX + bSize/2, bY + bSize/4, bX - bSize/2, bY + bSize/2); // Bottom curve

    // Draw a large "C" at the bottom (South Road)
    int cX = CENTER_X;
    int cY = windowHeight - 40;
    int cSize = 30;
    // Draw "C" using arc approximation with lines
    for (int i = 0; i < int(cSize/2); i++) {
        float angle = 0.75f * M_PI - i * M_PI / cSize;
        float nextAngle = 0.75f * M_PI - (i+1) * M_PI / cSize;
        SDL_RenderLine(renderer,
            cX + cSize/2 * cos(angle), cY + cSize/2 * sin(angle),
            cX + cSize/2 * cos(nextAngle), cY + cSize/2 * sin(nextAngle));
    }

    // Draw a large "D" at the left (West Road)
    int dX = 40;
    int dY = CENTER_Y;
    int dSize = 30;
    // Draw "D" using lines
    SDL_RenderLine(renderer, dX - dSize/2, dY - dSize/2, dX - dSize/2, dY + dSize/2); // Vertical
    SDL_RenderLine(renderer, dX - dSize/2, dY - dSize/2, dX + dSize/4, dY - dSize/2); // Top
    SDL_RenderLine(renderer, dX + dSize/4, dY - dSize/2, dX + dSize/2, dY); // Top curve
    SDL_RenderLine(renderer, dX + dSize/2, dY, dX + dSize/4, dY + dSize/2); // Bottom curve
    SDL_RenderLine(renderer, dX + dSize/4, dY + dSize/2, dX - dSize/2, dY + dSize/2); // Bottom

    // ---------- STEP 7: DRAW LANE FLOW ARROWS ----------
    // Draw arrows showing vehicle flow direction for each lane

    // --- A Lanes Flow (North) ---
    drawLaneFlowArrow(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH/2, CENTER_Y - ROAD_WIDTH, Direction::DOWN);
    drawLaneFlowArrow(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5, CENTER_Y - ROAD_WIDTH, Direction::DOWN);
    drawLaneFlowArrow(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5, CENTER_Y - ROAD_WIDTH, Direction::DOWN);

    // --- B Lanes Flow (East) ---
    drawLaneFlowArrow(CENTER_X + ROAD_WIDTH, CENTER_Y - ROAD_WIDTH/2 + LANE_WIDTH/2, Direction::LEFT);
    drawLaneFlowArrow(CENTER_X + ROAD_WIDTH, CENTER_Y - ROAD_WIDTH/2 + LANE_WIDTH*1.5, Direction::LEFT);
    drawLaneFlowArrow(CENTER_X + ROAD_WIDTH, CENTER_Y - ROAD_WIDTH/2 + LANE_WIDTH*2.5, Direction::LEFT);

    // --- C Lanes Flow (South) ---
    drawLaneFlowArrow(CENTER_X + LANE_WIDTH*1.5, CENTER_Y + ROAD_WIDTH, Direction::UP);
    drawLaneFlowArrow(CENTER_X + LANE_WIDTH*0.5, CENTER_Y + ROAD_WIDTH, Direction::UP);
    drawLaneFlowArrow(CENTER_X - LANE_WIDTH*0.5, CENTER_Y + ROAD_WIDTH, Direction::UP);

    // --- D Lanes Flow (West) ---
    drawLaneFlowArrow(CENTER_X - ROAD_WIDTH, CENTER_Y + LANE_WIDTH*1.5, Direction::RIGHT);
    drawLaneFlowArrow(CENTER_X - ROAD_WIDTH, CENTER_Y + LANE_WIDTH*0.5, Direction::RIGHT);
    drawLaneFlowArrow(CENTER_X - ROAD_WIDTH, CENTER_Y - LANE_WIDTH*0.5, Direction::RIGHT);

    // ---------- STEP 8: DRAW STOP LINES ----------
    // Draw white stop lines at the intersection
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Top stop line (A road)
    SDL_FRect topStop = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2 - 4),
        static_cast<float>(ROAD_WIDTH),
        4.0f
    };
    SDL_RenderFillRect(renderer, &topStop);

    // Bottom stop line (C road)
    SDL_FRect bottomStop = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y + ROAD_WIDTH/2),
        static_cast<float>(ROAD_WIDTH),
        4.0f
    };
    SDL_RenderFillRect(renderer, &bottomStop);

    // Left stop line (D road)
    SDL_FRect leftStop = {
        static_cast<float>(CENTER_X - ROAD_WIDTH/2 - 4),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2),
        4.0f,
        static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &leftStop);

    // Right stop line (B road)
    SDL_FRect rightStop = {
        static_cast<float>(CENTER_X + ROAD_WIDTH/2),
        static_cast<float>(CENTER_Y - ROAD_WIDTH/2),
        4.0f,
        static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &rightStop);

    // ---------- STEP 9: DRAW LEGEND ----------
    // Draw a small legend in the corner to explain colors

    int legendX = 20;
    int legendY = windowHeight - 140;
    int boxSize = 15;
    int spacing = 25;

    // Blue Box - Lane 1 (Incoming)
    SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255);
    SDL_FRect l1Box = {
        static_cast<float>(legendX),
        static_cast<float>(legendY),
        static_cast<float>(boxSize),
        static_cast<float>(boxSize)
    };
    SDL_RenderFillRect(renderer, &l1Box);

    // Draw "L1" next to box
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderLine(renderer, legendX + boxSize + 5, legendY + boxSize/2,
                  legendX + boxSize + 15, legendY + boxSize/2);

    // Orange Box - Lane A2 (Priority)
    SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);
    SDL_FRect l2Box = {
        static_cast<float>(legendX),
        static_cast<float>(legendY + spacing),
        static_cast<float>(boxSize),
        static_cast<float>(boxSize)
    };
    SDL_RenderFillRect(renderer, &l2Box);

    // Draw "A2" next to box
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderLine(renderer, legendX + boxSize + 5, legendY + spacing + boxSize/2 - 5,
                  legendX + boxSize + 5, legendY + spacing + boxSize/2 + 5);
    SDL_RenderLine(renderer, legendX + boxSize + 5, legendY + spacing + boxSize/2 - 5,
                  legendX + boxSize + 15, legendY + spacing + boxSize/2 - 5);
    SDL_RenderLine(renderer, legendX + boxSize + 15, legendY + spacing + boxSize/2 - 5,
                  legendX + boxSize + 15, legendY + spacing + boxSize/2);
    SDL_RenderLine(renderer, legendX + boxSize + 15, legendY + spacing + boxSize/2,
                  legendX + boxSize + 5, legendY + spacing + boxSize/2);
    SDL_RenderLine(renderer, legendX + boxSize + 5, legendY + spacing + boxSize/2,
                  legendX + boxSize + 15, legendY + spacing + boxSize/2 + 5);

    // Green Box - Lane 3 (Free)
    SDL_SetRenderDrawColor(renderer, 50, 205, 50, 255);
    SDL_FRect l3Box = {
        static_cast<float>(legendX),
        static_cast<float>(legendY + 2*spacing),
        static_cast<float>(boxSize),
        static_cast<float>(boxSize)
    };
    SDL_RenderFillRect(renderer, &l3Box);

    // Draw "L3" next to box
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderLine(renderer, legendX + boxSize + 5, legendY + 2*spacing + boxSize/2 - 5,
                  legendX + boxSize + 5, legendY + 2*spacing + boxSize/2 + 5);
    // Draw "3"
    SDL_RenderLine(renderer, legendX + boxSize + 10, legendY + 2*spacing + boxSize/2 - 5,
                  legendX + boxSize + 15, legendY + 2*spacing + boxSize/2 - 5);
    SDL_RenderLine(renderer, legendX + boxSize + 15, legendY + 2*spacing + boxSize/2 - 5,
                  legendX + boxSize + 15, legendY + 2*spacing + boxSize/2);
    SDL_RenderLine(renderer, legendX + boxSize + 15, legendY + 2*spacing + boxSize/2,
                  legendX + boxSize + 10, legendY + 2*spacing + boxSize/2);
    SDL_RenderLine(renderer, legendX + boxSize + 10, legendY + 2*spacing + boxSize/2,
                  legendX + boxSize + 15, legendY + 2*spacing + boxSize/2 + 5);

    // Yellow Box - Normal Lanes
    SDL_SetRenderDrawColor(renderer, 218, 165, 32, 255);
    SDL_FRect normalBox = {
        static_cast<float>(legendX),
        static_cast<float>(legendY + 3*spacing),
        static_cast<float>(boxSize),
        static_cast<float>(boxSize)
    };
    SDL_RenderFillRect(renderer, &normalBox);

    // Draw "L2" next to box
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderLine(renderer, legendX + boxSize + 5, legendY + 3*spacing + boxSize/2 - 5,
                  legendX + boxSize + 5, legendY + 3*spacing + boxSize/2 + 5);
    // Draw "2"
    SDL_RenderLine(renderer, legendX + boxSize + 10, legendY + 3*spacing + boxSize/2 - 5,
                  legendX + boxSize + 15, legendY + 3*spacing + boxSize/2 - 5);
    SDL_RenderLine(renderer, legendX + boxSize + 15, legendY + 3*spacing + boxSize/2 - 5,
                  legendX + boxSize + 15, legendY + 3*spacing + boxSize/2);
    SDL_RenderLine(renderer, legendX + boxSize + 15, legendY + 3*spacing + boxSize/2,
                  legendX + boxSize + 10, legendY + 3*spacing + boxSize/2);
    SDL_RenderLine(renderer, legendX + boxSize + 10, legendY + 3*spacing + boxSize/2,
                  legendX + boxSize + 15, legendY + 3*spacing + boxSize/2 + 5);
}


void Renderer::drawLaneFlowArrow(int x, int y, Direction dir) {
    // Draw a large flow arrow in the lane
    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 180); // Light gray, semi-transparent

    int arrowSize = 20;

    switch (dir) {
        case Direction::UP:
            // Arrow pointing up
            SDL_RenderLine(renderer, x, y - arrowSize, x - arrowSize/2, y); // Left diagonal
            SDL_RenderLine(renderer, x, y - arrowSize, x + arrowSize/2, y); // Right diagonal
            SDL_RenderLine(renderer, x, y - arrowSize, x, y + arrowSize); // Stem
            break;

        case Direction::DOWN:
            // Arrow pointing down
            SDL_RenderLine(renderer, x, y + arrowSize, x - arrowSize/2, y); // Left diagonal
            SDL_RenderLine(renderer, x, y + arrowSize, x + arrowSize/2, y); // Right diagonal
            SDL_RenderLine(renderer, x, y - arrowSize, x, y + arrowSize); // Stem
            break;

        case Direction::LEFT:
            // Arrow pointing left
            SDL_RenderLine(renderer, x - arrowSize, y, x, y - arrowSize/2); // Top diagonal
            SDL_RenderLine(renderer, x - arrowSize, y, x, y + arrowSize/2); // Bottom diagonal
            SDL_RenderLine(renderer, x - arrowSize, y, x + arrowSize, y); // Stem
            break;

        case Direction::RIGHT:
            // Arrow pointing right
            SDL_RenderLine(renderer, x + arrowSize, y, x, y - arrowSize/2); // Top diagonal
            SDL_RenderLine(renderer, x + arrowSize, y, x, y + arrowSize/2); // Bottom diagonal
            SDL_RenderLine(renderer, x - arrowSize, y, x + arrowSize, y); // Stem
            break;
    }
}



// Helper method to draw direction arrows
// FILE: src/visualization/Renderer.cpp
// Implementation of drawDirectionArrow method

// FILE: src/visualization/Renderer.cpp
// Corrected implementation of drawDirectionArrow method

void Renderer::drawDirectionArrow(int x, int y, Direction dir, SDL_Color color) {
    SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

    const int arrowSize = 12;

    SDL_FPoint points[3];

    switch (dir) {
        case Direction::UP:
            points[0] = {static_cast<float>(x), static_cast<float>(y - arrowSize/2)};
            points[1] = {static_cast<float>(x - arrowSize/2), static_cast<float>(y + arrowSize/2)};
            points[2] = {static_cast<float>(x + arrowSize/2), static_cast<float>(y + arrowSize/2)};
            break;

        case Direction::DOWN:
            points[0] = {static_cast<float>(x), static_cast<float>(y + arrowSize/2)};
            points[1] = {static_cast<float>(x - arrowSize/2), static_cast<float>(y - arrowSize/2)};
            points[2] = {static_cast<float>(x + arrowSize/2), static_cast<float>(y - arrowSize/2)};
            break;

        case Direction::LEFT:
            points[0] = {static_cast<float>(x - arrowSize/2), static_cast<float>(y)};
            points[1] = {static_cast<float>(x + arrowSize/2), static_cast<float>(y - arrowSize/2)};
            points[2] = {static_cast<float>(x + arrowSize/2), static_cast<float>(y + arrowSize/2)};
            break;

        case Direction::RIGHT:
            points[0] = {static_cast<float>(x + arrowSize/2), static_cast<float>(y)};
            points[1] = {static_cast<float>(x - arrowSize/2), static_cast<float>(y - arrowSize/2)};
            points[2] = {static_cast<float>(x - arrowSize/2), static_cast<float>(y + arrowSize/2)};
            break;
    }

    // Draw outline
    SDL_RenderLine(this->renderer, points[0].x, points[0].y, points[1].x, points[1].y);
    SDL_RenderLine(this->renderer, points[1].x, points[1].y, points[2].x, points[2].y);
    SDL_RenderLine(this->renderer, points[2].x, points[2].y, points[0].x, points[0].y);

    // Create SDL vertices for filled triangle
    SDL_Vertex vertices[3];
    SDL_FColor fcolor = {
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
        static_cast<float>(color.a) / 255.0f
    };

    // Set vertices
    for (int i = 0; i < 3; i++) {
        vertices[i].position = points[i];
        vertices[i].color = fcolor;
    }

    // Draw filled triangle
    SDL_RenderGeometry(this->renderer, NULL, vertices, 3, NULL, 0);
}

void Renderer::drawLaneLabels() {
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;
    const int LANE_WIDTH = Constants::LANE_WIDTH;
    const int CENTER_X = windowWidth / 2;
    const int CENTER_Y = windowHeight / 2;

    // Draw road identifiers with large symbols
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Road A (North) Identifier
    {
        int x = CENTER_X;
        int y = 30;
        int size = 25;

        // Draw "A" using lines
        SDL_RenderLine(renderer, x - size/2, y + size/2, x, y - size/2); // Left diagonal
        SDL_RenderLine(renderer, x, y - size/2, x + size/2, y + size/2); // Right diagonal
        SDL_RenderLine(renderer, x - size/4, y, x + size/4, y); // Middle bar

        // Draw "NORTH" indicator (arrow pointing up)
        SDL_RenderLine(renderer, x, y + size + 5, x, y + size + 20); // Stem
        SDL_RenderLine(renderer, x, y + size + 5, x - 5, y + size + 10); // Left arrow
        SDL_RenderLine(renderer, x, y + size + 5, x + 5, y + size + 10); // Right arrow
    }

    // Road B (East) Identifier
    {
        int x = windowWidth - 30;
        int y = CENTER_Y;
        int size = 25;

        // Draw "B" using lines
        SDL_RenderLine(renderer, x - size/2, y - size/2, x - size/2, y + size/2); // Vertical
        SDL_RenderLine(renderer, x - size/2, y - size/2, x + size/3, y - size/2); // Top
        SDL_RenderLine(renderer, x + size/3, y - size/2, x + size/2, y - size/4); // Top curve
        SDL_RenderLine(renderer, x + size/2, y - size/4, x + size/3, y); // To middle
        SDL_RenderLine(renderer, x - size/2, y, x + size/3, y); // Middle
        SDL_RenderLine(renderer, x + size/3, y, x + size/2, y + size/4); // From middle
        SDL_RenderLine(renderer, x + size/2, y + size/4, x + size/3, y + size/2); // Bottom curve
        SDL_RenderLine(renderer, x + size/3, y + size/2, x - size/2, y + size/2); // Bottom

        // Draw "EAST" indicator (arrow pointing right)
        SDL_RenderLine(renderer, x - size - 5, y, x - size - 20, y); // Stem
        SDL_RenderLine(renderer, x - size - 5, y, x - size - 10, y - 5); // Top arrow
        SDL_RenderLine(renderer, x - size - 5, y, x - size - 10, y + 5); // Bottom arrow
    }

    // Road C (South) Identifier
    {
        int x = CENTER_X;
        int y = windowHeight - 30;
        int size = 25;

        // Draw "C" using lines
        SDL_RenderLine(renderer, x + size/2, y - size/2, x - size/2, y - size/2); // Top
        SDL_RenderLine(renderer, x - size/2, y - size/2, x - size/2, y + size/2); // Left
        SDL_RenderLine(renderer, x - size/2, y + size/2, x + size/2, y + size/2); // Bottom

        // Draw "SOUTH" indicator (arrow pointing down)
        SDL_RenderLine(renderer, x, y - size - 5, x, y - size - 20); // Stem
        SDL_RenderLine(renderer, x, y - size - 5, x - 5, y - size - 10); // Left arrow
        SDL_RenderLine(renderer, x, y - size - 5, x + 5, y - size - 10); // Right arrow
    }

    // Road D (West) Identifier
    {
        int x = 30;
        int y = CENTER_Y;
        int size = 25;

        // Draw "D" using lines
        SDL_RenderLine(renderer, x - size/2, y - size/2, x - size/2, y + size/2); // Vertical
        SDL_RenderLine(renderer, x - size/2, y - size/2, x + size/4, y - size/2); // Top
        SDL_RenderLine(renderer, x + size/4, y - size/2, x + size/2, y); // Top curve
        SDL_RenderLine(renderer, x + size/2, y, x + size/4, y + size/2); // Bottom curve
        SDL_RenderLine(renderer, x + size/4, y + size/2, x - size/2, y + size/2); // Bottom

        // Draw "WEST" indicator (arrow pointing left)
        SDL_RenderLine(renderer, x + size + 5, y, x + size + 20, y); // Stem
        SDL_RenderLine(renderer, x + size + 5, y, x + size + 10, y - 5); // Top arrow
        SDL_RenderLine(renderer, x + size + 5, y, x + size + 10, y + 5); // Bottom arrow
    }

    // Draw lane identifiers with distinctive markers

    // A Lanes (North)
    {
        // A1 (Incoming) - Blue marker
        SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255); // Dodger Blue
        SDL_FRect a1Box = {
            static_cast<float>(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*0.5f - 15.0f),
            static_cast<float>(CENTER_Y - ROAD_WIDTH/2 - 30.0f),
            30.0f, 20.0f
        };
        SDL_RenderFillRect(renderer, &a1Box);

        // Draw "A1" inside
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // A
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*0.5f - 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*0.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*0.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*0.5f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f);
        // 1
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*0.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*0.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f);

        // A2 (Priority) - Orange marker
        SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255); // Orange
        SDL_FRect a2Box = {
            static_cast<float>(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f - 15.0f),
            static_cast<float>(CENTER_Y - ROAD_WIDTH/2 - 30.0f),
            30.0f, 20.0f
        };
        SDL_RenderFillRect(renderer, &a2Box);

        // Draw "A2" inside
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // A
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f - 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f);
        // 2
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 20.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 20.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 20.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 20.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f);

        // Draw "P" for priority (above the marker)
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 35.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 45.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 45.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 45.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 45.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 40.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 40.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*1.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 40.0f);

        // A3 (Free) - Green marker
        SDL_SetRenderDrawColor(renderer, 50, 205, 50, 255); // Lime Green
        SDL_FRect a3Box = {
            static_cast<float>(CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f - 15.0f),
            static_cast<float>(CENTER_Y - ROAD_WIDTH/2 - 30.0f),
            30.0f, 20.0f
        };
        SDL_RenderFillRect(renderer, &a3Box);

        // Draw "A3" inside
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // A
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f - 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f);
        // 3
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 25.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 20.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 20.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 20.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 20.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 10.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 15.0f);

        // Draw "F" for free (above the marker)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 35.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 45.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 45.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 45.0f);
        SDL_RenderLine(renderer, CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f - 5.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 40.0f,
                      CENTER_X - ROAD_WIDTH/2 + LANE_WIDTH*2.5f + 2.0f,
                      CENTER_Y - ROAD_WIDTH/2 - 40.0f);
    }

    // Similar implementations for B, C, and D lanes...
    // (abbreviated for brevity)
}

void Renderer::drawTrafficLights() {
    if (!trafficManager) {
        return;
    }

    TrafficLight* trafficLight = trafficManager->getTrafficLight();
    if (!trafficLight) {
        return;
    }

    // Draw traffic lights
    trafficLight->render(renderer);
}

void Renderer::drawVehicles() {
    if (!trafficManager) {
        return;
    }

    // Get all lanes from traffic manager
    const std::vector<Lane*>& lanes = trafficManager->getLanes();

    // Draw vehicles in each lane
    for (Lane* lane : lanes) {
        if (!lane) {
            continue;
        }

        const std::vector<Vehicle*>& vehicles = lane->getVehicles();
        int queuePos = 0;

        for (Vehicle* vehicle : vehicles) {
            if (vehicle) {
                vehicle->render(renderer, carTexture, queuePos);
                queuePos++;
            }
        }
    }
}

void Renderer::drawDebugOverlay() {
    // Draw semi-transparent background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200); // More opaque background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect overlayRect = {10, 10, 280, 180}; // Larger overlay
    SDL_RenderFillRect(renderer, &overlayRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Add border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &overlayRect);

    // Draw statistics
    drawStatistics();

    // Draw title
    drawText("Traffic Junction Simulator", 20, 20, {255, 255, 255, 255});
    drawText("Press D to toggle debug overlay", 20, 40, {200, 200, 200, 255});

    // Draw recent logs
    std::vector<std::string> logs = DebugLogger::getRecentLogs(5);
    int y = 170;

    for (const auto& log : logs) {
        std::string truncatedLog = log.length() > 50 ? log.substr(0, 47) + "..." : log;
        drawText(truncatedLog, 10, y, {200, 200, 200, 255});
        y += 20;
    }
}

void Renderer::drawStatistics() {
    if (!trafficManager) {
        return;
    }

    // Get statistics from traffic manager
    std::string stats = trafficManager->getStatistics();

    // Split into lines
    std::istringstream stream(stats);
    std::string line;
    int y = 60;

    while (std::getline(stream, line)) {
        // Check if line contains priority info
        if (line.find("PRIORITY") != std::string::npos) {
            drawText(line, 20, y, {255, 140, 0, 255}); // Highlight priority lanes
        } else if (line.find("A2") != std::string::npos) {
            drawText(line, 20, y, {255, 200, 0, 255}); // Highlight A2 lane
        } else {
            drawText(line, 20, y, {255, 255, 255, 255});
        }
        y += 20;
    }

    // Show current traffic light state
    SDL_Color stateColor = {255, 255, 255, 255};
    std::string stateText = "Traffic Light: ";

    auto* trafficLight = trafficManager->getTrafficLight();
    if (trafficLight) {
        auto currentState = trafficLight->getCurrentState();
        switch (currentState) {
            case TrafficLight::State::ALL_RED:
                stateText += "All Red";
                stateColor = {255, 100, 100, 255};
                break;
            case TrafficLight::State::A_GREEN:
                stateText += "A Green (North)";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::B_GREEN:
                stateText += "B Green (East)";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::C_GREEN:
                stateText += "C Green (South)";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::D_GREEN:
                stateText += "D Green (West)";
                stateColor = {100, 255, 100, 255};
                break;
        }
    }

    drawText(stateText, 20, y, stateColor);
}

void Renderer::drawText(const std::string& text, int x, int y, SDL_Color color) {
    // Since we don't have SDL_ttf configured, draw a colored rectangle
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect textRect = {static_cast<float>(x), static_cast<float>(y),
                         static_cast<float>(8 * text.length()), 15};

    // Draw colored rectangle representing text
    SDL_RenderFillRect(renderer, &textRect);

    // Draw text outline
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &textRect);
}

void Renderer::drawArrow(int x1, int y1, int x2, int y2, int x3, int y3, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Draw triangle outline
    SDL_RenderLine(renderer, x1, y1, x2, y2);
    SDL_RenderLine(renderer, x2, y2, x3, y3);
    SDL_RenderLine(renderer, x3, y3, x1, y1);

    // Create vertices for filled triangle with SDL_FColor for SDL3 compatibility
    SDL_Vertex vertices[3];

    // Convert SDL_Color to SDL_FColor for vertices
    SDL_FColor fcolor = {
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
        static_cast<float>(color.a) / 255.0f
    };

    // First vertex
    vertices[0].position.x = x1;
    vertices[0].position.y = y1;
    vertices[0].color = fcolor;

    // Second vertex
    vertices[1].position.x = x2;
    vertices[1].position.y = y2;
    vertices[1].color = fcolor;

    // Third vertex
    vertices[2].position.x = x3;
    vertices[2].position.y = y3;
    vertices[2].color = fcolor;

    // Draw the filled triangle
    SDL_RenderGeometry(renderer, NULL, vertices, 3, NULL, 0);
}

void Renderer::cleanup() {
    if (carTexture) {
        SDL_DestroyTexture(carTexture);
        carTexture = nullptr;
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    DebugLogger::log("Renderer resources cleaned up");
}

bool Renderer::isActive() const {
    return active;
}

void Renderer::toggleDebugOverlay() {
    showDebugOverlay = !showDebugOverlay;
    DebugLogger::log("Debug overlay " + std::string(showDebugOverlay ? "enabled" : "disabled"));
}

void Renderer::setFrameRateLimit(int fps) {
    frameRateLimit = fps;
}

void Renderer::setTrafficManager(TrafficManager* manager) {
    trafficManager = manager;
}