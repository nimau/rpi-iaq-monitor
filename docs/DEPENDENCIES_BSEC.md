# BSEC (Bosch Sensortec Environmental Cluster) Setup Guide

## Overview

**BSEC** is a proprietary software library from Bosch Sensortec that provides advanced processing for environmental sensor data from BME680/BME688 sensors. This library is **NOT included** in this repository due to licensing restrictions.

## Why BSEC?

BSEC enables:
- **Indoor Air Quality (IAQ) index** calculation
- **Gas resistance baseline** establishment 
- **CO2 equivalent estimation**
- **Breath VOC equivalent estimation**
- Advanced sensor fusion and calibration algorithms

## Download and Installation

### Step 1: Download BSEC

1. Visit the [Bosch Sensortec Developer Portal](https://developer.bosch-sensortec.com/)
2. Search for "BSEC Software Library" or "BME680 BSEC"
3. **Accept the license agreement** (required)
4. Download the appropriate package for your platform:
   - For Raspberry Pi: Choose Linux ARM version
   - For development: Choose the version matching your host system

### Step 2: Extract BSEC

```bash
# Example for a typical download
unzip BSEC_*.zip
cd BSEC_*

# The directory should contain:
# - inc/ or include/ (header files)
# - lib/ (library binaries) 
# - src/ (source files)
```

### Step 3: Set Environment Variable

Set the `BSEC_DIR` environment variable to point to your extracted BSEC directory:

```bash
export BSEC_DIR=/path/to/your/extracted/BSEC

# Make it persistent by adding to your shell profile:
echo 'export BSEC_DIR=/path/to/your/extracted/BSEC' >> ~/.bashrc
```

### Step 4: Validate Installation

Use the provided validation script:

```bash
./scripts/require_bsec.sh
```

This script will check that all required BSEC files are present and accessible.

## Build Instructions

### Using CMake (Recommended)

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

### Troubleshooting Build Issues

If you encounter build errors:

1. **Verify BSEC installation**:
   ```bash
   ./scripts/require_bsec.sh
   ```

2. **Check CMake output** for BSEC detection messages:
   ```
   -- BSEC Found:
   --   BSEC_DIR: /path/to/BSEC
   --   BSEC_INCLUDE_DIR: /path/to/BSEC/inc
   --   BSEC_LIBRARY: /path/to/BSEC/lib/libalgobsec.a
   ```

3. **Clear build cache** if changing BSEC_DIR:
   ```bash
   rm -rf build/
   mkdir build && cd build
   cmake -DBSEC_DIR=/new/path/to/BSEC ..
   ```

## BSEC Library Structure

A valid BSEC installation should contain:

```
BSEC_DIR/
├── inc/ (or include/ or src/)
│   ├── bsec_interface.h          # Main BSEC API
│   ├── bsec_datatypes.h          # Data structures
│   ├── bme68x.h                  # BME68x driver API  
│   └── bme68x_defs.h            # BME68x definitions
├── lib/ (or binaries/)
│   └── libalgobsec.a            # Compiled BSEC library
└── src/
    ├── bme68x.c                 # BME68x driver source
    └── bsec_integration.c       # Integration layer
```

## Platform-Specific Notes

### Raspberry Pi

- Use the **Linux ARM** version of BSEC
- For Pi 4/5 (ARM64): Choose ARM64/AArch64 binaries if available
- For Pi 3 (ARM32): Choose ARM32/ARMv7 binaries

### Cross-Compilation

When cross-compiling, ensure:
- BSEC binaries match your target architecture
- Set `BSEC_DIR` to point to target-appropriate BSEC installation
- Use appropriate toolchain files

## CI/CD Integration

### GitHub Actions Example

```yaml
- name: Setup BSEC
  run: |
    # Download BSEC (requires storing credentials securely)
    # Extract to known location
    export BSEC_DIR=/opt/bsec
    echo "BSEC_DIR=/opt/bsec" >> $GITHUB_ENV
    
- name: Validate BSEC
  run: ./scripts/require_bsec.sh
  
- name: Build
  run: |
    mkdir build && cd build
    cmake -DBSEC_DIR=$BSEC_DIR ..
    make
```

### Security Notes for CI

- **Never commit BSEC files** to your repository
- Store BSEC archive in **secure CI storage** (encrypted secrets, private artifacts)
- Download/extract BSEC during CI build process
- Consider caching extracted BSEC to speed up builds

## Legal and Licensing

- BSEC is **proprietary software** owned by Bosch Sensortec
- You must **accept Bosch's license** to use BSEC
- **Do not redistribute** BSEC files
- Each user/developer must download BSEC themselves
- Respect all terms and conditions from Bosch Sensortec

## Common Issues

### "BSEC library not found" Error

```
CMake Error: BSEC library not found. Please download the Bosch BSEC library and set BSEC_DIR.
```

**Solutions:**
1. Ensure `BSEC_DIR` is set correctly
2. Verify BSEC directory structure matches expected layout
3. Run `./scripts/require_bsec.sh` to diagnose issues

### Architecture Mismatch

If you get linking errors about architecture:
- Ensure BSEC binaries match your target platform
- For Raspberry Pi, use ARM versions, not x86
- Check that library format matches (static vs shared)

### Missing Source Files

If build fails finding `bme68x.c` or `bsec_integration.c`:
- These should be included in your BSEC download
- Check in `src/` directory of BSEC package
- Some BSEC packages may have different directory structures

## Support

- **BSEC issues**: Contact Bosch Sensortec support
- **Integration issues**: Check project documentation or open an issue
- **License questions**: Refer to Bosch Sensortec licensing terms

## Version Compatibility

This project has been tested with:
- BSEC v2.x (latest recommended)
- BME680/BME688 sensors
- ARM-based platforms (Raspberry Pi)

Always use the latest stable version of BSEC from Bosch Sensortec.
