#!/bin/bash

# Fix BSEC timing issues for RPi IAQ Monitor
# This script addresses the "WARNING: bsec_sensor_control: 100" timing violations

echo "Applying BSEC timing fixes for Raspberry Pi..."

# 1. Set CPU governor to performance mode to reduce timing jitter
if [ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]; then
    current_governor=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)
    echo "Current CPU governor: $current_governor"
    if [ "$current_governor" != "performance" ]; then
        echo "Setting CPU governor to performance mode..."
        echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor > /dev/null
        echo "CPU governor set to performance"
    fi
else
    echo "CPU frequency scaling not available"
fi

# 2. Check and set real-time priority capabilities
echo "Checking real-time priority capabilities..."
if [ -x "$(command -v setcap)" ]; then
    echo "Setting real-time priority capabilities for air-quality-monitor..."
    sudo setcap cap_sys_nice+ep build/air-quality-monitor
    echo "Real-time capabilities set"
else
    echo "WARNING: setcap not available - timing precision may be reduced"
fi

# 3. Create startup script with proper priority
cat > start_iaq_fixed.sh << 'STARTUP_EOF'
#!/bin/bash
cd "$(dirname "$0")"

# Create logs directory if it doesn't exist
mkdir -p logs

# Set high priority for the process
nice -n -10 ./build/air-quality-monitor 2>&1 | tee -a logs/$(date +%Y%m%d_%H%M%S).log
STARTUP_EOF

chmod +x start_iaq_fixed.sh

echo "BSEC timing fixes applied!"
echo "Usage:"
echo "  - Run directly: ./build/air-quality-monitor"
echo "  - Run with fixes: ./start_iaq_fixed.sh"
echo "  - For best results, run as root or with sudo for real-time scheduling"
