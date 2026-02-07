#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <optional>

#ifdef HAVE_JSON_SCHEMA_VALIDATOR
#include <nlohmann/json-schema.hpp>
#endif

namespace warehouse::utils {

/**
 * @brief JSON Schema validation utility
 */
class JsonValidator {
public:
    JsonValidator();
    
    // Load schema from file or string
    bool loadSchema(const std::string& schemaPath);
    bool loadSchemaFromString(const std::string& schemaJson);
    
    // Validate JSON against loaded schema
    bool validate(const nlohmann::json& data, std::string& errorMessage) const;
    bool validate(const std::string& jsonData, std::string& errorMessage) const;
    
    // Quick validation without error details
    bool isValid(const nlohmann::json& data) const;
    
    // Load contract schemas
    static std::optional<JsonValidator> loadWarehouseSchema();
    static std::optional<JsonValidator> loadLocationSchema();
    static std::optional<JsonValidator> loadCommonSchema();

private:
#ifdef HAVE_JSON_SCHEMA_VALIDATOR
    nlohmann::json_schema::json_validator validator_;
    bool schemaLoaded_ = false;
#endif
    
    std::string getContractSchemaPath(const std::string& schemaName) const;
};

} // namespace warehouse::utils
