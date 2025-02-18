// traffic_generator/src/main.cpp
#include "../include/Generator.h"
#include <thread>
#include <iostream>
#include <iomanip>

int main() {
    try {
        Generator generator;

        // Print header
        std::cout << "Traffic Generator Started\n"
                  << "========================\n\n"
                  << " ID  |        Lane         |  Direction | Status\n"
                  << "-----+--------------------+------------+--------\n";

        while (true) {
            generator.generateTraffic();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
