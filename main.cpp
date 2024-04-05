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
#include "homebridge_service.h"
#include "air_quality_service.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/sinks/rotating_file_sink.h"
#include "constants.h"

using namespace std;

void create_default_logger() {
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
    // Create a file rotating logger with 5mb size max and 3 rotated files.
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>("logs/log", 1048576 * 5, 3);
    file_sink->set_level(spdlog::level::debug);
    sinks.push_back(file_sink);
    auto combined_logger = std::make_shared<spdlog::logger>("default", begin(sinks), end(sinks));
    spdlog::set_default_logger(combined_logger);
}

int main(int, char**) {
    create_default_logger();
    spdlog::set_level(spdlog::level::info);

    spdlog::info("Init Homebridge service");
    HomeBridgeService homebridgeService(HomeBridgeServiceConfig{HOMEBRIDGE_URL, HOMEBRIDGE_PUBLISH_INTERVAL});
    homebridgeService.start();

    AirQualityService* airQualityService = AirQualityService::sharedInstance();
    airQualityService->setOnAirQualityChange([&](AirQuality airQuality) {
        spdlog::info("Air quality changed: iaq={} (accuracy: {}),temperature={}, pressure={}, humidity={} co2={}, bVOC={}, gas={}",
            airQuality.iaq, airQuality.iaq_accuracy, airQuality.temperature, airQuality.pressure, airQuality.humidity, airQuality.co2, airQuality.bVOC, airQuality.gas_percentage);

            homebridgeService.update("rpi4temperature", airQuality.temperature - IAQ_TEMP_OFFSET);
            homebridgeService.update("rpi4humidity", airQuality.humidity);

            float homebridgeIaq;
            if (airQuality.iaq_accuracy < 2) {
                homebridgeIaq = 0;
            } else if (airQuality.iaq < 51) {
                homebridgeIaq = 1;
            } else if (airQuality.iaq < 101) {
                homebridgeIaq = 2;
            } else if (airQuality.iaq < 151) {
                homebridgeIaq = 3;
            } else if (airQuality.iaq < 201) {
                homebridgeIaq = 4;
            } else {
                homebridgeIaq = 5;
            }
            homebridgeService.update("rpi4iaq", homebridgeIaq);
    });
    airQualityService->monitor();
    homebridgeService.stop();

    spdlog::info("program ended.");
}
