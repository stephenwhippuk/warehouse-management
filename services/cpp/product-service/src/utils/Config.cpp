#include "product/utils/Config.hpp"
#include <fstream>
#include <cstdlib>

namespace product::utils {

json Config::config_;
bool Config::loaded_ = false;

void Config::load(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + configFile);
    }
    
    file >> config_;
    loaded_ = true;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    auto result = getJsonByPath(key);
    if (result && result->is_string()) {
        return result->get<std::string>();
    }
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) {
    auto result = getJsonByPath(key);
    if (result && result->is_number_integer()) {
        return result->get<int>();
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    auto result = getJsonByPath(key);
    if (result && result->is_boolean()) {
        return result->get<bool>();
    }
    return defaultValue;
}

std::string Config::getEnv(const std::string& envVar, const std::string& defaultValue) {
    const char* value = std::getenv(envVar.c_str());
    return value ? std::string(value) : defaultValue;
}

std::optional<json> Config::getJsonByPath(const std::string& path) {
    if (!loaded_) {
        return std::nullopt;
    }
    
    size_t pos = 0;
    json current = config_;
    
    while (pos < path.length()) {
        size_t dotPos = path.find('.', pos);
        std::string key = path.substr(pos, dotPos == std::string::npos ? std::string::npos : dotPos - pos);
        
        if (!current.contains(key)) {
            return std::nullopt;
        }
        
        current = current[key];
        pos = dotPos == std::string::npos ? path.length() : dotPos + 1;
    }
    
    return current;
}

}  // namespace product::utils
