
// traffic_generator/src/Generator.cpp
#include "../include/Generator.h"

Generator::Generator() : nextVehicleId(1) {
    try {
        // Initialize RNG
        std::random_device rd;
        rng.seed(rd());
        lastGenTime = std::chrono::steady_clock::now();

        // Set up data directory
        dataDir = std::filesystem::current_path() / "data" / "lanes";
        std::filesystem::create_directories(dataDir);

        // Initialize lane settings
        initializeLaneSettings();

        // Initialize lane files
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

        clearAllFiles();
        std::cout << "Traffic Generator initialized. Data directory: " << dataDir << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error initializing Generator: " << e.what() << std::endl;
        throw;
    }
}

void Generator::initializeLaneSettings() {
    laneSettings = {
        {LaneId::AL1_INCOMING, {0.25, 12, "A1 (Left Incoming)"}},
        {LaneId::AL2_PRIORITY, {0.3, 15, "A2 (Left Priority)"}},
        {LaneId::AL3_FREELANE, {0.2, 8, "A3 (Left Free)"}},
        {LaneId::BL1_INCOMING, {0.25, 12, "B1 (Top Incoming)"}},
        {LaneId::BL2_NORMAL, {0.25, 12, "B2 (Top Normal)"}},
        {LaneId::BL3_FREELANE, {0.2, 8, "B3 (Top Free)"}},
        {LaneId::CL1_INCOMING, {0.25, 12, "C1 (Right Incoming)"}},
        {LaneId::CL2_NORMAL, {0.25, 12, "C2 (Right Normal)"}},
        {LaneId::CL3_FREELANE, {0.2, 8, "C3 (Right Free)"}},
        {LaneId::DL1_INCOMING, {0.25, 12, "D1 (Bottom Incoming)"}},
        {LaneId::DL2_NORMAL, {0.25, 12, "D2 (Bottom Normal)"}},
        {LaneId::DL3_FREELANE, {0.2, 8, "D3 (Bottom Free)"}}
    };
}

void Generator::clearAllFiles() {
    for (const auto& [_, filepath] : laneFiles) {
        try {
            std::ofstream file(filepath, std::ios::trunc);
            if (!file) {
                throw std::runtime_error("Failed to clear file: " + filepath.string());
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error clearing file: " << e.what() << std::endl;
            throw;
        }
    }
}

Direction Generator::generateRandomDirection() {
    std::uniform_int_distribution<> dist(0, 100);
    int roll = dist(rng);

    // 60% straight, 20% left, 20% right
    if (roll < 60) return Direction::STRAIGHT;
    if (roll < 80) return Direction::LEFT;
    return Direction::RIGHT;
}

void Generator::writeVehicleToFile(const std::filesystem::path& filepath, uint32_t id, Direction dir) {
    try {
        std::ofstream file(filepath.string(), std::ios::app);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + filepath.string());
        }

        char dirChar;
        std::string dirStr;
        switch (dir) {
            case Direction::STRAIGHT:
                dirChar = 'S';
                dirStr = "Straight";
                break;
            case Direction::LEFT:
                dirChar = 'L';
                dirStr = "Left";
                break;
            case Direction::RIGHT:
                dirChar = 'R';
                dirStr = "Right";
                break;
            default:
                dirChar = 'S';
                dirStr = "Straight";
        }

        file << id << "," << dirChar << ";\n";
        file.flush();

        // Find lane name for display
        auto laneIt = std::find_if(laneFiles.begin(), laneFiles.end(),
            [&filepath](const auto& pair) { return pair.second == filepath; });

        if (laneIt != laneFiles.end()) {
            const auto& settings = laneSettings[laneIt->first];
            size_t currentCount = countVehiclesInFile(filepath);

            std::cout << std::setw(4) << id << " | "
                     << std::setw(20) << settings.name << " | "
                     << std::setw(10) << dirStr << " | "
                     << "Vehicles: " << currentCount << "/"
                     << settings.maxVehicles << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error writing to file: " << e.what() << std::endl;
    }
}


