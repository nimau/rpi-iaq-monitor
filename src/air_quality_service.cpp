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

#include "air_quality_service.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <time.h>
#include "bsec_integration.h"
#include <sys/time.h>
#include "constants.h"
#include "precision_timing.h"

namespace fs = std::filesystem;
using namespace std;

#define I2C_BUS_ADDRESS BME68X_I2C_ADDR_HIGH 

#pragma pack(push, 1)
struct BSECSerializedState {
    uint32_t n_serialized_state;
    uint8_t serialized_state[BSEC_MAX_STATE_BLOB_SIZE];
};
#pragma pack(pop)

/**********************************************************************************************************************/
/* BSecProxy */
/**********************************************************************************************************************/

class BSecProxy {
private:
    BSecProxy() {}
    ~BSecProxy() {}
    
public:
    /*!
    * @brief           Write operation in either Wire or SPI
    *
    * param[in]        reg_addr        register address
    * param[in]        reg_data_ptr    pointer to the data to be written
    * param[in]        data_len        number of bytes to be written
    * param[in]        intf_ptr        interface pointer
    *
    * @return          result of the bus communication function
    */
    static int8_t bsec_i2c_register_write(uint8_t reg_addr, const uint8_t *reg_data_ptr, uint32_t data_len, void *intf_ptr) {
        int8_t ret = AirQualityService::sharedInstance()->writeI2CRegister(reg_addr, reg_data_ptr, data_len);
        return (ret < 0) ? BME68X_E_COM_FAIL : BME68X_OK;
    }

    /*!
    * @brief           Read operation in either Wire or SPI
    *
    * param[in]        reg_addr        register address
    * param[out]       reg_data_ptr    pointer to the memory to be used to store the read data
    * param[in]        data_len        number of bytes to be read
    * param[in]        intf_ptr        interface pointer
    * 
    * @return          result of the bus communication function
    */
    static int8_t bsec_i2c_register_read(uint8_t reg_addr, uint8_t *reg_data_ptr, uint32_t data_len, void *intf_ptr) {
        int8_t ret = AirQualityService::sharedInstance()->readI2CRegister(reg_addr, reg_data_ptr, data_len);
        return (ret < 0) ? BME68X_E_COM_FAIL : BME68X_OK; 
    }

    /*!
    * @brief           Capture the system time in microseconds
    *
    * @return          system_current_time    current system timestamp in microseconds
    */
    static int64_t bsec_get_timestamp_us() {
        // Simple relative timestamps - proven to work with BSEC
        static int64_t start_time_us = 0;
        int64_t current_us = PrecisionTiming::now_us();
        
        if (start_time_us == 0) start_time_us = current_us;
        return current_us - start_time_us;
    }

    /*!
    * @brief           System specific implementation of sleep function
    *
    * @param[in]       t_us     Time in microseconds
    * @param[in]       intf_ptr Pointer to the interface descriptor
    * 
    * @return          none
    */
    static void bsec_sleep_n(uint32_t t_us, void *intf_ptr) {
        // Use precise nanosleep - this was the working solution
        struct timespec req;
        req.tv_sec = t_us / 1000000;
        req.tv_nsec = (t_us % 1000000) * 1000;
        nanosleep(&req, NULL);
    }

    /*!
    * @brief           Handling of the ready outputs
    *
    * @param[in]       outputs               	output_t structure
    * @param[in]       bsec_status             value returned by the bsec_do_steps() call
    *
    * @return          none
    */
    static void bsec_output_ready(output_t *outputs, bsec_library_return_t bsec_status) {
    if (bsec_status == BSEC_OK) {
        AirQualityService::sharedInstance()->onAirQualityChange(AirQuality {
            .iaq = outputs->iaq,
            .iaq_accuracy = outputs->iaq_accuracy,
            .temperature = outputs->temperature,
            .pressure = outputs->raw_pressure,
            .humidity = outputs->humidity,
            .co2 = outputs->co2_equivalent,
            .bVOC = outputs->breath_voc_equivalent,
            .gas_percentage = outputs->gas_percentage
        });
    } else {
        spdlog::debug("[BSecProxy] output_ready: bsec_status: {}", bsec_status);
    }
    }

