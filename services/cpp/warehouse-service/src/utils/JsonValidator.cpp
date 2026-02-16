#include "warehouse/utils/JsonValidator.hpp"
#include "warehouse/utils/Logger.hpp"
#include <fstream>

namespace warehouse::utils {

using json = nlohmann::json;

JsonValidator::JsonValidator() {
#ifdef HAVE_JSON_SCHEMA_VALIDATOR
    schemaLoaded_ = false;
#endif
}

bool JsonValidator::loadSchema(const std::string& schemaPath) {
#ifdef HAVE_JSON_SCHEMA_VALIDATOR
    try {
        std::ifstream schemaFile(schemaPath);
        if (!schemaFile.is_open()) {
            Logger::error("Failed to open schema file: {}", schemaPath);
            return false;
        }
        
        json schemaJson = json::parse(schemaFile);
        validator_.set_root_schema(schemaJson);
        schemaLoaded_ = true;
        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to load schema: {}", e.what());
        return false;
    }
#else
    (void)schemaPath;
    Logger::warn("JSON Schema validation not available (library not found)");
    return false;
#endif
}

bool JsonValidator::loadSchemaFromString(const std::string& schemaJson) {
#ifdef HAVE_JSON_SCHEMA_VALIDATOR
    try {
        json schema = json::parse(schemaJson);
        validator_.set_root_schema(schema);
        schemaLoaded_ = true;
        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to parse schema: {}", e.what());
        return false;
    }
#else
    (void)schemaJson;
    return false;
#endif
}

bool JsonValidator::validate(const json& data, std::string& errorMessage) const {
#ifdef HAVE_JSON_SCHEMA_VALIDATOR
    if (!schemaLoaded_) {
        errorMessage = "No schema loaded";
        return false;
    }
    
    try {
        validator_.validate(data);
        return true;
    } catch (const std::exception& e) {
        errorMessage = e.what();
        return false;
    }
#else
    (void)data;
    (void)errorMessage;
    // If validator not available, accept everything
    return true;
#endif
}

bool JsonValidator::validate(const std::string& jsonData, std::string& errorMessage) const {
    try {
        json data = json::parse(jsonData);
        return validate(data, errorMessage);
    } catch (const std::exception& e) {
        errorMessage = std::string("JSON parse error: ") + e.what();
        return false;
    }
}

bool JsonValidator::isValid(const json& data) const {
    std::string dummy;
    return validate(data, dummy);
}

std::string JsonValidator::getContractSchemaPath(const std::string& schemaName) const {
    // Relative path to contracts from service binary location
    return "../../../contracts/schemas/v1/" + schemaName + ".schema.json";
}

std::optional<JsonValidator> JsonValidator::loadWarehouseSchema() {
    JsonValidator validator;
    if (validator.loadSchema(validator.getContractSchemaPath("warehouse"))) {
        return validator;
    }
    return std::nullopt;
}

std::optional<JsonValidator> JsonValidator::loadLocationSchema() {
    JsonValidator validator;
    if (validator.loadSchema(validator.getContractSchemaPath("location"))) {
        return validator;
    }
    return std::nullopt;
}

std::optional<JsonValidator> JsonValidator::loadCommonSchema() {
    JsonValidator validator;
    if (validator.loadSchema(validator.getContractSchemaPath("common"))) {
        return validator;
    }
    return std::nullopt;
}

} // namespace warehouse::utils
