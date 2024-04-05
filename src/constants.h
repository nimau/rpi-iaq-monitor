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

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define HOMEBRIDGE_URL ""                       // Homebridge URL to publish the data. Example: http://192.168.0.1:8581
#define HOMEBRIDGE_PUBLISH_INTERVAL 15          // publish interval in seconds

#define IAQ_SAVED_STATE_DIR "./saved_state"     // directory to save the IAQ state (will be created if it doesn't exist)
#define IAQ_SAVED_STATE_FILE "bsec_state_file"  // file to save the IAQ state (will be created if it doesn't exist)
#define IAQ_I2C_BUS_DEVICE "/dev/i2c-1"         // I2C bus device
#define IAQ_TEMP_OFFSET 9.0f                    // temperature offset in Celsius (depends on the sensor placement and the Raspberry Pi heat)


#endif // CONSTANTS_H_