    /*!
    * @brief           Load previous library state from non-volatile memory
    *
    * @param[in,out]   state_buffer    buffer to hold the loaded state string
    * @param[in]       n_buffer        size of the allocated state buffer
    *
    * @return          number of bytes copied to state_buffer
    */
    static uint32_t bsec_state_load(uint8_t *state_buffer, uint32_t n_buffer) {
        spdlog::info("[BSecProxy] BSec restore state...");

        // Here we will load a state string from a previous use of BSEC
        fstream bsec_state_file;
        string file_path = string(IAQ_SAVED_STATE_DIR) + "/" + IAQ_SAVED_STATE_FILE;
        if (!fs::exists(file_path)) {
            spdlog::debug("[BSecProxy] State file does not exist");
            return 0;
        }
        bsec_state_file.open(file_path, ios::in | ios::binary);
        char bufferRead[sizeof(BSECSerializedState)];
        bsec_state_file.read(bufferRead, sizeof(bufferRead));
        bsec_state_file.close();

        BSECSerializedState state;
        memcpy(&state, bufferRead, sizeof(BSECSerializedState));
        memcpy(state_buffer, state.serialized_state, state.n_serialized_state);
        return state.n_serialized_state;
    }

    /*!
    * @brief           Save library state to non-volatile memory
    *
    * @param[in]       state_buffer    buffer holding the state to be stored
    * @param[in]       length          length of the state string to be stored
    *
    * @return          none
    */
    static void bsec_state_save(const uint8_t *state_buffer, uint32_t length) {
        spdlog::info("[BSecProxy] BSec save state...");

        BSECSerializedState state;
        state.n_serialized_state = length;
        memcpy(state.serialized_state, state_buffer, length * sizeof(uint8_t));

        if (!fs::exists(IAQ_SAVED_STATE_DIR)) {
            spdlog::debug("[BSecProxy] State folder does not exist");
            fs::create_directory(IAQ_SAVED_STATE_DIR);
        }

        fstream bsec_state_file;
        string file_path = string(IAQ_SAVED_STATE_DIR) + "/" + IAQ_SAVED_STATE_FILE;
        bsec_state_file.open(file_path, ios::out | ios::binary);
        bsec_state_file.write(reinterpret_cast<char*>(&state), sizeof(BSECSerializedState));
        bsec_state_file.close();
    }
    
    /*!
    * @brief           Load library config from non-volatile memory
    *
    * @param[in,out]   config_buffer    buffer to hold the loaded state string
    * @param[in]       n_buffer        size of the allocated state buffer
    *
    * @return          number of bytes copied to config_buffer
    */
    static uint32_t bsec_config_load(uint8_t *config_buffer, uint32_t n_buffer) {
        spdlog::info("[BSecProxy] BSec restore config...");        
        // 33v 3s 4d
        const uint8_t bsec_config_iaq[492] = 
            {0,1,6,2,61,0,0,0,0,0,0,0,212,1,0,0,56,0,1,0,0,192,168,71,64,49,119,76,0,0,97,69,0,0,97,69,10,0,3,0,0,0,96,64,23,183,209,56,43,24,149,60,140,74,106,188,43,24,149,60,216,129,243,190,151,255,80,190,216,129,243,190,8,0,2,0,0,0,72,66,16,0,3,0,10,215,163,60,10,215,35,59,10,215,35,59,13,0,5,0,0,0,0,0,100,35,41,29,86,88,0,9,0,229,208,34,62,0,0,0,0,0,0,0,0,218,27,156,62,225,11,67,64,0,0,160,64,0,0,0,0,0,0,0,0,94,75,72,189,93,254,159,64,66,62,160,191,0,0,0,0,0,0,0,0,33,31,180,190,138,176,97,64,65,241,99,190,0,0,0,0,0,0,0,0,167,121,71,61,165,189,41,192,184,30,189,64,12,0,10,0,0,0,0,0,0,0,0,0,19,1,254,0,2,1,5,48,117,100,0,44,1,112,23,151,7,132,3,197,0,92,4,144,1,64,1,64,1,144,1,48,117,48,117,48,117,48,117,100,0,100,0,100,0,48,117,48,117,48,117,100,0,100,0,48,117,48,117,100,0,100,0,100,0,100,0,48,117,48,117,48,117,100,0,100,0,100,0,48,117,48,117,100,0,100,0,44,1,44,1,44,1,44,1,44,1,44,1,44,1,44,1,44,1,44,1,44,1,44,1,44,1,44,1,112,23,112,23,112,23,112,23,8,7,8,7,8,7,8,7,112,23,112,23,112,23,112,23,112,23,112,23,255,255,255,255,255,255,255,255,112,23,112,23,112,23,112,23,255,255,255,255,220,5,220,5,255,255,255,255,255,255,255,255,255,255,255,255,220,5,220,5,220,5,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,44,1,0,5,10,5,0,2,0,10,0,30,0,5,0,5,0,5,0,5,0,5,0,5,0,64,1,100,0,100,0,100,0,200,0,200,0,200,0,64,1,64,1,64,1,10,0,0,0,0,0,0,249,234,0,0};

        memcpy(config_buffer, bsec_config_iaq, n_buffer);
        return sizeof(bsec_config_iaq);
    }
};


