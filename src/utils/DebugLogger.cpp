#include "utils/DebugLogger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// Static class members initialization
std::string DebugLogger::logFilePath = "traffic_simulator.log";
std::vector<std::string> DebugLogger::recentLogs;
std::mutex DebugLogger::logMutex;
bool DebugLogger::initialized = false;

void DebugLogger::initialize(const std::string& path) {
    std::lock_guard<std::mutex> lock(logMutex);
    logFilePath = path;

    // Create/clear the log file
    std::ofstream file(logFilePath, std::ios::trunc);
    if (file.is_open()) {
        file << "=== Traffic Simulator Log ===\n";
        file.close();
    }

    initialized = true;
}

void DebugLogger::log(const std::string& message, LogLevel level) {
    if (!initialized) {
        initialize(); // Initialize with default path if not done already
    }

    std::string levelStr;
    switch (level) {
        case LogLevel::INFO:    levelStr = "INFO"; break;
        case LogLevel::WARNING: levelStr = "WARNING"; break;
        case LogLevel::ERROR:   levelStr = "ERROR"; break;
        case LogLevel::DEBUG:   levelStr = "DEBUG"; break;
        default:                levelStr = "INFO"; break;
    }

    std::string timestamp = getTimestamp();
    std::string formattedMessage = "[" + timestamp + "] [" + levelStr + "] " + message;

    // Store in recent logs (limited to last 100)
    {
        std::lock_guard<std::mutex> lock(logMutex);
        recentLogs.push_back(formattedMessage);
        if (recentLogs.size() > 100) {
            recentLogs.erase(recentLogs.begin());
        }
    }

    // Write to file
    writeToFile(formattedMessage);

    // Also output to console
    std::cout << formattedMessage << std::endl;
}

std::vector<std::string> DebugLogger::getRecentLogs(int count) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (count <= 0 || recentLogs.empty()) {
        return {};
    }

    if (count >= static_cast<int>(recentLogs.size())) {
        return recentLogs;
    }

    // Return last 'count' logs
    return std::vector<std::string>(
        recentLogs.end() - count,
        recentLogs.end()
    );
}

void DebugLogger::clearLogs() {
    std::lock_guard<std::mutex> lock(logMutex);
    recentLogs.clear();

    // Clear the log file
    std::ofstream file(logFilePath, std::ios::trunc);
    if (file.is_open()) {
        file << "=== Traffic Simulator Log (Cleared) ===\n";
        file.close();
    }
}

void DebugLogger::shutdown() {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!initialized) {
        return;
    }

    writeToFile("[" + getTimestamp() + "] [INFO] Logger shutdown");
    initialized = false;
}

std::string DebugLogger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

void DebugLogger::writeToFile(const std::string& message) {
    std::ofstream file(logFilePath, std::ios::app);
    if (file.is_open()) {
        file << message << std::endl;
        file.close();
    }
}
