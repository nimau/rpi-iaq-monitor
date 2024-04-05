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

#ifndef SIMPLE_I2C_BUS_H_
#define SIMPLE_I2C_BUS_H_

#include <cstdint>
#include <string>

#define I2C_BUS_MAX_BUFFER_SIZE 64

/*
    Simple class to read and write data to an I2C device on a RPI
*/

class SimpleI2CBus {
private:
    std::string device;
    uint8_t slaveAddress;
    int busfd;

public:
    SimpleI2CBus();
    ~SimpleI2CBus();

    /// @brief Open a file descriptor to an I2C bus
    /// @param device the device to open (something like "/dev/i2c-1")
    /// @param slaveAddress the I2C slave address (something like 0x76 or 0x77)
    /// @return the file descriptor or -1 if an error occurred
    int openI2CBus(std::string device, uint8_t slaveAddress);

    /// @brief Close the file descriptor to the I2C bus
    void closeI2CBus();

    /// @brief Write data to an I2C device
    /// @param reg_addr the register address to write to
    /// @param reg_data_ptr the data to write
    /// @param data_len the length of the data to write
    int writeData(uint8_t reg_addr, const uint8_t *reg_data_ptr, uint32_t data_len);

    /// @brief Read data from an I2C device
    /// @param reg_addr the register address to read from
    /// @param reg_data_ptr the buffer to store the data
    /// @param data_len the length of the data to read
    int readData(uint8_t reg_addr, uint8_t *reg_data_ptr, uint32_t data_len);

    /// @brief Check if the I2C bus is opened
    bool isOpened();
};

#endif // SIMPLE_I2C_BUS_H_
