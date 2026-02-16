#include "contract-plugin/SwaggerService.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;

namespace contract {

SwaggerService::SwaggerService(const ContractConfig& config,
                               std::shared_ptr<IClaimsLoader> claimsLoader)
    : config_(config)
    , claimsLoader_(claimsLoader) {
    
    // Try to load claims for service metadata
    if (claimsLoader_) {
        auto claimsOpt = claimsLoader_->loadClaims(config_.contractsPath + "/claims.json");
        if (claimsOpt) {
            claims_ = *claimsOpt;
            claimsLoaded_ = true;
            spdlog::debug("SwaggerService loaded claims from {}/claims.json", config_.contractsPath);
        } else {
            spdlog::warn("SwaggerService could not load claims.json, using config defaults");
        }
    }
}

json SwaggerService::generateSpec() {
    try {
        auto spec = createBaseSpec();
        
        // Load DTOs for schemas section
        auto dtoSchemas = loadDtoSchemas();
        for (const auto& [name, schema] : dtoSchemas) {
            spec["components"]["schemas"][name] = schema;
        }
        
        // Load endpoints and build paths
        auto endpoints = loadEndpoints();
        for (const auto& endpoint : endpoints) {
            if (!endpoint.contains("uri") || !endpoint.contains("method")) {
                continue;
            }
            
            std::string uri = endpoint["uri"];
            std::string method = endpoint["method"];
            std::transform(method.begin(), method.end(), method.begin(), ::tolower);
            
            // Create path item if doesn't exist
            if (!spec["paths"].contains(uri)) {
                spec["paths"][uri] = json::object();
            }
            
            // Add operation
            spec["paths"][uri][method] = endpointToOperation(endpoint);
        }
        
        return spec;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to generate OpenAPI spec: {}", e.what());
        throw std::runtime_error("Failed to generate OpenAPI specification: " + std::string(e.what()));
    }
}

json SwaggerService::createBaseSpec() {
    // Get service name and version from claims if available
    std::string title = config_.swaggerTitle;
    std::string version = config_.swaggerVersion;
    
    if (claimsLoaded_) {
        if (claims_.contains("service")) {
            title = claims_["service"].get<std::string>() + " API";
        }
        if (claims_.contains("version")) {
            version = claims_["version"];
        }
    }
    
    json spec = {
        {"openapi", "3.0.3"},
        {"info", {
            {"title", title},
            {"version", version},
            {"description", config_.swaggerDescription}
        }},
        {"servers", json::array({
            {{"url", "/", "description", "Current server"}}
        })},
        {"paths", json::object()},
        {"components", {
            {"schemas", json::object()}
        }}
    };
    
    return spec;
}

std::map<std::string, json> SwaggerService::loadDtoSchemas() {
    std::map<std::string, json> schemas;
    
    std::filesystem::path dtosDir = std::filesystem::path(config_.contractsPath) / "dtos";
    
    if (!std::filesystem::exists(dtosDir)) {
        spdlog::warn("DTOs directory not found: {}", dtosDir.string());
        return schemas;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(dtosDir)) {
        if (entry.path().extension() != ".json") continue;
        
        try {
            std::ifstream file(entry.path());
            json dtoJson;
            file >> dtoJson;
            
            if (dtoJson.contains("name")) {
                std::string name = dtoJson["name"];
                
                // Build OpenAPI schema from DTO definition
                json schema = {
                    {"type", "object"},
                    {"properties", json::object()},
                    {"required", json::array()}
                };
                
                if (dtoJson.contains("fields")) {
                    for (const auto& field : dtoJson["fields"]) {
                        if (!field.contains("name") || !field.contains("type")) continue;
                        
                        std::string fieldName = field["name"];
                        std::string fieldType = field["type"];
                        bool required = field.value("required", false);
                        
                        // Convert contract type to OpenAPI schema
                        json fieldSchema = contractTypeToSchema(fieldType);
                        
                        schema["properties"][fieldName] = fieldSchema;
                        
                        if (required) {
                            schema["required"].push_back(fieldName);
                        }
                    }
                }
                
                schemas[name] = schema;
            }
        } catch (const std::exception& e) {
            spdlog::warn("Failed to parse DTO {}: {}", entry.path().string(), e.what());
        }
    }
    
    spdlog::debug("Loaded {} DTO schemas", schemas.size());
    return schemas;
}

std::vector<json> SwaggerService::loadEndpoints() {
    std::vector<json> endpoints;
    
    std::filesystem::path endpointsDir = std::filesystem::path(config_.contractsPath) / "endpoints";
    
    if (!std::filesystem::exists(endpointsDir)) {
        spdlog::warn("Endpoints directory not found: {}", endpointsDir.string());
        return endpoints;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(endpointsDir)) {
        if (entry.path().extension() != ".json") continue;
        
        try {
            std::ifstream file(entry.path());
            json endpointJson;
            file >> endpointJson;
            endpoints.push_back(endpointJson);
        } catch (const std::exception& e) {
            spdlog::warn("Failed to parse endpoint {}: {}", entry.path().string(), e.what());
        }
    }
    
    spdlog::debug("Loaded {} endpoint contracts", endpoints.size());
    return endpoints;
}

json SwaggerService::endpointToOperation(const json& endpoint) {
    json operation = {
        {"summary", endpoint.value("name", "Endpoint")},
        {"description", endpoint.value("description", "")},
        {"parameters", json::array()},
        {"responses", json::object()}
    };
    
    // Add parameters
    if (endpoint.contains("parameters")) {
        for (const auto& param : endpoint["parameters"]) {
            if (!param.contains("name") || !param.contains("location")) continue;
            
            std::string location = param["location"];
            std::string in;
            if (location == "Route") in = "path";
            else if (location == "Query") in = "query";
            else if (location == "Header") in = "header";
            else if (location == "Body") {
                // Body parameter goes in requestBody, not parameters
                operation["requestBody"] = {
                    {"required", param.value("required", true)},
                    {"content", {
                        {"application/json", {
                            {"schema", {{"$ref", "#/components/schemas/" + param.value("type", "object")}}}
                        }}
                    }}
                };
                continue;
            } else {
                continue;
            }
            
            json paramSchema = {
                {"name", param["name"]},
                {"in", in},
                {"required", param.value("required", false)},
                {"schema", {{"type", "string"}}}
            };
            
            if (param.contains("type")) {
                std::string paramType = param["type"];
                paramSchema["schema"] = contractTypeToSchema(paramType);
            }
            
            operation["parameters"].push_back(paramSchema);
        }
    }
    
    // Add responses
    if (endpoint.contains("responses")) {
        for (const auto& response : endpoint["responses"]) {
            if (!response.contains("status")) continue;
            
            int status = response["status"];
            std::string statusStr = std::to_string(status);
            
            json responseObj = {
                {"description", response.value("description", "Response")}
            };
            
            if (response.contains("type")) {
                responseObj["content"] = {
                    {"application/json", {
                        {"schema", {{"$ref", "#/components/schemas/" + response.value("type", "object")}}}
                    }}
                };
            }
            
            operation["responses"][statusStr] = responseObj;
        }
    }
    
    return operation;
}

json SwaggerService::contractTypeToSchema(const std::string& contractType) {
    if (contractType == "UUID") {
        return {{"type", "string"}, {"format", "uuid"}};
    } else if (contractType == "string") {
        return {{"type", "string"}};
    } else if (contractType == "integer" || contractType == "PositiveInteger" || contractType == "NonNegativeInteger") {
        return {{"type", "integer"}};
    } else if (contractType == "number" || contractType == "double") {
        return {{"type", "number"}};
    } else if (contractType == "boolean") {
        return {{"type", "boolean"}};
    } else if (contractType == "DateTime") {
        return {{"type", "string"}, {"format", "date-time"}};
    } else {
        // Assume it's a reference to another DTO or complex type
        return {{"$ref", "#/components/schemas/" + contractType}};
    }
}

} // namespace contract
