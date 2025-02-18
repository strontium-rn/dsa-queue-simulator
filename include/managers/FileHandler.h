// include/managers/FileHandler.hpp
#pragma once
#include "core/Vehicle.h"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <fstream>

class FileHandler {
private:
    std::map<LaneId, std::filesystem::path> laneFiles;
    std::map<std::filesystem::path, int64_t> lastReadPositions;
    std::filesystem::path dataDir;

public:
    FileHandler();  // Declaration only, definition will be in cpp file

    std::vector<std::pair<LaneId, std::shared_ptr<Vehicle>>> readNewVehicles();
    void clearLaneFiles();

private:
    std::vector<std::shared_ptr<Vehicle>> parseVehicleData(const std::string& data, LaneId laneId);
};
