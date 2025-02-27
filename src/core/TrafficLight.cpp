// FILE: src/core/TrafficLight.cpp
#include "core/TrafficLight.h"
#include "utils/DebugLogger.h"
#include <sstream>
#include <cmath>
#include <SDL3/SDL.h>
#include "core/Constants.h"

TrafficLight::TrafficLight()
    : currentState(State::ALL_RED),
      nextState(State::A_GREEN),
      lastStateChangeTime(SDL_GetTicks()),
      isPriorityMode(false),
      shouldResumeNormalMode(false),
      forceAGreen(false),
      priorityModeStartTime(0) {

    DebugLogger::log("TrafficLight initialized");
}

TrafficLight::~TrafficLight() {
    DebugLogger::log("TrafficLight destroyed");
}

void TrafficLight::update(const std::vector<Lane*>& lanes) {
    uint32_t currentTime = SDL_GetTicks();
    uint32_t elapsedTime = currentTime - lastStateChangeTime;

    // CRITICAL: Find priority lane A2 directly
    Lane* al2Lane = nullptr;
    for (auto* lane : lanes) {
        if (lane->getLaneId() == 'A' && lane->getLaneNumber() == 2) {
            al2Lane = lane;
            break;
        }
    }

    // CRITICAL FIX: Direct priority detection and override
    if (al2Lane) {
        int vehicleCount = al2Lane->getVehicleCount();

        // CRITICAL: Force immediate transition to A_GREEN when vehicle count exceeds threshold
        if (vehicleCount > Constants::PRIORITY_THRESHOLD_HIGH) {
            if (!isPriorityMode) {
                isPriorityMode = true;
                forceAGreen = true;
                priorityModeStartTime = currentTime;

                std::ostringstream oss;
                oss << "!!! PRIORITY MODE ACTIVATED: A2 has " << vehicleCount << " vehicles (>10)";
                DebugLogger::log(oss.str(), DebugLogger::LogLevel::ERROR);

                // Force immediate transition to A_GREEN
                if (currentState != State::A_GREEN) {
                    // If not in ALL_RED, go to ALL_RED first
                    if (currentState != State::ALL_RED) {
                        currentState = State::ALL_RED;
                        nextState = State::A_GREEN;
                        lastStateChangeTime = currentTime;
                    } else {
                        // If already in ALL_RED, go directly to A_GREEN
                        currentState = State::A_GREEN;
                        nextState = State::ALL_RED;
                        lastStateChangeTime = currentTime;
                    }
                }
            }
        }
        // Exit priority mode when count drops below threshold
        else if (isPriorityMode && vehicleCount < Constants::PRIORITY_THRESHOLD_LOW) {
            isPriorityMode = false;
            forceAGreen = false;

            std::ostringstream oss;
            oss << "!!! PRIORITY MODE DEACTIVATED: A2 now has " << vehicleCount << " vehicles (<5)";
            DebugLogger::log(oss.str(), DebugLogger::LogLevel::ERROR);
        }

        // CRITICAL: Force-log the priority state for debugging
        if (isPriorityMode && currentTime - priorityModeStartTime > 5000) {
            std::ostringstream oss;
            oss << "Priority mode active: A2 has " << vehicleCount << " vehicles, light state: "
                << (currentState == State::A_GREEN ? "A_GREEN" :
                   (currentState == State::ALL_RED ? "ALL_RED" : "OTHER"));
            DebugLogger::log(oss.str(), DebugLogger::LogLevel::ERROR);
            priorityModeStartTime = currentTime;
        }
    }

    // CRITICAL FIX: If in priority mode, force the light to stay on A_GREEN longer
    if (isPriorityMode && forceAGreen) {
        if (currentState != State::A_GREEN) {
            if (elapsedTime >= allRedDuration) {
                // After ALL_RED, go back to A_GREEN in priority mode
                currentState = State::A_GREEN;
                nextState = State::ALL_RED;
                lastStateChangeTime = currentTime;

                DebugLogger::log("PRIORITY MODE: Forcing A_GREEN", DebugLogger::LogLevel::ERROR);
            }
        } else {
            // Extend the green duration in priority mode
            int priorityGreenDuration = 6000; // 6 seconds of green in priority mode

            if (elapsedTime >= priorityGreenDuration) {
                // Go to ALL_RED briefly before returning to A_GREEN
                currentState = State::ALL_RED;
                nextState = State::A_GREEN;
                lastStateChangeTime = currentTime;

                DebugLogger::log("PRIORITY MODE: Brief ALL_RED before returning to A_GREEN");
            }
        }

        return; // Skip normal light cycling in priority mode
    }

    // NORMAL MODE - Calculate appropriate duration
    int stateDuration;
    if (currentState == State::ALL_RED) {
        stateDuration = allRedDuration; // 2 seconds for ALL_RED
    } else {
        // Calculate average using lane counts
        float averageVehicleCount = calculateAverageVehicleCount(lanes);

        // Set duration using formula: Total time = |V| * t (2 seconds per vehicle)
        stateDuration = static_cast<int>(averageVehicleCount * 2000);

        // Apply minimum and maximum limits for reasonable times
        if (stateDuration < 3000) stateDuration = 3000; // Min 3 seconds
        if (stateDuration > 15000) stateDuration = 15000; // Max 15 seconds

        // Log the calculation
        std::ostringstream oss;
        oss << "Traffic light timing: |V| = " << averageVehicleCount
            << ", Duration = " << stateDuration / 1000.0f << " seconds";
        DebugLogger::log(oss.str());
    }

    // State transition in normal mode
    if (elapsedTime >= stateDuration) {
        // Change to next state
        currentState = nextState;

        // Log the state change clearly
        std::string stateStr;
        switch (currentState) {
            case State::ALL_RED: stateStr = "ALL_RED"; break;
            case State::A_GREEN: stateStr = "A_GREEN"; break;
            case State::B_GREEN: stateStr = "B_GREEN"; break;
            case State::C_GREEN: stateStr = "C_GREEN"; break;
            case State::D_GREEN: stateStr = "D_GREEN"; break;
        }

        DebugLogger::log("Traffic light changed to: " + stateStr);

        // Normal rotation pattern: ALL_RED → A → ALL_RED → B → ALL_RED → C → ALL_RED → D → ...
        if (currentState == State::ALL_RED) {
            // Cycle through green states
            switch (nextState) {
                case State::A_GREEN: nextState = State::B_GREEN; break;
                case State::B_GREEN: nextState = State::C_GREEN; break;
                case State::C_GREEN: nextState = State::D_GREEN; break;
                case State::D_GREEN: nextState = State::A_GREEN; break;
                default: nextState = State::A_GREEN; break;
            }
        } else {
            // Always go to ALL_RED after any green state
            nextState = State::ALL_RED;
        }

        lastStateChangeTime = currentTime;
    }
}

