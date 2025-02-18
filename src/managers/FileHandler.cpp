#include "managers/FileHandler.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

FileHandler::FileHandler() {
    // Get the executable path and set up data directory
    dataDir = std::filesystem::current_path() / "data" / "lanes";
    std::filesystem::create_directories(dataDir);

    // Initialize lane file mappings with absolute paths
    laneFiles = {
        {LaneId::AL1_INCOMING, dataDir / "lane_a1.txt"},
        {LaneId::AL2_PRIORITY, dataDir / "lane_a2.txt"},
        {LaneId::AL3_FREELANE, dataDir / "lane_a3.txt"},
        {LaneId::BL1_INCOMING, dataDir / "lane_b1.txt"},
        {LaneId::BL2_NORMAL, dataDir / "lane_b2.txt"},
        {LaneId::BL3_FREELANE, dataDir / "lane_b3.txt"},
        {LaneId::CL1_INCOMING, dataDir / "lane_c1.txt"},
        {LaneId::CL2_NORMAL, dataDir / "lane_c2.txt"},
        {LaneId::CL3_FREELANE, dataDir / "lane_c3.txt"},
        {LaneId::DL1_INCOMING, dataDir / "lane_d1.txt"},
        {LaneId::DL2_NORMAL, dataDir / "lane_d2.txt"},
        {LaneId::DL3_FREELANE, dataDir / "lane_d3.txt"}
    };

    // Create empty files and initialize read positions
    for (const auto& [_, filepath] : laneFiles) {
        std::ofstream file(filepath.string());
        lastReadPositions[filepath] = 0;
    }
}

// src/managers/FileHandler.cpp
std::vector<std::pair<LaneId, std::shared_ptr<Vehicle>>> FileHandler::readNewVehicles() {
    std::vector<std::pair<LaneId, std::shared_ptr<Vehicle>>> newVehicles;

    for (const auto& [laneId, filepath] : laneFiles) {
        std::ifstream file(filepath.string(), std::ios::in);
        if (!file) continue;

        // Get file size
        file.seekg(0, std::ios::end);
        int64_t fileSize = file.tellg();

        // If there's new data
        if (fileSize > lastReadPositions[filepath]) {
            file.seekg(lastReadPositions[filepath]);
            std::string line;

            while (std::getline(file, line)) {
                if (line.empty()) continue;

                size_t commaPos = line.find(',');
                size_t semicolonPos = line.find(';');

                if (commaPos != std::string::npos && semicolonPos != std::string::npos) {
                    try {
                        uint32_t id = std::stoul(line.substr(0, commaPos));
                        char dirChar = line[commaPos + 1];

                        Direction dir;
                        switch (dirChar) {
                            case 'S': dir = Direction::STRAIGHT; break;
                            case 'L': dir = Direction::LEFT; break;
                            case 'R': dir = Direction::RIGHT; break;
                            default: continue;
                        }

                        auto vehicle = std::make_shared<Vehicle>(id, dir, laneId);
                        newVehicles.emplace_back(laneId, vehicle);

                        std::cout << "Read vehicle " << id << " from lane "
                                 << static_cast<int>(laneId) << std::endl;
                    } catch (...) {
                        std::cerr << "Error parsing line: " << line << std::endl;
                    }
                }
            }

            lastReadPositions[filepath] = file.tellg();
        }
    }

    return newVehicles;
}

std::vector<std::shared_ptr<Vehicle>> FileHandler::parseVehicleData(
    const std::string& data, LaneId laneId) {
    std::vector<std::shared_ptr<Vehicle>> vehicles;
    std::stringstream ss(data);
    std::string vehicleData;

    while (std::getline(ss, vehicleData, ';')) {
        if (vehicleData.empty()) continue;

        // Expected format: "id,direction"
        std::stringstream vehicleSS(vehicleData);
        std::string idStr, dirStr;

        if (std::getline(vehicleSS, idStr, ',') &&
            std::getline(vehicleSS, dirStr, ',')) {
            try {
                uint32_t id = std::stoul(idStr);
                Direction dir;

                if (dirStr == "S") dir = Direction::STRAIGHT;
                else if (dirStr == "L") dir = Direction::LEFT;
                else if (dirStr == "R") dir = Direction::RIGHT;
                else continue;

                vehicles.push_back(std::make_shared<Vehicle>(id, dir, laneId));
            } catch (...) {
                // Skip invalid data
                continue;
            }
        }
    }

    return vehicles;
}

void FileHandler::clearLaneFiles() {
    for (const auto& [_, filepath] : laneFiles) {
        std::ofstream file(filepath, std::ios::trunc);
        lastReadPositions[filepath] = 0;
    }
}



