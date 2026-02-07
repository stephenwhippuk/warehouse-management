#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <map>

namespace inventory {
namespace utils {

class Config {
public:
    static void load(const std::string& configPath);
    static nlohmann::json get(const std::string& key);
    static std::string getString(const std::string& key, const std::string& defaultValue = "");
    static int getInt(const std::string& key, int defaultValue = 0);
    static bool getBool(const std::string& key, bool defaultValue = false);
    
    // Environment variable overrides
    static std::string getEnv(const std::string& key, const std::string& defaultValue = "");
    
private:
    static nlohmann::json config_;
    static std::map<std::string, std::string> env_;
};

} // namespace utils
} // namespace inventory