float TrafficLight::calculateAverageVehicleCount(const std::vector<Lane*>& lanes) {
    int normalLaneCount = 0;
    int totalVehicleCount = 0;

    for (auto* lane : lanes) {
        // Only count lane 2 (normal lanes)
        // In priority mode, exclude the priority lane (A2) from calculation
        if (lane->getLaneNumber() == 2 &&
            !(isPriorityMode && lane->getLaneId() == 'A')) {
            normalLaneCount++;
            totalVehicleCount += lane->getVehicleCount();
        }
    }

    // Calculate average: |V| = (1/n) * Σ|Li|
    float average = (normalLaneCount > 0) ?
        static_cast<float>(totalVehicleCount) / normalLaneCount : 0.0f;

    // Return at least 1 to ensure some duration
    return std::max(1.0f, average);
}

void TrafficLight::setNextState(State state) {
    nextState = state;
}

bool TrafficLight::isGreen(char lane) const {
    // CRITICAL: Explicitly check the state machine state
    switch (currentState) {
        case State::A_GREEN: return lane == 'A';
        case State::B_GREEN: return lane == 'B';
        case State::C_GREEN: return lane == 'C';
        case State::D_GREEN: return lane == 'D';
        case State::ALL_RED: return false;
        default: return false;
    }
}

void TrafficLight::render(SDL_Renderer* renderer) {
    // Render modern holographic-style traffic light control system

    // Constants for positioning
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 800;
    const int CENTER_X = WINDOW_WIDTH / 2;
    const int CENTER_Y = WINDOW_HEIGHT / 2;

    // Draw main traffic control center in top-right corner
    drawTrafficControlCenter(renderer);

    // Draw junction lights (one at each road)
    drawJunctionLight(renderer, CENTER_X, CENTER_Y - 100, 'A', isGreen('A')); // North (A) light
    drawJunctionLight(renderer, CENTER_X + 100, CENTER_Y, 'B', isGreen('B')); // East (B) light
    drawJunctionLight(renderer, CENTER_X, CENTER_Y + 100, 'C', isGreen('C')); // South (C) light
    drawJunctionLight(renderer, CENTER_X - 100, CENTER_Y, 'D', isGreen('D')); // West (D) light

    // Draw the state transition timer
    drawStateTimer(renderer);
}

