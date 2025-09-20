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

#ifndef HOMEBRIDGE_SERVICE_H_
#define HOMEBRIDGE_SERVICE_H_
#include <exception>
#include <string>
#include <map>
#include <mutex>
#include <thread>

struct HomeBridgeServiceConfig {
    std::string url;        // HomeBridge instance URL
    int publishInterval;    // Publish interval in seconds
};

class HomeBridgeServiceError: public std::exception {
private:
    std::string message;
public:
    HomeBridgeServiceError(std::string &message): message(message) { } 
    std::string what() {
        return message;
    }
};

class HomeBridgeService {
private:
    HomeBridgeServiceConfig config;
    bool running;
    std::thread publishing_thread;
    std::mutex sensors_map_mutex;
    std::map<std::string, double> sensors;          // last updated sensors values
    std::map<std::string, double> next_sensors;     // next sensors values to update
    
    void publish(const std::string& sensor_id, double value);
    
public:
    HomeBridgeService(HomeBridgeServiceConfig config);
    ~HomeBridgeService();

    /// @brief Update the value of a sensor
    /// @param sensor_id the HomeBridge sensor ID
    /// @param value 
    void update(const std::string& sensor_id, double value);

    /// @brief Start the HomeBridge service
    void start();

    /// @brief Stop the HomeBridge service
    void stop();
};

#endif // HOMEBRIDGE_SERVICE_H_