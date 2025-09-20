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

#include "ConfigManager.h"
#include "yaml-cpp/yaml.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

std::string Config::getSavedStatePath() const {
    return (fs::path(iaq_saved_state_dir) / iaq_saved_state_file).string();
}

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

Config ConfigManager::getDefaultConfig() {
    Config defaults;
    defaults.homebridge_url = "";  // Empty string disables Homebridge publishing
    defaults.homebridge_publish_interval_seconds = 15;
    defaults.iaq_temp_offset = 9.0f;
    defaults.iaq_i2c_bus_device = "/dev/i2c-1";
    defaults.iaq_saved_state_dir = "./saved_state";
    defaults.iaq_saved_state_file = "bsec_state_file";
    return defaults;
}

bool ConfigManager::load(const std::string& configPath) {
    spdlog::info("Loading configuration from: {}", configPath);
    effectiveConfigPath_ = configPath;
    
    // Start with default configuration
    config_ = getDefaultConfig();
    
    try {
        if (!fs::exists(configPath)) {
            spdlog::info("Configuration file does not exist, creating default: {}", configPath);
            if (!writeDefaultConfig(configPath)) {
                spdlog::warn("Failed to create default config file, using in-memory defaults");
            }
            ensureSavedStateDirectory();
            return true;
        }
        
        YAML::Node root = YAML::LoadFile(configPath);
        if (!parseConfigFromNode(root)) {
            spdlog::error("Failed to parse configuration, using defaults");
        }
        
        ensureSavedStateDirectory();
        return true;
        
    } catch (const YAML::Exception& e) {
        spdlog::error("YAML parsing error in {}: {}", configPath, e.what());
        spdlog::info("Using default configuration values");
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Error loading config file {}: {}", configPath, e.what());
        spdlog::info("Using default configuration values");
        return false;
    }
}

std::string ConfigManager::getStringOrDefault(const YAML::Node& node, const std::string& key, const std::string& defaultValue) {
    try {
        if (node[key]) {
            return node[key].as<std::string>();
        } else {
            spdlog::warn("Configuration key '{}' not found, using default: {}", key, defaultValue);
            return defaultValue;
        }
    } catch (const YAML::Exception& e) {
        spdlog::warn("Configuration key '{}' has invalid value, using default: {} (error: {})", 
                     key, defaultValue, e.what());
        return defaultValue;
    }
}

int ConfigManager::getIntOrDefault(const YAML::Node& node, const std::string& key, const int& defaultValue) {
    try {
        if (node[key]) {
            return node[key].as<int>();
        } else {
            spdlog::warn("Configuration key '{}' not found, using default: {}", key, defaultValue);
            return defaultValue;
        }
    } catch (const YAML::Exception& e) {
        spdlog::warn("Configuration key '{}' has invalid value, using default: {} (error: {})", 
                     key, defaultValue, e.what());
        return defaultValue;
    }
}

float ConfigManager::getFloatOrDefault(const YAML::Node& node, const std::string& key, const float& defaultValue) {
    try {
        if (node[key]) {
            return node[key].as<float>();
        } else {
            spdlog::warn("Configuration key '{}' not found, using default: {}", key, defaultValue);
            return defaultValue;
        }
    } catch (const YAML::Exception& e) {
        spdlog::warn("Configuration key '{}' has invalid value, using default: {} (error: {})", 
                     key, defaultValue, e.what());
        return defaultValue;
    }
}