/**********************************************************************************************************************/
/* AirQualityService Public Implementation */
/**********************************************************************************************************************/

AirQualityService* AirQualityService::shared {nullptr};
std::mutex AirQualityService::sharedInstanceMutex;

AirQualityService::AirQualityService() {
    spdlog::debug("AirQualityService init");
}

AirQualityService* AirQualityService::sharedInstance() {
    std::lock_guard<std::mutex> lock(sharedInstanceMutex);
    if (shared == nullptr)
    {
        shared = new AirQualityService();
    }
    return shared;
}

int AirQualityService::monitor() {
    return_values_init ret;

    spdlog::info("[AirQualityService] init");

    if (i2c_bus.openI2CBus(IAQ_I2C_BUS_DEVICE, I2C_BUS_ADDRESS) < 0) {
        spdlog::error("[AirQualityService] Failed to open the i2c bus");
        return -1;
    }

    // Get bsec version
    bsec_version_t version;
    bsec_get_version_m(bsecInstance, &version);
    spdlog::info("[AirQualityService] BSEC version: {}.{}.{}.{}", version.major, version.minor, version.major_bugfix, version.minor_bugfix);

    struct bme68x_dev bme_dev[NUM_OF_SENS];
    for (uint8_t i = 0; i < NUM_OF_SENS; ++i) {   
        /* Assigning a chunk of memory block to the bsecInstance */
        allocateMemory(bsec_mem_block[i], i);
        memset(&bme_dev[i],0,sizeof(bme_dev[i]));
        bme_dev[i].intf = BME68X_I2C_INTF;
        bme_dev[i].read = BSecProxy::bsec_i2c_register_read;
        bme_dev[i].write = BSecProxy::bsec_i2c_register_write;
        bme_dev[i].delay_us = BSecProxy::bsec_sleep_n;
        bme_dev[i].intf_ptr = nullptr;
        bme_dev[i].amb_temp = 0;

        /* Call to the function which initializes the BSEC library */
        ret = bsec_iot_init(SAMPLE_RATE, 0.0f, 
            BSecProxy::bsec_i2c_register_write, 
            BSecProxy::bsec_i2c_register_read, 
            BSecProxy::bsec_sleep_n, 
            BSecProxy::bsec_state_load, 
            BSecProxy::bsec_config_load, 
            bme_dev[i], i);
        if (ret.bme68x_status != BME68X_OK)
        {
            /* Could not intialize BME68x */
            spdlog::error("[AirQualityService] Could not intialize BME68x -> Error: {}", ret.bme68x_status);
            return (int)ret.bme68x_status;
        }
        else if (ret.bsec_status != BSEC_OK)
        {
            /* Could not intialize BSEC library */
            if (ret.bsec_status == BSEC_W_SU_SAMPLERATEMISMATCH)
            {
                /* Handle here the error */
                spdlog::error("[AirQualityService] The sample rate doesn't match the config. The SAMPLE_RATE is defined in bsec_integration.h file.");
            }
            spdlog::error("[AirQualityService] Could not intialize BSEC library -> {}", ret.bsec_status);
            return (int)ret.bsec_status;
        }
    }

    spdlog::info("[AirQualityService] Starting air monitoring");

    /* Call to endless loop function which reads and processes data based on sensor settings */
    /* State is saved every 10.000 samples, which means every 10.000 * 3 secs = 500 minutes  */
    bsec_iot_loop(BSecProxy::bsec_sleep_n, BSecProxy::bsec_get_timestamp_us, BSecProxy::bsec_output_ready, BSecProxy::bsec_state_save, 10000);

    spdlog::info("[AirQualityService] Air monitoring stopped!");

    return 0;
}

void AirQualityService::setOnAirQualityChange(std::function<void(AirQuality)> onQualityChange) {
    this->onAirQualityChange = onQualityChange;
}

void AirQualityService::outputReady(AirQuality output) {
    onAirQualityChange(output);
}
    
int8_t AirQualityService::readI2CRegister(uint8_t reg_addr, uint8_t *reg_data_ptr, uint32_t data_len) {
    if (!i2c_bus.isOpened()) {
        return -1;
    }
    return i2c_bus.readData(reg_addr, reg_data_ptr, data_len);
}

int8_t AirQualityService::writeI2CRegister(uint8_t reg_addr, const uint8_t *reg_data_ptr, uint32_t data_len) {
    if (!i2c_bus.isOpened()) {
        return -1;
    }
    return i2c_bus.writeData(reg_addr, reg_data_ptr, data_len);
}