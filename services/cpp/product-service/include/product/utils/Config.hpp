#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace product::utils {

/**
 * @brief Configuration management
 * 
 * Loads configuration from application.json and environment variables
 */
class Config {
public:
    static void load(const std::string& configFile = "config/application.json");
    
    static std::string getString(const std::string& key, const std::string& defaultValue = "");
    static int getInt(const std::string& key, int defaultValue = 0);
    static bool getBool(const std::string& key, bool defaultValue = false);
    
    static std::string getEnv(const std::string& envVar, const std::string& defaultValue = "");

private:
    static json config_;
    static bool loaded_;

    static std::optional<json> getJsonByPath(const std::string& path);
};

}  // namespace product::utils
