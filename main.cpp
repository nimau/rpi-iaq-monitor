/*
* RPi IAQ Monitor
* Copyright (C) 2024  Nicolas Mauri
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <string>
#include <cstring>
#include "homebridge_service.h"
#include "air_quality_service.h"
#include "ConfigManager.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/sinks/rotating_file_sink.h"

using namespace std;

void create_default_logger() {
    std::vector<spdlog::sink_ptr> sinks;
    
    // Console sink with color
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    console_sink->set_pattern("[%^%l%$] %v");
    sinks.push_back(console_sink);
    
    // File sink with rotation
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "air_quality_monitor.log", 1048576 * 5, 3);
    file_sink->set_level(spdlog::level::debug);
    file_sink->set_pattern("[%H:%M:%S %z] [%^%l%$] %v");
    sinks.push_back(file_sink);
    
    // Create logger
    auto logger = std::make_shared<spdlog::logger>("default", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(logger);
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "RPi IAQ Monitor - Indoor Air Quality monitoring with Homebridge integration\n\n";
    std::cout << "Options:\n";
    std::cout << "  -c, --config PATH    Specify configuration file path (default: ./config.yaml)\n";
    std::cout << "  -h, --help          Show this help message and exit\n\n";
    std::cout << "Configuration:\n";
    std::cout << "  The application uses a YAML configuration file to specify settings like\n";
    std::cout << "  Homebridge URL, sensor parameters, and file paths. If no configuration file\n";
    std::cout << "  exists, a default one will be created automatically.\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << program_name << "                    # Use default config.yaml\n";
    std::cout << "  " << program_name << " -c /path/to/my.yaml  # Use custom config file\n";
}

std::string parse_command_line(int argc, char** argv) {
    std::string configPath = "./config.yaml"; // default
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) && i + 1 < argc) {
            configPath = argv[i + 1];
            i++; // Skip next argument since it's the config path
        } else {
            std::cerr << "Unknown argument: " << argv[i] << std::endl;
            print_usage(argv[0]);
            exit(1);
        }
    }
    
    return configPath;
}

int main(int argc, char** argv) {
    create_default_logger();
    spdlog::set_level(spdlog::level::info);
    
    // Parse command line arguments
    std::string configPath = parse_command_line(argc, argv);
    
    // Load configuration
    if (!ConfigManager::instance().load(configPath)) {
        spdlog::error("Failed to load configuration, but continuing with defaults");
    }
    
    const Config& config = ConfigManager::instance().get();
    
    // Initialize Homebridge service based on configuration
    std::unique_ptr<HomeBridgeService> homebridgeService;
    if (!config.homebridge_url.empty()) {
        spdlog::info("Initializing Homebridge service with URL: {}", config.homebridge_url);
        homebridgeService = std::make_unique<HomeBridgeService>(
            HomeBridgeServiceConfig{config.homebridge_url, config.homebridge_publish_interval_seconds}
        );
        homebridgeService->start();
    } else {
        spdlog::info("Homebridge service disabled (empty URL in configuration)");
    }
    
    // Initialize Air Quality Service
    AirQualityService* airQualityService = AirQualityService::sharedInstance();
    
    // Set up air quality monitoring callback
    airQualityService->setOnAirQualityChange([&](AirQuality airQuality) {
        float correctedTemp = airQuality.temperature - config.iaq_temp_offset;
        
        spdlog::info("IAQ={} (accuracy: {}), T°={}, P={}, H={}, CO2={}, VOC={}, Gas={}", 
            ValueInterpretor::iaqStr(airQuality.iaq), 
            airQuality.iaq_accuracy, 
            int(round(correctedTemp)), 
            int(airQuality.pressure / 100.0), 
            ValueInterpretor::humidityStr(airQuality.humidity), 
            ValueInterpretor::co2Str(airQuality.co2), 
            ValueInterpretor::bvocStr(airQuality.bVOC), 
            ValueInterpretor::gasStr(airQuality.gas_percentage));
        
        // Update Homebridge if service is available
        if (homebridgeService) {
            homebridgeService->update("rpi4temperature", correctedTemp);
            homebridgeService->update("rpi4humidity", airQuality.humidity);
            homebridgeService->update("rpi4pressure", airQuality.pressure / 100.0);
            homebridgeService->update("rpi4iaq", airQuality.iaq);
            homebridgeService->update("rpi4co2", airQuality.co2);
            homebridgeService->update("rpi4bvoc", airQuality.bVOC);
            homebridgeService->update("rpi4gas", airQuality.gas_percentage);
            homebridgeService->update("rpi4iaq_accuracy", airQuality.iaq_accuracy);
        }
    });
    
    // Start air quality monitoring - the service will handle all I2C and sensor configuration
    airQualityService->start();
    
    return 0;
}
