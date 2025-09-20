/*
 * High-precision timing utilities for BSEC integration
 * Addresses timing violations that cause "WARNING: bsec_sensor_control: 100"
 */

#pragma once

#include <cstdint>
#include <time.h>
#include <errno.h>

#ifdef ENABLE_TIMING_DEBUG
#include <spdlog/spdlog.h>
#define TIMING_DEBUG(...) do { \
    static int debug_counter = 0; \
    if (++debug_counter % 30 == 1) { \
        spdlog::debug(__VA_ARGS__); \
    } \
} while(0)
#else
#define TIMING_DEBUG(...) do { } while(0)
#endif

namespace PrecisionTiming {

/**
 * Get current time in nanoseconds using monotonic clock
 * This clock is immune to system time changes and NTP adjustments
 */
inline int64_t now_ns() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
        // Fallback to regular monotonic clock if RAW not available
        clock_gettime(CLOCK_MONOTONIC, &ts);
    }
    return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

/**
 * Get current time in microseconds for BSEC compatibility
 */
inline int64_t now_us() {
    return now_ns() / 1000LL;
}

/**
 * Sleep until an absolute time point (nanoseconds)
 * More precise than relative sleep_for()
 */
inline void sleep_until_ns(int64_t target_ns) {
    struct timespec target_ts;
    target_ts.tv_sec = target_ns / 1000000000LL;
    target_ts.tv_nsec = target_ns % 1000000000LL;
    
    int result;
    do {
        result = clock_nanosleep(CLOCK_MONOTONIC_RAW, TIMER_ABSTIME, &target_ts, nullptr);
        if (result != 0 && result != EINTR) {
            // Fallback to regular monotonic clock if RAW not available
            result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target_ts, nullptr);
        }
    } while (result == EINTR);
    
    // If we're already past the target time, return immediately
}

/**
 * Sleep until an absolute time point (microseconds)
 * For compatibility with BSEC's microsecond-based interface
 */
inline void sleep_until_us(int64_t target_us) {
    sleep_until_ns(target_us * 1000LL);
}

/**
 * Check if we're running late compared to expected schedule
 * Returns delay in microseconds (positive = late, negative = early)
 */
inline int64_t calculate_delay_us(int64_t now_us, int64_t expected_us) {
    return now_us - expected_us;
}

/**
 * Log timing violation if delay exceeds threshold
 */
inline void check_timing_violation(int64_t now_us, int64_t expected_us, int64_t threshold_us = 1000) {
    int64_t delay = calculate_delay_us(now_us, expected_us);
    if (delay > threshold_us) {
        TIMING_DEBUG("TIMING VIOLATION: {} us late (expected: {}, actual: {})", 
                    delay, expected_us, now_us);
    }
}

} // namespace PrecisionTiming
