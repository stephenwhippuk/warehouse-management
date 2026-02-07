#pragma once

#include <string>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>

namespace warehouse::utils {

/**
 * @brief Configuration management
 */
class Config {
public:
    static Config& instance();
    
    bool load(const std::string& configFile);
    bool loadFromJson(const nlohmann::json& config);
    
    // Get configuration values
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    double getDouble(const std::string& key, double defaultValue = 0.0) const;
    
    // Get nested values
    std::optional<nlohmann::json> getJson(const std::string& key) const;
    
    // Set configuration values
    void set(const std::string& key, const std::string& value);
    void set(const std::string& key, int value);
    void set(const std::string& key, bool value);
    void set(const std::string& key, double value);
    
    // Environment variable support
    void setFromEnv(const std::string& key, const std::string& envVar);
    
    // Server configuration
    struct ServerConfig {
        std::string host = "0.0.0.0";
        int port = 8080;
        int maxThreads = 10;
        int maxQueued = 100;
    };
    
    ServerConfig getServerConfig() const;
    
    // Database configuration
    struct DatabaseConfig {
        std::string host = "localhost";
        int port = 5432;
        std::string database = "warehouse_db";
        std::string user = "warehouse";
        std::string password;
        int maxConnections = 10;
    };
    
    DatabaseConfig getDatabaseConfig() const;

private:
    Config() = default;
    nlohmann::json config_;
    
    nlohmann::json getNestedValue(const std::string& key) const;
};

} // namespace warehouse::utils
