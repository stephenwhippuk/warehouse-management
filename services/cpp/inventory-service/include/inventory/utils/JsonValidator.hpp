#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace inventory {
namespace utils {

class JsonValidator {
public:
    static bool validate(const nlohmann::json& data, const std::string& schemaPath);
    static bool validateInventory(const nlohmann::json& data);
    
private:
    static std::string loadSchema(const std::string& schemaPath);
};

} // namespace utils
} // namespace inventory
