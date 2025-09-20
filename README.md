# rpi-iaq-monitor

Simple C++ application to monitor IAQ (Indoor Air Quality) using a Raspberry Pi and a BME680/BME688 sensor accessed via the I2C bus.

The values are published to a given instance of HomeBridge (can be easily disabled in `main.cpp`), the URL of the HomeBridge instance needs to be set in `src/constants.h`.

## Dependencies

### System Dependencies
```bash
sudo apt update
sudo apt install build-essential cmake libssl-dev libspdlog-dev libi2c-dev
```

### BSEC Library (Required)

⚠️ **Important**: This project requires the proprietary **BSEC (Bosch Sensortec Environmental Cluster)** library, which is **NOT included** in this repository due to licensing restrictions.

**📖 [Complete BSEC Setup Guide →](docs/DEPENDENCIES_BSEC.md)**

**Quick Setup:**

1. Download BSEC from [Bosch Sensortec](https://developer.bosch-sensortec.com/) (search: "BSEC Software Library")
2. Accept the license and extract the archive
3. Set environment variable: `export BSEC_DIR=/path/to/extracted/BSEC`
4. Validate installation: `./scripts/require_bsec.sh`

## Hardware Setup

### Enable I2C on Raspberry Pi

```bash
sudo raspi-config
```

Select `3 Interface Options` → `I5 I2C` → `Yes` to enable I2C.

### Sensor Connection

Connect your BME680/BME688 sensor to the Raspberry Pi I2C bus:
- VCC → 3.3V
- GND → Ground  
- SCL → GPIO 3 (Pin 5)
- SDA → GPIO 2 (Pin 3)

## Building

### Prerequisites Check

First, validate your BSEC installation:

```bash
./scripts/require_bsec.sh
```

### Build Commands

```bash
# Method 1: Using environment variable
export BSEC_DIR=/path/to/your/BSEC
mkdir build && cd build
cmake ..
make

# Method 2: Specifying BSEC_DIR directly to CMake  
mkdir build && cd build
cmake -DBSEC_DIR=/path/to/your/BSEC ..
make
```

### Build Optimization

For release builds with full optimization:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## Running

```bash
cd build
./air-quality-monitor
```

**Note**: It takes time for the IAQ accuracy to improve. The sensor needs to establish a baseline for gas resistance measurements.

## Configuration

### HomeBridge Integration

Edit `src/constants.h` to configure HomeBridge URL and other settings.

### Sensor Configuration

The project uses `BSEC_SAMPLE_RATE_LP` (Low Power mode) by default. You can modify this in the source code if needed.

## Troubleshooting

### Build Issues

1. **"BSEC library not found"**: 
   - Run `./scripts/require_bsec.sh` to diagnose BSEC installation
   - Ensure `BSEC_DIR` points to correct directory
   - See [BSEC Setup Guide](docs/DEPENDENCIES_BSEC.md)

2. **I2C permission errors**:
   ```bash
   sudo usermod -a -G i2c $USER
   # Log out and back in
   ```

3. **Sensor not detected**:
   ```bash
   i2cdetect -y 1
   # Should show device at address 0x77 (BME680) or 0x76
   ```

### Runtime Issues

- **IAQ accuracy is 0**: This is normal initially. The sensor needs time to calibrate.
- **Values seem incorrect**: Ensure proper sensor mounting and ventilation.

## Development

### Debug Build

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
gdb ./air-quality-monitor
```

### Architecture Optimization

The build system automatically optimizes for your Raspberry Pi model:
- Pi 4/5 (ARM64): Uses Cortex-A72 optimizations
- Pi 3 (ARM32): Uses Cortex-A53 optimizations

## Legal and Licensing

- **BSEC Library**: Proprietary software from Bosch Sensortec. Users must download and accept Bosch's license terms.
- **Project Code**: [Your chosen license - specify here]

⚠️ **Do not redistribute BSEC library files**. Each user must download BSEC themselves from Bosch Sensortec.

## Support

- **BSEC-related issues**: Contact Bosch Sensortec
- **Integration/build issues**: Open an issue in this repository
- **Hardware issues**: Check connections and I2C configuration

## Version Compatibility

- **BSEC**: v2.x recommended
- **Sensors**: BME680, BME688  
- **Platform**: Raspberry Pi 3/4/5
- **OS**: Raspberry Pi OS (Debian-based)
