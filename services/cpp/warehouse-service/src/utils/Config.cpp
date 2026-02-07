#include "warehouse/utils/Config.hpp"
#include "warehouse/utils/Logger.hpp"
#include <fstream>
#include <cstdlib>

namespace warehouse::utils {

Config& Config::instance() {
    static Config config;
    return config;
}

bool Config::load(const std::string& configFile) {
    try {
        std::ifstream file(configFile);
        if (!file.is_open()) {
            return false;
        }
        
        config_ = json::parse(file);
        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to load config: {}", e.what());
        return false;
    }
}

bool Config::loadFromJson(const json& config) {
    config_ = config;
    return true;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
    try {
        auto value = getNestedValue(key);
        if (value.is_string()) {
            return value.get<std::string>();
        }
    } catch (...) {
    }
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) const {
    try {
        auto value = getNestedValue(key);
        if (value.is_number_integer()) {
            return value.get<int>();
        }
    } catch (...) {
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    try {
        auto value = getNestedValue(key);
        if (value.is_boolean()) {
            return value.get<bool>();
        }
    } catch (...) {
    }
    return defaultValue;
}

double Config::getDouble(const std::string& key, double defaultValue) const {
    try {
        auto value = getNestedValue(key);
        if (value.is_number()) {
            return value.get<double>();
        }
    } catch (...) {
    }
    return defaultValue;
}

std::optional<json> Config::getJson(const std::string& key) const {
    try {
        return getNestedValue(key);
    } catch (...) {
        return std::nullopt;
    }
}

void Config::set(const std::string& key, const std::string& value) {
    config_[key] = value;
}

void Config::set(const std::string& key, int value) {
    config_[key] = value;
}

void Config::set(const std::string& key, bool value) {
    config_[key] = value;
}

void Config::set(const std::string& key, double value) {
    config_[key] = value;
}

void Config::setFromEnv(const std::string& key, const std::string& envVar) {
    const char* value = std::getenv(envVar.c_str());
    if (value != nullptr) {
        config_[key] = std::string(value);
    }
}

json Config::getNestedValue(const std::string& key) const {
    json current = config_;
    std::istringstream iss(key);
    std::string segment;
    
    while (std::getline(iss, segment, '.')) {
        if (current.contains(segment)) {
            current = current[segment];
        } else {
            throw std::runtime_error("Key not found: " + key);
        }
    }
    
    return current;
}

Config::ServerConfig Config::getServerConfig() const {
    ServerConfig config;
    config.host = getString("server.host", "0.0.0.0");
    config.port = getInt("server.port", 8080);
    config.maxThreads = getInt("server.maxThreads", 10);
    config.maxQueued = getInt("server.maxQueued", 100);
    return config;
}

Config::DatabaseConfig Config::getDatabaseConfig() const {
    DatabaseConfig config;
    config.host = getString("database.host", "localhost");
    config.port = getInt("database.port", 5432);
    config.database = getString("database.database", "warehouse_db");
    config.user = getString("database.user", "warehouse");
    config.password = getString("database.password", "");
    config.maxConnections = getInt("database.maxConnections", 10);
    return config;
}

} // namespace warehouse::utils
