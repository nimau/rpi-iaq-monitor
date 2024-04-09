# rpi-iaq-monitor
Simple C++ application to monitor IAQ (Indoor Air Quality) using a RPi4 and a BME68x sensor accessed via the I2C bus.

The values are published to a given instance of HomeBridge (can be easily disabled in `main.cpp`), the URL of the HomeBridge instance needs to be set in `src/constants.h`.

# Dependencies
build-essential, cmake, libssl-dev, libspdlog-dev, libi2c-dev, raspi-config, gdb

# Configuration
* Download the BOSCH software from https://www.bosch-sensortec.com/software-tools/software/bme688-software/

* Copy `libalgobsec.a` to `bsec/lib/`.

* Copy the following source files
```
bme68x_defs.h
bme68x.h
bme68x.c
bsec_datatypes.h`
bsec_integration.h
bsec_integration.c
bsec_interface_multi.h
```
in `bsec/src/`.

* Enable I2C on the RPi with
`sudo raspi-config`.

  To enable I2C select `3 Interface Options` then `I5 I2C`.

* In the `bsec_integration.h` file

  change `SAMPLE_RATE` when `OUTPUT_MODE == IAQ` (line 114) to

  `#define SAMPLE_RATE BSEC_SAMPLE_RATE_LP`.

  to match the configuration used by `air_quality_service.cpp` (feel free to use another one depending on your needs).

# Compilation
```
mkdir build && cd build
cmake ..
cd ..
cmake --build ./build --config release --target all -j 6 --
```

# Run it
```
cd build
./air-quality-monitor
```

**Note: It will take some time for the IAQ accuracy to change.**