size_t Generator::countVehiclesInFile(const std::filesystem::path& filepath) const {  // Made const
    try {
        std::ifstream file(filepath.string());
        if (!file) return 0;

        size_t count = 0;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) count++;
        }
        return count;
    }
    catch (const std::exception& e) {
        std::cerr << "Error counting vehicles in file: " << e.what() << std::endl;
        return 0;
    }
}

bool Generator::shouldGenerateVehicle(LaneId laneId, size_t currentCount) {
    if (laneSettings.find(laneId) == laneSettings.end()) return false;

    const auto& settings = laneSettings[laneId];
    if (currentCount >= settings.maxVehicles) return false;

    std::uniform_real_distribution<> dist(0.0, 1.0);
    return dist(rng) < settings.spawnProbability;
}

// In Generator.cpp, update the generateTraffic function:
void Generator::generateTraffic() {
    try {
        // Get current time
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastGenTime).count();

        // Add minimum delay between generations
        if (elapsed < 1000) { // 1 second minimum delay
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 - elapsed));
            return;
        }

        // Clear screen and reprint header periodically
        static int updateCounter = 0;
        if (updateCounter++ % 20 == 0) {
            std::cout << "\033[2J\033[H";  // Clear screen and move cursor to top
            std::cout << "Traffic Generator Started\n"
                     << "========================\n\n"
                     << std::setw(4) << "ID" << " | "
                     << std::setw(20) << "Lane" << " | "
                     << std::setw(10) << "Direction" << " | "
                     << "Status\n"
                     << "-----+--------------------+------------+--------\n";
        }

        std::uniform_real_distribution<> dist(0.0, 1.0);
        bool anyVehicleGenerated = false;

        for (const auto& [laneId, filepath] : laneFiles) {
            std::atomic<bool> fileAccessTimeout{false};
            std::thread timeoutThread([&fileAccessTimeout]() {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                fileAccessTimeout = true;
            });
            timeoutThread.detach();

            try {
                double spawnProbability;
                int maxVehicles;

                // Set spawn probabilities and max vehicles per lane
                switch (laneId) {
                    case LaneId::AL2_PRIORITY:
                        spawnProbability = 0.15;  // 15% chance for priority lane
                        maxVehicles = 15;
                        break;
                    case LaneId::AL3_FREELANE:
                    case LaneId::BL3_FREELANE:
                    case LaneId::CL3_FREELANE:
                    case LaneId::DL3_FREELANE:
                        spawnProbability = 0.1;   // 10% chance for free lanes
                        maxVehicles = 8;
                        break;
                    default:
                        spawnProbability = 0.12;  // 12% chance for normal lanes
                        maxVehicles = 12;
                        break;
                }

                // Check if file is accessible
                if (fileAccessTimeout) {
                    std::cerr << "Timeout accessing file: " << filepath << std::endl;
                    continue;
                }

                size_t currentVehicles = countVehiclesInFile(filepath);

                if (currentVehicles < maxVehicles && dist(rng) < spawnProbability) {
                    Direction dir = generateRandomDirection();
                    writeVehicleToFile(filepath, nextVehicleId, dir);
                    anyVehicleGenerated = true;
                    nextVehicleId++;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error processing lane: " << e.what() << std::endl;
                continue;
            }
        }

        lastGenTime = std::chrono::steady_clock::now();

        // Add a small delay if no vehicles were generated
        if (!anyVehicleGenerated) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error in traffic generation: " << e.what() << std::endl;
    }
}


void Generator::displayStatus() const {
    std::cout << "\nCurrent Lane Status:\n";
    std::cout << std::string(50, '-') << std::endl;

    for (const auto& [laneId, filepath] : laneFiles) {
        const auto& settings = laneSettings.at(laneId);
        size_t count = countVehiclesInFile(filepath);

        std::cout << std::setw(20) << settings.name << " | "
                 << std::setw(8) << count << "/"
                 << std::setw(3) << settings.maxVehicles
                 << " vehicles | Spawn rate: "
                 << std::fixed << std::setprecision(1)
                 << (settings.spawnProbability * 100.0) << "%\n";
    }
    std::cout << std::string(50, '-') << std::endl;
}
