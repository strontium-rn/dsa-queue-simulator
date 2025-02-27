// FILE: src/managers/FileHandler.cpp
#include "managers/FileHandler.h"
#include "utils/DebugLogger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

FileHandler::FileHandler(const std::string& dataPath)
    : dataPath(dataPath) {

    DebugLogger::log("FileHandler created with path: " + dataPath);
}

FileHandler::~FileHandler() {
    DebugLogger::log("FileHandler destroyed");
}

std::vector<Vehicle*> FileHandler::readVehiclesFromFiles() {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<Vehicle*> vehicles;

    // Ensure directory exists before trying to read
    if (!fs::exists(dataPath)) {
        DebugLogger::log("Data path doesn't exist: " + dataPath, DebugLogger::LogLevel::WARNING);
        return vehicles;
    }

    // Read from each lane file (A, B, C, D)
    for (char laneId : {'A', 'B', 'C', 'D'}) {
        std::string filePath = getLaneFilePath(laneId);

        // Only try to read if file exists
        if (fs::exists(filePath)) {
            auto laneVehicles = readVehiclesFromFile(laneId);
            vehicles.insert(vehicles.end(), laneVehicles.begin(), laneVehicles.end());
        }
    }

    // If we found any vehicles, log the total
    if (!vehicles.empty()) {
        std::ostringstream oss;
        oss << "Read " << vehicles.size() << " vehicles from lane files";
        DebugLogger::log(oss.str());
    }

    return vehicles;
}

