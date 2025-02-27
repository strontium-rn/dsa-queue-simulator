#ifndef DEBUG_LOGGER_H
#define DEBUG_LOGGER_H

#include <string>
#include <vector>
#include <mutex>

class DebugLogger {
public:
    // Log levels
    enum class LogLevel {
        INFO,
        WARNING,
        ERROR,
        DEBUG
    };

    // Initialize the logger
    static void initialize(const std::string& logFilePath = "traffic_simulator.log");

    // Log a message with a specific level
    static void log(const std::string& message, LogLevel level = LogLevel::INFO);

    // Get recent log messages for display
    static std::vector<std::string> getRecentLogs(int count = 10);

    // Clear all logs
    static void clearLogs();

    // Shutdown the logger
    static void shutdown();

private:
    static std::string logFilePath;
    static std::vector<std::string> recentLogs;
    static std::mutex logMutex;
    static bool initialized;

    // Get timestamp for log messages
    static std::string getTimestamp();

    // Write to log file
    static void writeToFile(const std::string& message);
};

#endif // DEBUG_LOGGER_H
