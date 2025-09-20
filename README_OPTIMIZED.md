# RPi IAQ Monitor - Optimized Version

High-performance Indoor Air Quality monitor using Raspberry Pi and BME68x sensor with **zero BSEC timing violations**.

## ✅ Performance Highlights

- **🎯 Zero timing warnings**: Eliminated `WARNING: bsec_sensor_control: 100` errors
- **⚡ Optimized builds**: ARM64-specific optimizations with LTO
- **📊 Stable measurements**: Consistent 2-second IAQ readings
- **🔧 Debug instrumentation**: Built-in timing violation detection
- **⚙️ Production ready**: Systemd service with real-time scheduling

## Quick Start

### Production Use
```bash
# Build optimized version
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run optimized binary
./air-quality-monitor

# Or install as system service
sudo systemctl enable iaq-monitor.service
sudo systemctl start iaq-monitor.service
```

### Development/Debugging
```bash
# Build with timing instrumentation
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run with debug output
./air-quality-monitor  # Shows timing debug info
```

## Architecture & Optimizations

### High-Precision Timing System
- **Monotonic clock**: Immune to system time changes
- **Absolute time sleeping**: Precise timing regardless of interrupts  
- **BSEC-compliant scheduling**: Meets strict timing requirements

### System-Level Optimizations
- **CPU governor**: Performance mode for consistent timing
- **I2C bus**: 400kHz for faster sensor communication
- **Real-time priority**: SCHED_FIFO scheduling for guaranteed execution

### Build Optimizations
- **Architecture-specific**: ARM64 Cortex-A72 optimizations
- **Link-time optimization**: -O3 -flto for maximum performance
- **Stripped binaries**: Reduced size for production deployment

## File Structure

```
├── src/
│   ├── precision_timing.h          # High-precision timing utilities
│   ├── air_quality_service.cpp     # Optimized BSEC integration
│   └── ...
├── build/                          # Release build (optimized)
├── build-debug/                    # Debug build (with instrumentation)
├── start_iaq_optimized.sh         # Optimized startup script
├── monitor_performance.sh         # Performance monitoring
└── PERFORMANCE_OPTIMIZATION_REPORT.md  # Detailed optimization report
```

## Configuration

### System Configuration
The optimization automatically configures:
- CPU governor: `performance`
- I2C speed: 400kHz (in `/boot/firmware/config.txt`)
- Process scheduling: Real-time FIFO priority

### Build Configuration
- **Release**: `-O3 -flto -mcpu=cortex-a72` (production)
- **Debug**: `-O0 -g -DENABLE_TIMING_DEBUG` (development)

## Monitoring & Maintenance

### Performance Monitoring
```bash
# Check system status
./monitor_performance.sh

# Monitor service logs
journalctl -u iaq-monitor.service -f

# Verify no timing violations
journalctl -u iaq-monitor.service | grep -c "WARNING.*bsec"  # Should be 0
```

### Health Checks
- **CPU temperature**: `vcgencmd measure_temp` (should be <60°C)
- **Throttling status**: `vcgencmd get_throttled` (should be 0x0)
- **I2C devices**: `i2cdetect -y 1` (should show sensor at 0x77)

## Troubleshooting

### Common Issues

**Issue**: Still seeing BSEC warnings after optimization
```bash
# Check CPU governor
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
# Should show "performance"

# Reset if needed
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

**Issue**: I2C communication errors
```bash
# Check I2C speed configuration
grep i2c /boot/firmware/config.txt
# Should show: dtparam=i2c_arm=on,i2c_arm_baudrate=400000

# Verify sensor detection
i2cdetect -y 1
# Should show sensor at address 0x77
```

**Issue**: High CPU usage
```bash
# Check if debug build is running instead of release
ls -la ./build*/air-quality-monitor
# Use the smaller release build in ./build/
```

## Technical Details

### Timing Architecture
```cpp
// Before (problematic):
auto now = std::chrono::system_clock::now();
std::this_thread::sleep_for(std::chrono::microseconds(delay));

// After (optimized):
int64_t now = PrecisionTiming::now_ns();
PrecisionTiming::sleep_until_ns(target_time);
```

### BSEC Integration
- Uses `CLOCK_MONOTONIC_RAW` for immune timing
- Implements absolute-time sleeping with `clock_nanosleep()`
- Rate-limited debug logging to avoid timing perturbation
- Violation detection with configurable thresholds

## Hardware Compatibility

### Tested Platforms
- **Raspberry Pi 4** (ARM64 - Cortex-A72): ✅ Fully optimized
- **Raspberry Pi 3** (ARM32 - Cortex-A53): ✅ Compatible
- **Raspberry Pi 5** (ARM64 - Cortex-A76): ✅ Should work

### Sensor Support  
- **BME680**: ✅ BSEC 1.x integration
- **BME688**: ✅ BSEC 1.x (2.x upgrade recommended)

## Development

### Debug Mode
Enable timing instrumentation:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_TIMING_DEBUG=ON
```

Debug output includes:
- Timestamp precision logging
- Sleep duration verification  
- Timing violation detection
- Rate-limited debug messages (every 30 cycles)

### Contributing
When modifying timing-critical code:
1. Test with both Release and Debug builds
2. Verify zero timing violations: `grep WARNING logs/`
3. Monitor CPU usage and temperature
4. Run for extended periods (>2 hours) for stability testing

## License
GNU General Public License v3.0

## Support
- Check `PERFORMANCE_OPTIMIZATION_REPORT.md` for detailed technical information
- Monitor logs: `journalctl -u iaq-monitor.service`
- Performance issues: Use debug build to identify timing violations