std::vector<Vehicle*> FileHandler::readVehiclesFromFile(char laneId) {
    std::vector<Vehicle*> vehicles;
    std::string filePath = getLaneFilePath(laneId);

    // Multiple attempts to open file (addresses file locking issues)
    std::ifstream file;
    int attempts = 0;
    while (attempts < 3) {
        file.open(filePath);
        if (file.is_open()) {
            break;
        }
        // Wait and retry if file is locked
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        attempts++;
    }

    if (!file.is_open()) {
        return vehicles;
    }

    std::vector<std::string> lines;
    std::string line;

    // Read all lines from file
    while (std::getline(file, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    file.close();

    // Don't modify file if no lines were read
    if (lines.empty()) {
        return vehicles;
    }

    // Process lines first before clearing file (prevents data loss if parsing fails)
    std::vector<Vehicle*> parsedVehicles;
    for (const auto& line : lines) {
        Vehicle* vehicle = parseVehicleLine(line);
        if (vehicle) {
            parsedVehicles.push_back(vehicle);
        }
    }

    // Clear the file after reading to prevent duplicates - with error handling
    bool fileClearedSuccessfully = false;
    attempts = 0;
    while (!fileClearedSuccessfully && attempts < 3) {
        try {
            std::ofstream clearFile(filePath, std::ios::trunc);
            if (clearFile.is_open()) {
                clearFile.close();
                fileClearedSuccessfully = true;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                attempts++;
            }
        } catch (const std::exception& e) {
            DebugLogger::log("Error clearing file: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
            attempts++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    if (!fileClearedSuccessfully) {
        DebugLogger::log("Failed to clear file after reading: " + filePath, DebugLogger::LogLevel::ERROR);
    }

    // Log number of vehicles read
    if (!parsedVehicles.empty()) {
        std::ostringstream oss;
        oss << "Read " << parsedVehicles.size() << " vehicles from lane " << laneId;
        DebugLogger::log(oss.str());
    }

    return parsedVehicles;
}

Vehicle* FileHandler::parseVehicleLine(const std::string& line) {
    // Expected formats:
    // "vehicleId_L{laneNumber}:laneId"
    // "vehicleId_L{laneNumber}_DIRECTION:laneId"
    size_t pos = line.find(":");
    if (pos == std::string::npos) {
        DebugLogger::log("Error parsing line (missing colon): " + line, DebugLogger::LogLevel::ERROR);
        return nullptr;
    }

    std::string vehicleId = line.substr(0, pos);

    // Ensure there's a lane ID after the colon
    if (pos + 1 >= line.length()) {
        DebugLogger::log("Error parsing line (missing lane ID): " + line, DebugLogger::LogLevel::ERROR);
        return nullptr;
    }

    char laneId = line[pos + 1];

    // Extract lane number from ID (format: V1_L2 where 2 is the lane number)
    int laneNumber = 2; // Default is lane 2
    size_t lanePos = vehicleId.find("_L");
    if (lanePos != std::string::npos && lanePos + 2 < vehicleId.length()) {
        char laneNumChar = vehicleId[lanePos + 2];
        if (laneNumChar >= '1' && laneNumChar <= '3') {
            laneNumber = laneNumChar - '0';
        }
    }

    // Don't spawn vehicles in Lane 1 (L1)
    if (laneNumber == 1) {
        DebugLogger::log("Ignoring vehicle in Lane 1: " + line, DebugLogger::LogLevel::WARNING);
        return nullptr;
    }

    // Determine direction from ID - important for vehicle behavior
    Destination destination = Destination::STRAIGHT; // Default

    if (laneNumber == 3) {
        // Lane 3 always turns LEFT
        destination = Destination::LEFT;
    } else if (laneNumber == 2) {
        // Check for direction in ID
        if (vehicleId.find("_LEFT") != std::string::npos) {
            destination = Destination::LEFT;
        } else if (vehicleId.find("_STRAIGHT") != std::string::npos) {
            destination = Destination::STRAIGHT;
        } else {
            // Default for L2 if not specified is STRAIGHT
            destination = Destination::STRAIGHT;
        }
    }

    // Determine if it's an emergency vehicle
    bool isEmergency = vehicleId.find("_E") != std::string::npos || vehicleId.find("E_") != std::string::npos;

    // Validate lane ID
    if (laneId != 'A' && laneId != 'B' && laneId != 'C' && laneId != 'D') {
        DebugLogger::log("Invalid lane ID in line: " + line, DebugLogger::LogLevel::ERROR);
        return nullptr;
    }

    // Create the vehicle with the specified destination
    Vehicle* vehicle = new Vehicle(vehicleId, laneId, laneNumber, isEmergency);
    vehicle->setDestination(destination);

    std::ostringstream oss;
    oss << "Created vehicle " << vehicleId << " for lane " << laneId << laneNumber;
    switch (destination) {
        case Destination::STRAIGHT: oss << " (STRAIGHT)"; break;
        case Destination::LEFT: oss << " (LEFT)"; break;
        case Destination::RIGHT: oss << " (RIGHT)"; break;
    }
    if (isEmergency) {
        oss << " [EMERGENCY]";
    }
    DebugLogger::log(oss.str());

    return vehicle;
}

void FileHandler::writeLaneStatus(char laneId, int laneNumber, int vehicleCount, bool isPriority) {
    std::lock_guard<std::mutex> lock(mutex);
    std::string statusPath = getLaneStatusFilePath();

    // Make sure the directory exists
    fs::path dir = fs::path(statusPath).parent_path();
    if (!fs::exists(dir)) {
        try {
            fs::create_directories(dir);
        } catch (const std::exception& e) {
            DebugLogger::log("Error creating directory: " + std::string(e.what()),
                           DebugLogger::LogLevel::ERROR);
            return;
        }
    }

    std::ofstream file(statusPath, std::ios::app);
    if (file.is_open()) {
        file << laneId << laneNumber << ": " << vehicleCount << " vehicles"
             << (isPriority ? " (PRIORITY)" : "") << std::endl;
        file.close();
    } else {
        DebugLogger::log("Warning: Could not open lane status file for writing",
                       DebugLogger::LogLevel::WARNING);
    }
}

bool FileHandler::checkFilesExist() {
    // Make sure data directory exists
    if (!fs::exists(dataPath)) {
        DebugLogger::log("Data directory doesn't exist: " + dataPath, DebugLogger::LogLevel::WARNING);
        return false;
    }

    // Check for lane files
    bool allFilesExist = true;
    for (char laneId : {'A', 'B', 'C', 'D'}) {
        std::string filePath = getLaneFilePath(laneId);
        if (!fs::exists(filePath)) {
            DebugLogger::log("Lane file doesn't exist: " + filePath, DebugLogger::LogLevel::WARNING);
            allFilesExist = false;
        }
    }

    return allFilesExist;
}

bool FileHandler::initializeFiles() {
    std::lock_guard<std::mutex> lock(mutex);

    try {
        // Create data directory if it doesn't exist
        if (!fs::exists(dataPath)) {
            if (!fs::create_directories(dataPath)) {
                DebugLogger::log("Error: Failed to create directory " + dataPath,
                               DebugLogger::LogLevel::ERROR);
                return false;
            }
            DebugLogger::log("Created directory: " + dataPath);
        }

        // Create lane files if they don't exist
        for (char laneId : {'A', 'B', 'C', 'D'}) {
            std::string filePath = getLaneFilePath(laneId);
            if (!fs::exists(filePath)) {
                std::ofstream file(filePath);
                if (!file.is_open()) {
                    DebugLogger::log("Error: Failed to create file " + filePath,
                                   DebugLogger::LogLevel::ERROR);
                    return false;
                }
                file.close();
                DebugLogger::log("Created file: " + filePath);
            }
        }

        // Create or clear lane status file
        std::string statusPath = getLaneStatusFilePath();
        std::ofstream statusFile(statusPath, std::ios::trunc);
        if (!statusFile.is_open()) {
            DebugLogger::log("Error: Failed to create lane status file",
                           DebugLogger::LogLevel::ERROR);
            return false;
        }
        statusFile << "=== Lane Status Log ===\n";
        statusFile.close();

        DebugLogger::log("All files initialized successfully");
        return true;
    } catch (const std::exception& e) {
        DebugLogger::log("Error initializing files: " + std::string(e.what()),
                       DebugLogger::LogLevel::ERROR);
        return false;
    }
}

std::string FileHandler::getLaneFilePath(char laneId) const {
    return dataPath + "/lane" + laneId + ".txt";
}

std::string FileHandler::getLaneStatusFilePath() const {
    return dataPath + "/lane_status.txt";
}
