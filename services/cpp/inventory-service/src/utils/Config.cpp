#include "inventory/utils/Config.hpp"
#include <fstream>
#include <cstdlib>

namespace inventory {
namespace utils {

nlohmann::json Config::config_;
std::map<std::string, std::string> Config::env_;

void Config::load(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + configPath);
    }
    
    file >> config_;
}

nlohmann::json Config::get(const std::string& key) {
    if (config_.contains(key)) {
        return config_[key];
    }
    return nlohmann::json();
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    // Check environment variable first
    std::string envValue = getEnv(key);
    if (!envValue.empty()) {
        return envValue;
    }
    
    // Check config file
    if (config_.contains(key) && config_[key].is_string()) {
        return config_[key].get<std::string>();
    }
    
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) {
    if (config_.contains(key) && config_[key].is_number()) {
        return config_[key].get<int>();
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    if (config_.contains(key) && config_[key].is_boolean()) {
        return config_[key].get<bool>();
    }
    return defaultValue;
}

std::string Config::getEnv(const std::string& key, const std::string& defaultValue) {
    const char* val = std::getenv(key.c_str());
    return val ? std::string(val) : defaultValue;
}

} // namespace utils
} // namespace inventory
