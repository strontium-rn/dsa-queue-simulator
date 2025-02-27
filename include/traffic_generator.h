// include/traffic_generator.h
#pragma once
#include <string>
#include <random>
#include <map>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <mutex>

// Define constants similar to those in the main project
namespace Constants {
    static constexpr int MAX_QUEUE_SIZE = 100;
    static constexpr int PRIORITY_THRESHOLD = 10;
    static constexpr int NORMAL_THRESHOLD = 5;

    // Lane IDs (matching the main project)
    enum class LaneId {
        AL1_INCOMING = 0,
        AL2_PRIORITY = 1,
        AL3_FREELANE = 2,
        BL1_INCOMING = 3,
        BL2_NORMAL = 4,
        BL3_FREELANE = 5,
        CL1_INCOMING = 6,
        CL2_NORMAL = 7,
        CL3_FREELANE = 8,
        DL1_INCOMING = 9,
        DL2_NORMAL = 10,
        DL3_FREELANE = 11
    };

    // Direction values
    enum class Direction {
        STRAIGHT = 0,
        LEFT = 1,
        RIGHT = 2
    };
}

// Generator class to create vehicle data
class Generator {
private:
    std::mt19937 rng;  // Random number generator
    std::map<Constants::LaneId, std::filesystem::path> laneFiles;  // Lane file paths
    uint32_t nextVehicleId;  // ID counter for vehicles
    std::filesystem::path dataDir;  // Directory for data files
    std::mutex fileMutex;  // Thread safety for file operations

    // Settings for each lane
    struct LaneConfig {
        double spawnRate;
        int maxVehicles;
        bool canGoStraight;
        bool canGoLeft;
        bool canGoRight;
    };
    std::map<Constants::LaneId, LaneConfig> laneConfigs;

    // Private helper methods
    void initializeLaneFiles();
    void setupLaneConfigs();
    Constants::Direction getRandomDirection(const LaneConfig& config);
    size_t countVehiclesInFile(const std::filesystem::path& filepath) const;
    void writeVehicleToFile(const std::filesystem::path& filepath, uint32_t id,
                          Constants::Direction dir);
    void writeVehicleToCombinedFile(Constants::LaneId lane, uint32_t id);
    bool shouldGenerateVehicle(const LaneConfig& config, size_t currentCount);
    void clearAllFiles();
    void logGeneration(Constants::LaneId lane, uint32_t vehicleId, Constants::Direction dir,
                     size_t currentCount, int maxCount);

public:
    Generator();

    // Generate traffic for all lanes
    void generateTraffic();

    // Display generator status
    void displayStatus() const;
};
