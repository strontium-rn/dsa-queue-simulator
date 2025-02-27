// FILE: include/managers/FileHandler.h
#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <string>
#include <vector>
#include <mutex>
#include "core/Vehicle.h"

class FileHandler {
public:
    FileHandler(const std::string& dataPath = "data/lanes");
    ~FileHandler();

    // Read vehicles from lane files
    std::vector<Vehicle*> readVehiclesFromFiles();

    // Write lane status to file (for debugging/monitoring)
    void writeLaneStatus(char laneId, int laneNumber, int vehicleCount, bool isPriority);

    // Check if files exist/are readable
    bool checkFilesExist();

    // Create directories and empty files if they don't exist
    bool initializeFiles();

private:
    std::string dataPath;
    std::mutex mutex;

    // Lane file paths
    std::string getLaneFilePath(char laneId) const;

    // Read vehicles from a specific lane file
    std::vector<Vehicle*> readVehiclesFromFile(char laneId);

    // Parse a vehicle line from the file
    Vehicle* parseVehicleLine(const std::string& line);

    // Get the lane status file path
    std::string getLaneStatusFilePath() const;
};

#endif // FILE_HANDLER_Hendif // FILE_HANDLER_H
