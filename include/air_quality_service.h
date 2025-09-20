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

struct ValueInterpretor {
    static std::string humidityStr(float const& value) {
        if (value < 40) {
            return "DRY";
        }
        if (value < 60) {
            return "OPTIMAL";
        }
        return "TOO HUMID";
    }

    static std::string bvocStr(float const& value) {
        if (value <= 200) {
            return "VERY GOOD";
        }
        if (value <= 300) {
            return "GOOD";
        }
        if (value <= 400) {
            return "ACCEPTABLE";
        }
        if (value <= 600) {
            return "MODERATE";
        }
        if (value <= 1000) {
            return "POOR";
        }
        return "BAD";
    }

    static std::string co2Str(float const& value) {
        if (value <= 400) {
            return "IDEAL";
        }
        if (value <= 800) {
            return "GOOD";
        }
        if (value <= 1000) {
            return "ACCEPTABLE";
        }
        if (value <= 1500) {
            return "POOR";
        }
        if (value <= 2500) {
            return "VERY POOR";
        }
        if (value <= 5000) {
            return "UNHEALTHY";
        }
        return "HAZARDOUS";
    }

    static std::string iaqStr(float const& value) {
        if (value <= 50) {
            return "EXCELLENT";
        }
        if (value <= 100) {
            return "GOOD";
        }
        if (value <= 150) {
            return "LIGHTLY POLLUTED";
        }
        if (value <= 200) {
            return "MODERATELY POLLUTED";
        }
        if (value <= 300) {
            return "HEAVILY POLLUTED";
        }
        return "SEVERELY POLLUTED";
    }

    static int iaqIndex(float const& value) {
        if (value < 2) {
            return 0;
        } else if (value < 51) {
            return 1;
        } else if (value < 101) {
            return 2;
        } else if (value < 151) {
            return 3;
        } else if (value < 201) {
            return 4;
        } else {
            return 5;
        }
    }

    static std::string gasStr(float const& value) {
        if (value <= 50) {
            return "POOR";
        }
        if (value < 70) {
            return "MODERATE";
        }
        if (value < 90) {
            return "GOOD";
        }
        return "VERY GOOD";
    }

};

class BSecProxy;

class AirQualityService {
public:
    static AirQualityService* sharedInstance();
    AirQualityService(const AirQualityService& obj) = delete; 
    void operator=(const AirQualityService &) = delete;

    void start();
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
