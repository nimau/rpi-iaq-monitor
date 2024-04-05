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

#ifndef AIR_QUALITY_SERVICE_H_
#define AIR_QUALITY_SERVICE_H_

#include <unistd.h>
#include <cstdint>
#include <functional>
#include <mutex>
#include "simple_i2c_bus.h"

struct AirQuality {
    float iaq;
    int iaq_accuracy;
    float temperature;
    float pressure;
    float humidity;
    float co2;
    float bVOC;
    float gas_percentage;
};

class BSecProxy;

class AirQualityService {
public:
    static AirQualityService* sharedInstance();
    AirQualityService(const AirQualityService& obj) = delete; 
    void operator=(const AirQualityService &) = delete;

    int monitor();
    void setOnAirQualityChange(std::function<void(AirQuality)> onQualityChange);

    friend class BSecProxy;

private:
    AirQualityService();

    static AirQualityService* shared;
    static std::mutex sharedInstanceMutex;

    SimpleI2CBus i2c_bus;
    std::function<void(AirQuality)> onAirQualityChange;
    void outputReady(AirQuality output);
    int8_t readI2CRegister(uint8_t reg_addr, uint8_t *reg_data_ptr, uint32_t data_len);
    int8_t writeI2CRegister(uint8_t reg_addr, const uint8_t *reg_data_ptr, uint32_t data_len);
};

#endif // AIR_QUALITY_SERVICE_H_