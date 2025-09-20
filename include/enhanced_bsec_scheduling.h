/*
 * Enhanced BSEC Scheduling Logic
 * Addresses timing violations by implementing proper scheduling around next_call
 */

#pragma once

#include "precision_timing.h"
#include <spdlog/spdlog.h>

namespace EnhancedBSECScheduling {

/**
 * Enhanced BSEC loop with proper scheduling logic
 * Key improvements:
 * 1. Sleep until exact next_call time (not just check if >= next_call)
 * 2. Use monotonic timestamps consistently
 * 3. Detect and log timing violations
 * 4. Prevent timing drift accumulation
 */
class BSECScheduler {
private:
    static constexpr int64_t TIMING_VIOLATION_THRESHOLD_US = 1000; // 1ms tolerance
    static constexpr int64_t MAX_TIMING_DRIFT_US = 10000;          // 10ms max drift
    
    int64_t last_call_time_us = 0;
    int64_t scheduling_start_time_us = 0;
    uint32_t timing_violation_count = 0;
    uint32_t total_cycles = 0;

public:
    /**
     * Wait until the exact BSEC next_call time
     * This replaces the simple >= check with precise absolute timing
     */
    bool waitForNextCall(int64_t next_call_ns, uint8_t sensor_id = 0) {
        int64_t next_call_us = next_call_ns / 1000LL;
        int64_t now_us = PrecisionTiming::now_us();
        
        // Check if we're already late
        int64_t delay_us = now_us - next_call_us;
        if (delay_us > TIMING_VIOLATION_THRESHOLD_US) {
            timing_violation_count++;
            TIMING_DEBUG("Sensor {} TIMING VIOLATION: {}us late (cycle {})", 
                        sensor_id, delay_us, total_cycles);
            
            // If we're extremely late, this indicates a systemic issue
            if (delay_us > MAX_TIMING_DRIFT_US) {
                spdlog::warn("Sensor {} severe timing drift: {}us late, resetting schedule", 
                            sensor_id, delay_us);
                return false; // Signal caller to reset scheduling
            }
            
            return true; // Continue but we're late
        }
        
        // Sleep until the exact next_call time
        PrecisionTiming::sleep_until_us(next_call_us);
        
        // Verify we woke up on time
        int64_t actual_wake_us = PrecisionTiming::now_us();
        PrecisionTiming::check_timing_violation(actual_wake_us, next_call_us, 
                                               TIMING_VIOLATION_THRESHOLD_US);
        
        return true;
    }
    
    /**
     * Get current timestamp for BSEC calls
     * Always uses monotonic clock for consistency
     */
    int64_t getCurrentTimestamp() {
        int64_t now_us = PrecisionTiming::now_us();
        last_call_time_us = now_us;
        total_cycles++;
        
        TIMING_DEBUG("BSEC timestamp: {} us (cycle {})", now_us, total_cycles);
        
        return now_us;
    }
    
    /**
     * Log scheduling statistics periodically
     */
    void logSchedulingStats() {
        if (total_cycles % 100 == 0) { // Every 100 cycles
            float violation_rate = (float)timing_violation_count / total_cycles * 100.0f;
            spdlog::info("BSEC Scheduling Stats: {} cycles, {:.1f}% violations", 
                        total_cycles, violation_rate);
            
            if (violation_rate > 5.0f) { // More than 5% violations
                spdlog::warn("High timing violation rate: {:.1f}% - check system performance", 
                            violation_rate);
            }
        }
    }
    
    /**
     * Reset timing statistics (useful after configuration changes)
     */
    void resetStats() {
        timing_violation_count = 0;
        total_cycles = 0;
        scheduling_start_time_us = PrecisionTiming::now_us();
        spdlog::info("BSEC scheduling statistics reset");
    }
};

} // namespace EnhancedBSECScheduling
