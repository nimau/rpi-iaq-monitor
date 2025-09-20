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

#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_

#include <string>
#include <memory>

// Forward declaration
namespace YAML {
    class Node;
}

struct Config {
    std::string homebridge_url;
    int homebridge_publish_interval_seconds;
    float iaq_temp_offset;
    std::string iaq_i2c_bus_device;
    std::string iaq_saved_state_dir;
    std::string iaq_saved_state_file;
    
    // Helper method to get full saved state path
    std::string getSavedStatePath() const;
};

class ConfigManager {
public:
    // Singleton access
    static ConfigManager& instance();
    
    // Configuration loading
    bool load(const std::string& configPath);
    
    // Configuration access
    const Config& get() const { return config_; }
    
    // Path information
    const std::string& effectiveConfigPath() const { return effectiveConfigPath_; }
    
    // Configuration file management
    static bool writeDefaultConfig(const std::string& configPath);
    bool ensureDefaultConfigExists(const std::string& configPath);
    
    // Get default configuration values
    static Config getDefaultConfig();
    
private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Helper methods - implementations in .cpp file
    std::string getStringOrDefault(const YAML::Node& node, const std::string& key, const std::string& defaultValue);
    int getIntOrDefault(const YAML::Node& node, const std::string& key, const int& defaultValue);
    float getFloatOrDefault(const YAML::Node& node, const std::string& key, const float& defaultValue);
    
    bool parseConfigFromNode(const YAML::Node& node);
    void ensureSavedStateDirectory();
    
    Config config_;
    std::string effectiveConfigPath_;
};

#endif // CONFIG_MANAGER_H_
