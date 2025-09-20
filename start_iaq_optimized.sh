#!/bin/bash

# Optimize system for IAQ monitoring
echo "Setting CPU governor to performance..."
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor > /dev/null

# Start with real-time priority (requires CAP_SYS_NICE)
echo "Starting IAQ monitor with optimized settings..."
cd /home/ubuntu/work/rpi-iaq-monitor

# Option 1: With real-time priority (if available)
if command -v chrt > /dev/null 2>&1; then
    echo "Using real-time scheduling..."
    sudo chrt -f 25 ./build/air-quality-monitor
else
    # Option 2: With high nice priority
    echo "Using high priority scheduling..."
    sudo nice -10 ./build/air-quality-monitor
fi
