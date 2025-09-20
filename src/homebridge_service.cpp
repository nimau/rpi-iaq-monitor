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

#include "homebridge_service.h"
#include <iostream>
#include "constants.h"
#include <cpr/cpr.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <mutex>

using namespace std;

HomeBridgeService::HomeBridgeService(HomeBridgeServiceConfig config) {
    this->config = config;
    running = false;
}

HomeBridgeService::~HomeBridgeService() {
    stop();
    if (publishing_thread.joinable()) {
        publishing_thread.join();
    }
}

void HomeBridgeService::update(const string& sensor_id, double value) {
    sensors_map_mutex.lock();
    next_sensors[sensor_id] = value;
    sensors_map_mutex.unlock();
}

void HomeBridgeService::publish(const string& sensor_id, double value) {
    // Skip publishing if URL is empty
    if (string(config.url).empty()) {
        spdlog::debug("[HomeBridgeService] Skipping publish (URL not configured): {}: {}", sensor_id, value);
        sensors[string(sensor_id)] = value;
        return;
    }
    
    spdlog::debug("[HomeBridgeService] publishing {}: {}", sensor_id, value);
    cpr::Url URL{config.url};
    cpr::Parameters params{
        {"accessoryId", sensor_id},
        {"value", to_string(value)}
    };
    cpr::Response response{cpr::Get(URL, params)};
    if (response.status_code != 200) {
        throw HomeBridgeServiceError(response.text);
    }
    sensors[string(sensor_id)] = value;
}

void HomeBridgeService::start() {
    if (running) {
        return;
    }
    
    // Check if HomeBridge is configured
    if (string(config.url).empty()) {
        spdlog::info("[HomeBridgeService] HomeBridge URL not configured - service will run in local-only mode");
    }
    
    publishing_thread = thread([=]() {
        spdlog::info("[HomeBridgeService] started");
        running = true;
        while (running) {
            sensors_map_mutex.lock();
            for (auto& sensor : next_sensors) {
                sensors[sensor.first] = sensor.second;
            }
            next_sensors.clear();
            sensors_map_mutex.unlock();
            for (auto& sensor : sensors) {
                try {
                    publish(sensor.first.c_str(), sensor.second);
                } catch (HomeBridgeServiceError& e) {
                    spdlog::error("[HomeBridgeService] Error: {}", e.what());
                } catch (exception& e) {
                    spdlog::error("[HomeBridgeService] Error: {}", e.what());
                }
            }
            this_thread::sleep_for(chrono::seconds(config.publishInterval));
        }
        running = false;
        spdlog::info("[HomeBridgeService] stopped");
    });
}

void HomeBridgeService::stop() {
    running = false;
}
