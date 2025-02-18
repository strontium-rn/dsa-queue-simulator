// traffic_generator/include/Generator.h
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
#include "core/Constants.h"

class Generator {
private:
    std::mt19937 rng;
    std::map<LaneId, std::filesystem::path> laneFiles;
    uint32_t nextVehicleId;
    std::filesystem::path dataDir;
    std::chrono::steady_clock::time_point lastGenTime;

    // Generation settings
    struct LaneSettings {
        double spawnProbability;
        int maxVehicles;
        std::string name;
    };
    std::map<LaneId, LaneSettings> laneSettings;

    // Private methods
    Direction generateRandomDirection();
    void writeVehicleToFile(const std::filesystem::path& filepath, uint32_t id, Direction dir);
    size_t countVehiclesInFile(const std::filesystem::path& filepath) const;  // Made const
    void initializeLaneSettings();
    bool shouldGenerateVehicle(LaneId laneId, size_t currentCount);
    void clearAllFiles();

public:
    Generator();
    void generateTraffic();
    void displayStatus() const;
};
