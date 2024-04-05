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

#include "simple_i2c_bus.h"
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

extern "C"
{
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}

SimpleI2CBus::SimpleI2CBus() {
    spdlog::debug("[SimpleI2CBus] init");
    device = "";
    slaveAddress = 0;
    busfd = -1;
}

SimpleI2CBus::~SimpleI2CBus() {
    spdlog::debug("[SimpleI2CBus] deinit");
    closeI2CBus();
}

bool SimpleI2CBus::isOpened() {
    return busfd != -1;
}

int SimpleI2CBus::openI2CBus(std::string device, uint8_t slaveAddress) {
    spdlog::debug("[SimpleI2CBus] openI2CBus: device={}, slaveAddress={}", device, slaveAddress);
    // Open the I2C bus
    int busfd = 0;
    if ((busfd = open(device.c_str(), O_RDWR)) < 0) {
        spdlog::error("[SimpleI2CBus] Failed to open the i2c bus");
        return -1;
    }

    // Specify the I2C slave address using an ioctl call
    if (ioctl(busfd, I2C_SLAVE, slaveAddress) < 0) {
        spdlog::error("[SimpleI2CBus] Failed to acquire bus access or talk to slave");
        close(busfd);
        return -1;
    }

    this->device = device;
    this->slaveAddress = slaveAddress;
    this->busfd = busfd;
    spdlog::info("[SimpleI2CBus] I2C bus opened");

    return busfd;
}

void SimpleI2CBus::closeI2CBus() {
    close(busfd);
    busfd = -1;
}

int SimpleI2CBus::writeData(uint8_t reg_addr, const uint8_t *reg_data_ptr, uint32_t data_len) {
    if (busfd < 0) {
        spdlog::error("[SimpleI2CBus] Failed to write to the i2c bus: bus not open");
        return -1;
    }

    if (data_len + 1 > I2C_BUS_MAX_BUFFER_SIZE) {
        spdlog::error("[SimpleI2CBus] Failed to write to the i2c bus: buffer not big enough for data len: {}", data_len);
        closeI2CBus();
        return -1;
    }

    // We need to write the register address first
    uint8_t buffer[I2C_BUS_MAX_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    buffer[0] = reg_addr;
    memcpy(buffer + 1, reg_data_ptr, data_len);

    int ret;
    ret = write(busfd, buffer, data_len + 1);
    if (ret < 0) {
        spdlog::error("[SimpleI2CBus] Failed to write to the i2c bus");
        closeI2CBus();
    }    

    return ret;
}

int SimpleI2CBus::readData(uint8_t reg_addr, uint8_t *reg_data_ptr, uint32_t data_len) {
	int ret;

    // Select the register to read from by writing its address
    ret = i2c_smbus_write_byte(busfd, reg_addr);
    if (ret < 0) {
        spdlog::error("[SimpleI2CBus] Failed to select register");
        closeI2CBus();
        return ret;
    }

    ret = read(busfd, reg_data_ptr, data_len);
    if (ret < 0) {
        spdlog::error("[SimpleI2CBus] Failed to read from the i2c bus");        
        closeI2CBus();
        return ret;
    }

    return ret;
}
