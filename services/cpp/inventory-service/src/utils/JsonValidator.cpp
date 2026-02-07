#include "inventory/utils/JsonValidator.hpp"
#include <fstream>
#include <sstream>

namespace inventory {
namespace utils {

bool JsonValidator::validate(const nlohmann::json& data, const std::string& schemaPath) {
    // TODO: Implement JSON Schema validation
    // Use nlohmann/json-schema-validator or similar
    return true;
}

bool JsonValidator::validateInventory(const nlohmann::json& data) {
    // TODO: Load and validate against inventory.schema.json
    return validate(data, "../../contracts/schemas/v1/inventory.schema.json");
}

std::string JsonValidator::loadSchema(const std::string& schemaPath) {
    std::ifstream file(schemaPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open schema file: " + schemaPath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace utils
} // namespace inventory
