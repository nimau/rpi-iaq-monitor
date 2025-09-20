#!/bin/bash

# Performance monitoring script for IAQ Monitor
echo "=== IAQ Monitor Performance Report ==="
echo "Date: $(date)"
echo

echo "=== System Information ==="
echo "CPU Temperature: $(vcgencmd measure_temp)"
echo "Throttling Status: $(vcgencmd get_throttled)"
echo "CPU Governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
echo

echo "=== I2C Status ==="
echo "I2C devices:"
i2cdetect -y 1
echo

echo "=== Process Status ==="
ps aux | grep air-quality-monitor | grep -v grep
echo

echo "=== Recent BSEC Warnings (last 100 lines) ==="
if [ -f "/home/ubuntu/work/rpi-iaq-monitor/build/logs/log" ]; then
    tail -100 /home/ubuntu/work/rpi-iaq-monitor/build/logs/log | grep -E "(WARNING|ERROR)" | tail -10
    echo "Total warnings in last 100 lines: $(tail -100 /home/ubuntu/work/rpi-iaq-monitor/build/logs/log | grep -c 'bsec_sensor_control.*100')"
else
    echo "Log file not found"
fi
echo

echo "=== System Load ==="
uptime
echo

echo "=== Memory Usage ==="
free -h