bool ConfigManager::parseConfigFromNode(const YAML::Node& node) {
    try {
        Config defaults = getDefaultConfig();
        
        config_.homebridge_url = getStringOrDefault(node, "homebridge_url", defaults.homebridge_url);
        config_.homebridge_publish_interval_seconds = getIntOrDefault(node, "homebridge_publish_interval_seconds", defaults.homebridge_publish_interval_seconds);
        config_.iaq_temp_offset = getFloatOrDefault(node, "iaq_temp_offset", defaults.iaq_temp_offset);
        config_.iaq_i2c_bus_device = getStringOrDefault(node, "iaq_i2c_bus_device", defaults.iaq_i2c_bus_device);
        config_.iaq_saved_state_dir = getStringOrDefault(node, "iaq_saved_state_dir", defaults.iaq_saved_state_dir);
        config_.iaq_saved_state_file = getStringOrDefault(node, "iaq_saved_state_file", defaults.iaq_saved_state_file);
        
        // Validation
        if (config_.homebridge_publish_interval_seconds <= 0) {
            spdlog::warn("Invalid homebridge_publish_interval_seconds: {}, using default: {}", 
                         config_.homebridge_publish_interval_seconds, defaults.homebridge_publish_interval_seconds);
            config_.homebridge_publish_interval_seconds = defaults.homebridge_publish_interval_seconds;
        }
        
        // Log effective configuration
        spdlog::info("Configuration loaded successfully:");
        spdlog::info("  homebridge_url: {}", config_.homebridge_url.empty() ? "[disabled]" : config_.homebridge_url);
        spdlog::info("  homebridge_publish_interval_seconds: {}", config_.homebridge_publish_interval_seconds);
        spdlog::info("  iaq_temp_offset: {}", config_.iaq_temp_offset);
        spdlog::info("  iaq_i2c_bus_device: {}", config_.iaq_i2c_bus_device);
        spdlog::info("  iaq_saved_state_dir: {}", config_.iaq_saved_state_dir);
        spdlog::info("  iaq_saved_state_file: {}", config_.iaq_saved_state_file);
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error parsing configuration: {}", e.what());
        return false;
    }
}

void ConfigManager::ensureSavedStateDirectory() {
    try {
        if (!fs::exists(config_.iaq_saved_state_dir)) {
            fs::create_directories(config_.iaq_saved_state_dir);
            spdlog::info("Created saved state directory: {}", config_.iaq_saved_state_dir);
        }
    } catch (const fs::filesystem_error& e) {
        spdlog::warn("Failed to create saved state directory {}: {}", config_.iaq_saved_state_dir, e.what());
    }
}

bool ConfigManager::writeDefaultConfig(const std::string& configPath) {
    try {
        if (fs::exists(configPath)) {
            spdlog::info("Configuration file already exists, not overwriting: {}", configPath);
            return true;
        }
        
        // Create directory if needed
        fs::path configDir = fs::path(configPath).parent_path();
        if (!configDir.empty() && !fs::exists(configDir)) {
            fs::create_directories(configDir);
        }
        
        std::ofstream file(configPath);
        if (!file.is_open()) {
            spdlog::error("Failed to create config file: {}", configPath);
            return false;
        }
        
        file << "# RPi IAQ Monitor Configuration\n";
        file << "# This file is automatically created with default values\n";
        file << "# Modify values as needed and restart the application\n\n";
        
        file << "# Homebridge URL for publishing sensor data\n";
        file << "# Leave empty to disable Homebridge integration\n";
        file << "# Example: \"http://192.168.1.100:51828\"\n";
        file << "homebridge_url: \"\"\n\n";
        
        file << "# Interval in seconds for publishing data to Homebridge\n";
        file << "homebridge_publish_interval_seconds: 15\n\n";
        
        file << "# Temperature offset in Celsius to compensate for sensor placement\n";
        file << "# This accounts for heat from the Raspberry Pi\n";
        file << "iaq_temp_offset: 9.0\n\n";
        
        file << "# I2C bus device path for the BME68x sensor\n";
        file << "iaq_i2c_bus_device: \"/dev/i2c-1\"\n\n";
        
        file << "# Directory to store BSEC algorithm state (for calibration persistence)\n";
        file << "iaq_saved_state_dir: \"./saved_state\"\n\n";
        
        file << "# Filename for BSEC state file\n";
        file << "iaq_saved_state_file: \"bsec_state_file\"\n";
        
        file.close();
        spdlog::info("Created default configuration file: {}", configPath);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Error writing default config file {}: {}", configPath, e.what());
        return false;
    }
}

bool ConfigManager::ensureDefaultConfigExists(const std::string& configPath) {
    if (!fs::exists(configPath)) {
        return writeDefaultConfig(configPath);
    }
    return true;
}