void TrafficLight::drawTrafficControlCenter(SDL_Renderer* renderer) {
    // Draw holographic traffic management system display in top-right corner
    const int PANEL_WIDTH = 160;
    const int PANEL_HEIGHT = 160;
    const int PANEL_X = 800 - PANEL_WIDTH - 20;
    const int PANEL_Y = 20;

    // Draw glass-style panel with dark blue semi-transparent background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Panel main background
    SDL_SetRenderDrawColor(renderer, 10, 20, 40, 180);
    SDL_FRect panel = {
        static_cast<float>(PANEL_X),
        static_cast<float>(PANEL_Y),
        static_cast<float>(PANEL_WIDTH),
        static_cast<float>(PANEL_HEIGHT)
    };
    SDL_RenderFillRect(renderer, &panel);

    // Panel outer glow effect
    for (int i = 1; i <= 3; i++) {
        SDL_SetRenderDrawColor(renderer, 100, 140, 200, 40 / i);
        SDL_FRect glow = {
            panel.x - i, panel.y - i,
            panel.w + i*2, panel.h + i*2
        };
        SDL_RenderRect(renderer, &glow);
    }

    // Panel border
    SDL_SetRenderDrawColor(renderer, 100, 140, 200, 255);
    SDL_RenderRect(renderer, &panel);

    // Panel top header
    SDL_SetRenderDrawColor(renderer, 40, 60, 100, 200);
    SDL_FRect header = {
        panel.x, panel.y,
        panel.w, 25.0f
    };
    SDL_RenderFillRect(renderer, &header);

    // Draw "TRAFFIC CONTROL" title
    SDL_SetRenderDrawColor(renderer, 220, 230, 255, 255);
    drawPanelText(renderer, "TRAFFIC CONTROL", PANEL_X + PANEL_WIDTH/2 - 55, PANEL_Y + 8);

    // Inner panel for light status
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 200);
    SDL_FRect innerPanel = {
        panel.x + 8, panel.y + 35,
        panel.w - 16, panel.h - 45
    };
    SDL_RenderFillRect(renderer, &innerPanel);

    // Inner panel border
    SDL_SetRenderDrawColor(renderer, 80, 100, 160, 150);
    SDL_RenderRect(renderer, &innerPanel);

    // Draw road light indicators
    const int LIGHT_SIZE = 18;
    const int LIGHT_SPACING = 30;
    const int START_Y = PANEL_Y + 45;

    // Road labels
    const char* roads[] = {"ROAD A", "ROAD B", "ROAD C", "ROAD D"};

    for (int i = 0; i < 4; i++) {
        int y = START_Y + i * LIGHT_SPACING;

        // Draw road label
        SDL_SetRenderDrawColor(renderer, 180, 200, 255, 255);
        drawPanelText(renderer, roads[i], PANEL_X + 15, y);

        // Determine light color based on state
        bool isRoad = false;
        switch (i) {
            case 0: isRoad = isGreen('A'); break;
            case 1: isRoad = isGreen('B'); break;
            case 2: isRoad = isGreen('C'); break;
            case 3: isRoad = isGreen('D'); break;
        }

        // Draw holographic light indicator
        drawHolographicLight(renderer, PANEL_X + PANEL_WIDTH - 30, y, LIGHT_SIZE, isRoad);
    }

    // Draw priority mode indicator if active
    if (isPriorityMode) {
        // Flashing priority alert
        uint32_t time = SDL_GetTicks();
        bool flash = (time / 500) % 2 == 0;

        SDL_SetRenderDrawColor(renderer, flash ? 255 : 200, flash ? 140 : 100, 0, 200);
        SDL_FRect priorityBox = {
            panel.x + 10, panel.y + panel.h - 30,
            panel.w - 20, 20.0f
        };
        SDL_RenderFillRect(renderer, &priorityBox);

        // Priority text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        drawPanelText(renderer, "PRIORITY MODE: A2", PANEL_X + 20, PANEL_Y + PANEL_HEIGHT - 25);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void TrafficLight::drawJunctionLight(SDL_Renderer* renderer, int x, int y, char roadId, bool isGreen) {
    // Draw futuristic traffic light on the roads
    const int LIGHT_WIDTH = 30;
    const int LIGHT_HEIGHT = 50;

    // Adjust position based on road
    float posX = x;
    float posY = y;

    // Adjust for road position
    switch (roadId) {
        case 'A': // North
            posX -= LIGHT_WIDTH / 2;
            posY -= LIGHT_HEIGHT + 20;
            break;
        case 'B': // East
            posX += 20;
            posY -= LIGHT_HEIGHT / 2;
            break;
        case 'C': // South
            posX -= LIGHT_WIDTH / 2;
            posY += 20;
            break;
        case 'D': // West
            posX -= LIGHT_WIDTH + 20;
            posY -= LIGHT_HEIGHT / 2;
            break;
    }

    // Draw light housing
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Light housing (dark)
    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 220);
    SDL_FRect housing = {
        posX, posY,
        static_cast<float>(LIGHT_WIDTH),
        static_cast<float>(LIGHT_HEIGHT)
    };
    SDL_RenderFillRect(renderer, &housing);

    // Housing border
    SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
    SDL_RenderRect(renderer, &housing);

    // Light lens
    float lensSize = 20.0f;
    float lensX = posX + (LIGHT_WIDTH - lensSize) / 2;
    float lensY = posY + (LIGHT_HEIGHT - lensSize) / 2;

    // Draw lens background
    SDL_SetRenderDrawColor(renderer, 30, 30, 35, 255);
    SDL_FRect lens = {
        lensX, lensY,
        lensSize, lensSize
    };
    SDL_RenderFillRect(renderer, &lens);

    // Draw active light (red or green)
    if (isGreen) {
        // Green with glow effect
        // Inner bright glow
        SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
        SDL_FRect greenLight = {
            lensX + 2, lensY + 2,
            lensSize - 4, lensSize - 4
        };
        SDL_RenderFillRect(renderer, &greenLight);

        // Add inner highlight
        SDL_SetRenderDrawColor(renderer, 180, 255, 180, 200);
        SDL_FRect greenHighlight = {
            lensX + 4, lensY + 4,
            lensSize/2, lensSize/2
        };
        SDL_RenderFillRect(renderer, &greenHighlight);

        // Outer glow
        for (int i = 1; i <= 5; i++) {
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, 200 / i);
            SDL_FRect greenGlow = {
                lensX - i, lensY - i,
                lensSize + i*2, lensSize + i*2
            };
            SDL_RenderRect(renderer, &greenGlow);
        }
    } else {
        // Red with glow effect
        // Inner bright glow
        SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
        SDL_FRect redLight = {
            lensX + 2, lensY + 2,
            lensSize - 4, lensSize - 4
        };
        SDL_RenderFillRect(renderer, &redLight);

        // Add inner highlight
        SDL_SetRenderDrawColor(renderer, 255, 150, 150, 200);
        SDL_FRect redHighlight = {
            lensX + 4, lensY + 4,
            lensSize/2, lensSize/2
        };
        SDL_RenderFillRect(renderer, &redHighlight);

        // Outer glow
        for (int i = 1; i <= 5; i++) {
            SDL_SetRenderDrawColor(renderer, 255, 50, 50, 200 / i);
            SDL_FRect redGlow = {
                lensX - i, lensY - i,
                lensSize + i*2, lensSize + i*2
            };
            SDL_RenderRect(renderer, &redGlow);
        }
    }

    // Draw lens border
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
    SDL_RenderRect(renderer, &lens);

    // Draw road identifier
    SDL_SetRenderDrawColor(renderer, 200, 220, 255, 255);
    char roadChar[2] = {roadId, '\0'};

    // Position depends on road
    float textX, textY;
    switch (roadId) {
        case 'A': // North
            textX = posX + LIGHT_WIDTH / 2 - 4;
            textY = posY + 5;
            break;
        case 'B': // East
            textX = posX + 5;
            textY = posY + 5;
            break;
        case 'C': // South
            textX = posX + LIGHT_WIDTH / 2 - 4;
            textY = posY + 5;
            break;
        case 'D': // West
            textX = posX + LIGHT_WIDTH - 10;
            textY = posY + 5;
            break;
    }

    drawPanelText(renderer, roadChar, textX, textY);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void TrafficLight::drawStateTimer(SDL_Renderer* renderer) {
    // Draw a timer showing state progression with animation
    uint32_t currentTime = SDL_GetTicks();
    uint32_t elapsedTime = currentTime - lastStateChangeTime;

    // Calculate state duration
    int stateDuration;
    if (currentState == State::ALL_RED) {
        stateDuration = allRedDuration;
    } else if (isPriorityMode && currentState == State::A_GREEN) {
        stateDuration = 6000; // 6 seconds in priority mode
    } else {
        float avgVehicleCount = 5.0f; // Default fallback
        stateDuration = static_cast<int>(avgVehicleCount * 2000);
        stateDuration = std::max(3000, std::min(stateDuration, 15000));
    }

    // Calculate progress (0.0 to 1.0)
    float progress = static_cast<float>(elapsedTime) / static_cast<float>(stateDuration);
    progress = std::min(1.0f, progress);

    // Draw progress arc in the control panel
    const int CENTER_X = 800 - 100;
    const int CENTER_Y = 100;
    const int RADIUS = 30;

    // Draw background circle
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 30, 40, 60, 200);

    for (int i = 0; i < 360; i += 5) {
        float radian = i * M_PI / 180.0f;
        SDL_RenderLine(renderer,
                      CENTER_X, CENTER_Y,
                      CENTER_X + RADIUS * cosf(radian),
                      CENTER_Y + RADIUS * sinf(radian));
    }

    // Draw progress arc
    int progressDegrees = static_cast<int>(progress * 360);

    // Choose color based on state
    SDL_Color arcColor;
    switch (currentState) {
        case State::ALL_RED:
            arcColor = {255, 100, 100, 255}; // Red
            break;
        case State::A_GREEN:
            arcColor = isPriorityMode ? SDL_Color{255, 180, 0, 255} : SDL_Color{100, 255, 100, 255};
            break;
        default:
            arcColor = {100, 255, 100, 255}; // Green
            break;
    }

    // Draw the arc segments
    SDL_SetRenderDrawColor(renderer, arcColor.r, arcColor.g, arcColor.b, arcColor.a);

    for (int i = 0; i < progressDegrees; i += 2) {
        float radian = i * M_PI / 180.0f;
        SDL_RenderLine(renderer,
                      CENTER_X, CENTER_Y,
                      CENTER_X + RADIUS * cosf(radian),
                      CENTER_Y + RADIUS * sinf(radian));
    }

    // Draw a clock hand for visual effect
    float handRadian = progressDegrees * M_PI / 180.0f;
    SDL_RenderLine(renderer,
                  CENTER_X, CENTER_Y,
                  CENTER_X + (RADIUS-5) * cosf(handRadian),
                  CENTER_Y + (RADIUS-5) * sinf(handRadian));

    // Draw time remaining text
    int secondsRemaining = (stateDuration - elapsedTime) / 1000 + 1;
    std::string timeStr = std::to_string(secondsRemaining) + "s";

    SDL_SetRenderDrawColor(renderer, 220, 230, 255, 255);
    drawPanelText(renderer, timeStr.c_str(), CENTER_X - 8, CENTER_Y - 5);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void TrafficLight::drawHolographicLight(SDL_Renderer* renderer, int x, int y, int size, bool isActive) {
    // Draw a holographic light indicator
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Flickering effect for hologram
    uint32_t time = SDL_GetTicks();
    float flicker = 0.8f + 0.2f * sin(time * 0.01f);

    // Background
    SDL_SetRenderDrawColor(renderer, 30, 40, 60, 200);
    SDL_FRect lightBg = {
        static_cast<float>(x - size/2),
        static_cast<float>(y - size/2),
        static_cast<float>(size),
        static_cast<float>(size)
    };
    SDL_RenderFillRect(renderer, &lightBg);

    // Light state
    if (isActive) {
        // Active green light
        SDL_SetRenderDrawColor(renderer,
                               static_cast<Uint8>(100 * flicker),
                               static_cast<Uint8>(255 * flicker),
                               static_cast<Uint8>(100 * flicker),
                               200);

        // Inner light
        SDL_FRect innerLight = {
            lightBg.x + 2, lightBg.y + 2,
            lightBg.w - 4, lightBg.h - 4
        };
        SDL_RenderFillRect(renderer, &innerLight);

        // Glow
        for (int i = 1; i <= 3; i++) {
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, static_cast<Uint8>(100/i * flicker));
            SDL_FRect glow = {
                lightBg.x - i, lightBg.y - i,
                lightBg.w + i*2, lightBg.h + i*2
            };
            SDL_RenderRect(renderer, &glow);
        }
    } else {
        // Inactive red light
        SDL_SetRenderDrawColor(renderer,
                               static_cast<Uint8>(255 * flicker),
                               static_cast<Uint8>(80 * flicker),
                               static_cast<Uint8>(80 * flicker),
                               200);

        // Inner light
        SDL_FRect innerLight = {
            lightBg.x + 2, lightBg.y + 2,
            lightBg.w - 4, lightBg.h - 4
        };
        SDL_RenderFillRect(renderer, &innerLight);

        // Glow
        for (int i = 1; i <= 3; i++) {
            SDL_SetRenderDrawColor(renderer, 255, 80, 80, static_cast<Uint8>(100/i * flicker));
            SDL_FRect glow = {
                lightBg.x - i, lightBg.y - i,
                lightBg.w + i*2, lightBg.h + i*2
            };
            SDL_RenderRect(renderer, &glow);
        }
    }

    // Border
    SDL_SetRenderDrawColor(renderer, 100, 140, 200, 255);
    SDL_RenderRect(renderer, &lightBg);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void TrafficLight::drawPanelText(SDL_Renderer* renderer, const char* text, int x, int y) {
    // Simplified text drawing for the panel
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Draw characters
    for (int i = 0; text[i] != '\0'; i++) {
        drawPanelChar(renderer, text[i], x + i*8, y);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void TrafficLight::drawPanelChar(SDL_Renderer* renderer, char c, int x, int y) {
    // Simplified monospaced character drawing
    switch (c) {
        case 'A':
            SDL_RenderLine(renderer, x+3, y, x, y+9);      // Left diagonal
            SDL_RenderLine(renderer, x+3, y, x+6, y+9);    // Right diagonal
            SDL_RenderLine(renderer, x+1, y+6, x+5, y+6);  // Middle
            break;
        case 'B':
            SDL_RenderLine(renderer, x, y, x, y+9);        // Left vertical
            SDL_RenderLine(renderer, x, y, x+4, y);        // Top
            SDL_RenderLine(renderer, x+4, y, x+5, y+2);    // Top curve
            SDL_RenderLine(renderer, x+5, y+2, x+4, y+4);  // Middle top
            SDL_RenderLine(renderer, x, y+4, x+4, y+4);    // Middle
            SDL_RenderLine(renderer, x+4, y+4, x+5, y+7);  // Middle bottom
            SDL_RenderLine(renderer, x+5, y+7, x+4, y+9);  // Bottom curve
            SDL_RenderLine(renderer, x+4, y+9, x, y+9);    // Bottom
            break;
        case 'C':
            SDL_RenderLine(renderer, x+5, y+2, x+2, y);    // Top right
            SDL_RenderLine(renderer, x+2, y, x, y+2);      // Top left
            SDL_RenderLine(renderer, x, y+2, x, y+7);      // Left
            SDL_RenderLine(renderer, x, y+7, x+2, y+9);    // Bottom left
            SDL_RenderLine(renderer, x+2, y+9, x+5, y+7);  // Bottom right
            break;
        case 'D':
            SDL_RenderLine(renderer, x, y, x, y+9);        // Left vertical
            SDL_RenderLine(renderer, x, y, x+3, y);        // Top
            SDL_RenderLine(renderer, x+3, y, x+5, y+2);    // Top right
            SDL_RenderLine(renderer, x+5, y+2, x+5, y+7);  // Right
            SDL_RenderLine(renderer, x+5, y+7, x+3, y+9);  // Bottom right
            SDL_RenderLine(renderer, x+3, y+9, x, y+9);    // Bottom
            break;
        case 'E':
            SDL_RenderLine(renderer, x, y, x, y+9);        // Vertical
            SDL_RenderLine(renderer, x, y, x+5, y);        // Top
            SDL_RenderLine(renderer, x, y+4, x+4, y+4);    // Middle
            SDL_RenderLine(renderer, x, y+9, x+5, y+9);    // Bottom
            break;
        case 'F':
            SDL_RenderLine(renderer, x, y, x, y+9);        // Vertical
            SDL_RenderLine(renderer, x, y, x+5, y);        // Top
            SDL_RenderLine(renderer, x, y+4, x+4, y+4);    // Middle
            break;
        case 'G':
            SDL_RenderLine(renderer, x+5, y+2, x+2, y);    // Top right
            SDL_RenderLine(renderer, x+2, y, x, y+2);      // Top left
            SDL_RenderLine(renderer, x, y+2, x, y+7);      // Left
            SDL_RenderLine(renderer, x, y+7, x+2, y+9);    // Bottom left
            SDL_RenderLine(renderer, x+2, y+9, x+5, y+7);  // Bottom right
            SDL_RenderLine(renderer, x+5, y+7, x+5, y+5);  // Right
            SDL_RenderLine(renderer, x+5, y+5, x+3, y+5);  // G hook
            break;
        case 'H':
            SDL_RenderLine(renderer, x, y, x, y+9);        // Left vertical
            SDL_RenderLine(renderer, x+5, y, x+5, y+9);    // Right vertical
            SDL_RenderLine(renderer, x, y+4, x+5, y+4);    // Middle
            break;
        case 'I':
            SDL_RenderLine(renderer, x+2, y, x+2, y+9);    // Vertical
            break;
        case 'L':
            SDL_RenderLine(renderer, x, y, x, y+9);        // Vertical
            SDL_RenderLine(renderer, x, y+9, x+5, y+9);    // Bottom
            break;
        case 'M':
            SDL_RenderLine(renderer, x, y, x, y+9);        // Left vertical
            SDL_RenderLine(renderer, x+5, y, x+5, y+9);    // Right vertical
            SDL_RenderLine(renderer, x, y, x+2, y+5);      // Left diagonal
            SDL_RenderLine(renderer, x+5, y, x+3, y+5);    // Right diagonal
            break;
        case 'N':
            SDL_RenderLine(renderer, x, y, x, y+9);        // Left vertical
            SDL_RenderLine(renderer, x+5, y, x+5, y+9);    // Right vertical
            SDL_RenderLine(renderer, x, y, x+5, y+9);      // Diagonal
            break;
        case 'O':
            SDL_RenderLine(renderer, x+2, y, x+3, y);      // Top
            SDL_RenderLine(renderer, x, y+2, x, y+7);      // Left
            SDL_RenderLine(renderer, x+2, y+9, x+3, y+9);  // Bottom
            SDL_RenderLine(renderer, x+5, y+2, x+5, y+7);  // Right
            SDL_RenderLine(renderer, x+2, y, x, y+2);      // Top left
            SDL_RenderLine(renderer, x+3, y, x+5, y+2);    // Top right
            SDL_RenderLine(renderer, x, y+7, x+2, y+9);    // Bottom left
            SDL_RenderLine(renderer, x+5, y+7, x+3, y+9);  // Bottom right
            break;
        case 'P':
            SDL_RenderLine(renderer, x, y, x, y+9);        // Vertical
            SDL_RenderLine(renderer, x, y, x+3, y);        // Top
            SDL_RenderLine(renderer, x+3, y, x+5, y+2);    // Top curve
            SDL_RenderLine(renderer, x+5, y+2, x+3, y+5);  // Bottom curve
            SDL_RenderLine(renderer, x+3, y+5, x, y+5);    // Bottom
            break;
        case 'R':
            SDL_RenderLine(renderer, x, y, x, y+9);        // Vertical
            SDL_RenderLine(renderer, x, y, x+3, y);        // Top
            SDL_RenderLine(renderer, x+3, y, x+5, y+2);    // Top curve
            SDL_RenderLine(renderer, x+5, y+2, x+3, y+5);  // Bottom curve
            SDL_RenderLine(renderer, x+3, y+5, x, y+5);    // Middle
            SDL_RenderLine(renderer, x+2, y+5, x+5, y+9);  // Diagonal
            break;
        case 'S':
            SDL_RenderLine(renderer, x+5, y+2, x+2, y);    // Top right
            SDL_RenderLine(renderer, x+2, y, x, y+2);      // Top left
            SDL_RenderLine(renderer, x, y+2, x+2, y+4);    // Middle left
            SDL_RenderLine(renderer, x+2, y+4, x+3, y+4);  // Middle
            SDL_RenderLine(renderer, x+3, y+4, x+5, y+6);  // Middle right
            SDL_RenderLine(renderer, x+5, y+6, x+3, y+9);  // Bottom right
            SDL_RenderLine(renderer, x+3, y+9, x, y+7);    // Bottom left
            break;
        case 'T':
            SDL_RenderLine(renderer, x, y, x+5, y);        // Top
            SDL_RenderLine(renderer, x+2, y, x+2, y+9);    // Vertical
            break;
        case 'Y':
            SDL_RenderLine(renderer, x, y, x+2, y+4);      // Left diagonal
            SDL_RenderLine(renderer, x+5, y, x+3, y+4);    // Right diagonal
            SDL_RenderLine(renderer, x+2, y+4, x+2, y+9);  // Bottom vertical
            break;
        case '0':
            SDL_RenderLine(renderer, x+2, y, x+3, y);      // Top
            SDL_RenderLine(renderer, x, y+2, x, y+7);      // Left
            SDL_RenderLine(renderer, x+2, y+9, x+3, y+9);  // Bottom
            SDL_RenderLine(renderer, x+5, y+2, x+5, y+7);  // Right
            SDL_RenderLine(renderer, x+2, y, x, y+2);      // Top left
            SDL_RenderLine(renderer, x+3, y, x+5, y+2);    // Top right
            SDL_RenderLine(renderer, x, y+7, x+2, y+9);    // Bottom left
            SDL_RenderLine(renderer, x+5, y+7, x+3, y+9);  // Bottom right
            break;
        case '1':
            SDL_RenderLine(renderer, x+2, y, x+2, y+9);    // Vertical
            SDL_RenderLine(renderer, x+1, y+2, x+2, y);    // Diagonal
            break;
        case '2':
            SDL_RenderLine(renderer, x+1, y+1, x+2, y);    // Top left curve
            SDL_RenderLine(renderer, x+2, y, x+4, y);      // Top
            SDL_RenderLine(renderer, x+4, y, x+5, y+2);    // Top right curve
            SDL_RenderLine(renderer, x+5, y+2, x+3, y+5);  // Middle curve
            SDL_RenderLine(renderer, x+3, y+5, x, y+9);    // Bottom left diagonal
            SDL_RenderLine(renderer, x, y+9, x+5, y+9);    // Bottom
            break;
        case '3':
            SDL_RenderLine(renderer, x+1, y+1, x+3, y);    // Top left curve
            SDL_RenderLine(renderer, x+3, y, x+4, y+1);    // Top right curve
            SDL_RenderLine(renderer, x+4, y+1, x+3, y+4);  // Middle top curve
            SDL_RenderLine(renderer, x+3, y+4, x+4, y+5);  // Middle bottom curve
            SDL_RenderLine(renderer, x+4, y+5, x+3, y+8);  // Bottom right curve
            SDL_RenderLine(renderer, x+3, y+8, x+1, y+9);  // Bottom left curve
            SDL_RenderLine(renderer, x+2, y+4, x+3, y+4);  // Middle connect
            break;
        case '4':
            SDL_RenderLine(renderer, x+4, y, x+4, y+9);    // Right vertical
            SDL_RenderLine(renderer, x+4, y, x, y+6);      // Diagonal
            SDL_RenderLine(renderer, x, y+6, x+5, y+6);    // Horizontal
            break;
        case '5':
            SDL_RenderLine(renderer, x+5, y, x, y);        // Top
            SDL_RenderLine(renderer, x, y, x, y+4);        // Left vertical
            SDL_RenderLine(renderer, x, y+4, x+4, y+4);    // Middle
            SDL_RenderLine(renderer, x+4, y+4, x+5, y+6);  // Middle right curve
            SDL_RenderLine(renderer, x+5, y+6, x+4, y+9);  // Bottom right curve
            SDL_RenderLine(renderer, x+4, y+9, x, y+9);    // Bottom
            break;
        case '6':
            SDL_RenderLine(renderer, x+5, y+1, x+3, y);    // Top right curve
            SDL_RenderLine(renderer, x+3, y, x+1, y+1);    // Top left curve
            SDL_RenderLine(renderer, x+1, y+1, x, y+3);    // Upper left curve
            SDL_RenderLine(renderer, x, y+3, x, y+7);      // Left vertical
            SDL_RenderLine(renderer, x, y+7, x+2, y+9);    // Bottom left curve
            SDL_RenderLine(renderer, x+2, y+9, x+4, y+8);  // Bottom curve
            SDL_RenderLine(renderer, x+4, y+8, x+5, y+6);  // Bottom right curve
            SDL_RenderLine(renderer, x+5, y+6, x+4, y+4);  // Middle right curve
            SDL_RenderLine(renderer, x+4, y+4, x, y+4);    // Middle
            break;
        case '7':
            SDL_RenderLine(renderer, x, y, x+5, y);        // Top
            SDL_RenderLine(renderer, x+5, y, x+2, y+9);    // Diagonal
            break;
        case '8':
            SDL_RenderLine(renderer, x+1, y+1, x+2, y);    // Top left curve
            SDL_RenderLine(renderer, x+2, y, x+3, y);      // Top
            SDL_RenderLine(renderer, x+3, y, x+4, y+1);    // Top right curve
            SDL_RenderLine(renderer, x+4, y+1, x+4, y+3);  // Upper right vertical
            SDL_RenderLine(renderer, x+4, y+3, x+3, y+4);  // Middle top right
            SDL_RenderLine(renderer, x+3, y+4, x+2, y+4);  // Middle
            SDL_RenderLine(renderer, x+2, y+4, x+1, y+3);  // Middle top left
            SDL_RenderLine(renderer, x+1, y+3, x+1, y+1);  // Upper left vertical
            SDL_RenderLine(renderer, x+1, y+5, x+2, y+4);  // Middle bottom left
            SDL_RenderLine(renderer, x+2, y+4, x+3, y+4);  // Middle
            SDL_RenderLine(renderer, x+3, y+4, x+4, y+5);  // Middle bottom right
            SDL_RenderLine(renderer, x+4, y+5, x+4, y+7);  // Lower right vertical
            SDL_RenderLine(renderer, x+4, y+7, x+3, y+9);  // Bottom right curve
            SDL_RenderLine(renderer, x+3, y+9, x+2, y+9);  // Bottom
            SDL_RenderLine(renderer, x+2, y+9, x+1, y+7);  // Bottom left curve
            SDL_RenderLine(renderer, x+1, y+7, x+1, y+5);  // Lower left vertical
            break;
        case '9':
            SDL_RenderLine(renderer, x+1, y+1, x+2, y);    // Top left curve
            SDL_RenderLine(renderer, x+2, y, x+3, y);      // Top
            SDL_RenderLine(renderer, x+3, y, x+4, y+1);    // Top right curve
            SDL_RenderLine(renderer, x+4, y+1, x+5, y+3);  // Upper right curve
            SDL_RenderLine(renderer, x+5, y+3, x+5, y+7);  // Right vertical
            SDL_RenderLine(renderer, x+5, y+7, x+3, y+9);  // Bottom right curve
            SDL_RenderLine(renderer, x+3, y+9, x+1, y+8);  // Bottom curve
            SDL_RenderLine(renderer, x+1, y+1, x, y+3);    // Upper left curve
            SDL_RenderLine(renderer, x, y+3, x+1, y+5);    // Middle left curve
            SDL_RenderLine(renderer, x+1, y+5, x+5, y+5);  // Middle
            break;
        case ':':
            SDL_RenderPoint(renderer, x+2, y+2);           // Top dot
            SDL_RenderPoint(renderer, x+2, y+7);           // Bottom dot
            break;
        case ' ':
            // Space - do nothing
            break;
        case '-':
            SDL_RenderLine(renderer, x, y+4, x+4, y+4);    // Middle
            break;
        case '_':
            SDL_RenderLine(renderer, x, y+9, x+5, y+9);    // Bottom
            break;
        case '.':
            SDL_RenderPoint(renderer, x+2, y+9);           // Dot
            break;
        case ',':
            SDL_RenderLine(renderer, x+2, y+7, x+1, y+9);  // Comma
            break;
        case '!':
            SDL_RenderLine(renderer, x+2, y, x+2, y+6);    // Vertical
            SDL_RenderPoint(renderer, x+2, y+9);           // Bottom dot
            break;
        case '/':
            SDL_RenderLine(renderer, x+5, y, x, y+9);      // Diagonal
            break;
        case '\\':
            SDL_RenderLine(renderer, x, y, x+5, y+9);      // Diagonal
            break;
        case '(':
            SDL_RenderLine(renderer, x+3, y, x+1, y+4);    // Top curve
            SDL_RenderLine(renderer, x+1, y+4, x+3, y+9);  // Bottom curve
            break;
        case ')':
            SDL_RenderLine(renderer, x+1, y, x+3, y+4);    // Top curve
            SDL_RenderLine(renderer, x+3, y+4, x+1, y+9);  // Bottom curve
            break;
        case '+':
            SDL_RenderLine(renderer, x, y+4, x+4, y+4);    // Horizontal
            SDL_RenderLine(renderer, x+2, y+2, x+2, y+7);  // Vertical
            break;
        case '=':
            SDL_RenderLine(renderer, x, y+3, x+4, y+3);    // Top
            SDL_RenderLine(renderer, x, y+6, x+4, y+6);    // Bottom
            break;
        case '[':
            SDL_RenderLine(renderer, x+3, y, x+1, y);      // Top
            SDL_RenderLine(renderer, x+1, y, x+1, y+9);    // Vertical
            SDL_RenderLine(renderer, x+1, y+9, x+3, y+9);  // Bottom
            break;
        case ']':
            SDL_RenderLine(renderer, x+1, y, x+3, y);      // Top
            SDL_RenderLine(renderer, x+3, y, x+3, y+9);    // Vertical
            SDL_RenderLine(renderer, x+3, y+9, x+1, y+9);  // Bottom
            break;
        case '{':
            SDL_RenderLine(renderer, x+3, y, x+2, y+1);    // Top curve
            SDL_RenderLine(renderer, x+2, y+1, x+2, y+3);  // Upper vertical
            SDL_RenderLine(renderer, x+2, y+3, x+1, y+4);  // Middle top curve
            SDL_RenderLine(renderer, x+1, y+4, x+2, y+5);  // Middle bottom curve
            SDL_RenderLine(renderer, x+2, y+5, x+2, y+8);  // Lower vertical
            SDL_RenderLine(renderer, x+2, y+8, x+3, y+9);  // Bottom curve
            break;
        case '}':
            SDL_RenderLine(renderer, x+1, y, x+2, y+1);    // Top curve
            SDL_RenderLine(renderer, x+2, y+1, x+2, y+3);  // Upper vertical
            SDL_RenderLine(renderer, x+2, y+3, x+3, y+4);  // Middle top curve
            SDL_RenderLine(renderer, x+3, y+4, x+2, y+5);  // Middle bottom curve
            SDL_RenderLine(renderer, x+2, y+5, x+2, y+8);  // Lower vertical
            SDL_RenderLine(renderer, x+2, y+8, x+1, y+9);  // Bottom curve
            break;
        case '>':
            SDL_RenderLine(renderer, x, y+2, x+3, y+4);    // Top
            SDL_RenderLine(renderer, x+3, y+4, x, y+7);    // Bottom
            break;
        case '<':
            SDL_RenderLine(renderer, x+3, y+2, x, y+4);    // Top
            SDL_RenderLine(renderer, x, y+4, x+3, y+7);    // Bottom
            break;
        case '\'':
            SDL_RenderLine(renderer, x+2, y, x+2, y+2);    // Quote
            break;
        case '"':
            SDL_RenderLine(renderer, x+1, y, x+1, y+2);    // Left quote
            SDL_RenderLine(renderer, x+3, y, x+3, y+2);    // Right quote
            break;
        case '`':
            SDL_RenderLine(renderer, x+1, y, x+2, y+2);    // Backtick
            break;
        case '~':
            SDL_RenderLine(renderer, x, y+4, x+2, y+3);    // Left curve
            SDL_RenderLine(renderer, x+2, y+3, x+4, y+5);  // Right curve
            break;
        case '@':
            SDL_RenderLine(renderer, x+3, y, x+1, y+2);    // Top left curve
            SDL_RenderLine(renderer, x+1, y+2, x+1, y+7);  // Left vertical
            SDL_RenderLine(renderer, x+1, y+7, x+3, y+9);  // Bottom left curve
            SDL_RenderLine(renderer, x+3, y+9, x+5, y+7);  // Bottom right curve
            SDL_RenderLine(renderer, x+5, y+7, x+5, y+2);  // Right vertical
            SDL_RenderLine(renderer, x+5, y+2, x+3, y);    // Top right curve
            SDL_RenderLine(renderer, x+3, y+5, x+3, y+7);  // Inner vertical
            SDL_RenderLine(renderer, x+3, y+7, x+4, y+7);  // Inner horizontal
            SDL_RenderLine(renderer, x+4, y+7, x+5, y+5);  // Inner curve
            break;
        case '#':
            SDL_RenderLine(renderer, x+1, y+1, x+1, y+8);  // Left vertical
            SDL_RenderLine(renderer, x+4, y+1, x+4, y+8);  // Right vertical
            SDL_RenderLine(renderer, x, y+3, x+5, y+3);    // Top horizontal
            SDL_RenderLine(renderer, x, y+6, x+5, y+6);    // Bottom horizontal
            break;
        case '$':
            SDL_RenderLine(renderer, x+2, y, x+2, y+9);    // Middle vertical
            SDL_RenderLine(renderer, x+4, y+1, x+2, y+1);  // Top right
            SDL_RenderLine(renderer, x+2, y+1, x, y+3);    // Top left curve
            SDL_RenderLine(renderer, x, y+3, x+2, y+5);    // Middle left curve
            SDL_RenderLine(renderer, x+2, y+5, x+4, y+5);  // Middle
            SDL_RenderLine(renderer, x+4, y+5, x+5, y+7);  // Bottom right curve
            SDL_RenderLine(renderer, x+5, y+7, x+3, y+9);  // Bottom curve
            break;
        case '%':
            SDL_RenderLine(renderer, x+5, y, x, y+9);      // Main diagonal
            SDL_RenderPoint(renderer, x+1, y+2);           // Top circle
            SDL_RenderPoint(renderer, x+4, y+7);           // Bottom circle
            break;
        case '^':
            SDL_RenderLine(renderer, x+1, y+3, x+2, y);    // Left diagonal
            SDL_RenderLine(renderer, x+2, y, x+3, y+3);    // Right diagonal
            break;
        case '&':
            SDL_RenderLine(renderer, x+4, y+1, x+2, y);    // Top right
            SDL_RenderLine(renderer, x+2, y, x, y+2);      // Top left curve
            SDL_RenderLine(renderer, x, y+2, x+2, y+4);    // Middle left curve
            SDL_RenderLine(renderer, x+2, y+4, x, y+7);    // Middle right curve
            SDL_RenderLine(renderer, x, y+7, x+2, y+9);    // Bottom left curve
            SDL_RenderLine(renderer, x+2, y+9, x+4, y+7);  // Bottom right curve
            SDL_RenderLine(renderer, x+4, y+7, x+5, y+9);  // Bottom right diagonal
            break;
        case '*':
            SDL_RenderLine(renderer, x+2, y+2, x+2, y+7);  // Vertical
            SDL_RenderLine(renderer, x, y+4, x+4, y+4);    // Horizontal
            SDL_RenderLine(renderer, x+1, y+2, x+3, y+7);  // Diagonal 1
            SDL_RenderLine(renderer, x+3, y+2, x+1, y+7);  // Diagonal 2
            break;
        default:
            // For unknown characters, draw a rectangle
            SDL_FRect charBox = {
                static_cast<float>(x), static_cast<float>(y),
                5.0f, 9.0f
            };
            SDL_RenderRect(renderer, &charBox);
            break;
    }
}
