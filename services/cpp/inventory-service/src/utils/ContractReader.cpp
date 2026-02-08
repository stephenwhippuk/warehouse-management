#include "inventory/utils/ContractReader.hpp"
#include "inventory/utils/Logger.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace inventory {
namespace utils {

ContractReader::ContractReader(const std::string& contractsPath) 
    : contractsPath_(contractsPath) {
    if (!fs::exists(contractsPath_)) {
        throw std::runtime_error("Contracts path does not exist: " + contractsPath_);
    }
}

std::map<std::string, ContractReader::DtoDefinition> ContractReader::loadDtos() {
    std::map<std::string, DtoDefinition> dtos;
    std::string dtosPath = contractsPath_ + "/dtos";

    if (!fs::exists(dtosPath)) {
        Logger::warn("DTOs directory not found: {}", dtosPath);
        return dtos;
    }

    for (const auto& entry : fs::directory_iterator(dtosPath)) {
        if (entry.path().extension() == ".json") {
            try {
                json j = loadJsonFile(entry.path().string());
                DtoDefinition dto = parseDto(j);
                dtos[dto.name] = dto;
                Logger::debug("Loaded DTO: {}", dto.name);
            } catch (const std::exception& e) {
                Logger::error("Failed to load DTO from {}: {}", entry.path().string(), e.what());
            }
        }
    }

    return dtos;
}

std::map<std::string, ContractReader::RequestDefinition> ContractReader::loadRequests() {
    std::map<std::string, RequestDefinition> requests;
    std::string requestsPath = contractsPath_ + "/requests";

    if (!fs::exists(requestsPath)) {
        Logger::warn("Requests directory not found: {}", requestsPath);
        return requests;
    }

    for (const auto& entry : fs::directory_iterator(requestsPath)) {
        if (entry.path().extension() == ".json") {
            try {
                json j = loadJsonFile(entry.path().string());
                RequestDefinition request = parseRequest(j);
                requests[request.name] = request;
                Logger::debug("Loaded Request: {}", request.name);
            } catch (const std::exception& e) {
                Logger::error("Failed to load Request from {}: {}", entry.path().string(), e.what());
            }
        }
    }

    return requests;
}

std::vector<ContractReader::EndpointDefinition> ContractReader::loadEndpoints() {
    std::vector<EndpointDefinition> endpoints;
    std::string endpointsPath = contractsPath_ + "/endpoints";

    if (!fs::exists(endpointsPath)) {
        Logger::warn("Endpoints directory not found: {}", endpointsPath);
        return endpoints;
    }

    for (const auto& entry : fs::directory_iterator(endpointsPath)) {
        if (entry.path().extension() == ".json") {
            try {
                json j = loadJsonFile(entry.path().string());
                EndpointDefinition endpoint = parseEndpoint(j);
                endpoints.push_back(endpoint);
                Logger::debug("Loaded Endpoint: {} {} {}", endpoint.method, endpoint.uri, endpoint.name);
            } catch (const std::exception& e) {
                Logger::error("Failed to load Endpoint from {}: {}", entry.path().string(), e.what());
            }
        }
    }

    return endpoints;
}

json ContractReader::contractTypeToJsonSchema(const std::string& contractType) {
    // Map contract types to JSON Schema types
    static const std::map<std::string, json> typeMap = {
        {"UUID", {{"type", "string"}, {"format", "uuid"}}},
        {"DateTime", {{"type", "string"}, {"format", "date-time"}}},
        {"Date", {{"type", "string"}, {"format", "date"}}},
        {"PositiveInteger", {{"type", "integer"}, {"minimum", 1}}},
        {"NonNegativeInteger", {{"type", "integer"}, {"minimum", 0}}},
        {"string", {{"type", "string"}}},
        {"integer", {{"type", "integer"}}},
        {"number", {{"type", "number"}}},
        {"boolean", {{"type", "boolean"}}},
        {"object", {{"type", "object"}}},
        {"array", {{"type", "array"}}},
        {"InventoryStatus", {
            {"type", "string"}, 
            {"enum", json::array({"available", "reserved", "allocated", "damaged", "expired", "quarantine", "recalled"})}
        }}
    };

    auto it = typeMap.find(contractType);
    if (it != typeMap.end()) {
        return it->second;
    }

    // If not a primitive type, assume it's a reference to another schema
    // Return as a $ref that will be resolved later
    return {{"$ref", "#/components/schemas/" + contractType}};
}

json ContractReader::dtoToSchema(const DtoDefinition& dto) {
    json schema = {
        {"type", "object"},
        {"description", dto.description},
        {"properties", json::object()},
        {"required", json::array()}
    };

    for (const auto& field : dto.fields) {
        json fieldSchema = contractTypeToJsonSchema(field.type);
        
        if (!field.description.empty()) {
            fieldSchema["description"] = field.description;
        }

        schema["properties"][field.name] = fieldSchema;

        if (field.required) {
            schema["required"].push_back(field.name);
        }
    }

    // If no required fields, remove the empty array
    if (schema["required"].empty()) {
        schema.erase("required");
    }

    return schema;
}

json ContractReader::requestToSchema(const RequestDefinition& request) {
    json schema = {
        {"type", "object"},
        {"description", request.description},
        {"properties", json::object()},
        {"required", json::array()}
    };

    for (const auto& param : request.parameters) {
        json paramSchema = contractTypeToJsonSchema(param.type);
        
        if (!param.description.empty()) {
            paramSchema["description"] = param.description;
        }

        schema["properties"][param.name] = paramSchema;

        if (param.required) {
            schema["required"].push_back(param.name);
        }
    }

    // If no required fields, remove the empty array
    if (schema["required"].empty()) {
        schema.erase("required");
    }

    return schema;
}

json ContractReader::loadJsonFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parse error in " + filePath + ": " + e.what());
    }

    return j;
}

ContractReader::DtoDefinition ContractReader::parseDto(const json& j) {
    DtoDefinition dto;
    dto.name = j.at("name").get<std::string>();
    dto.version = j.at("version").get<std::string>();
    dto.description = j.value("description", "");
    dto.basis = j.value("basis", json::array());

    if (j.contains("fields") && j["fields"].is_array()) {
        for (const auto& fieldJson : j["fields"]) {
            DtoField field;
            field.name = fieldJson.at("name").get<std::string>();
            field.type = fieldJson.at("type").get<std::string>();
            field.required = fieldJson.value("required", false);
            field.description = fieldJson.value("description", "");
            field.source = fieldJson.value("source", "");
            dto.fields.push_back(field);
        }
    }

    return dto;
}

ContractReader::RequestDefinition ContractReader::parseRequest(const json& j) {
    RequestDefinition request;
    request.name = j.at("name").get<std::string>();
    request.version = j.at("version").get<std::string>();
    request.type = j.value("type", "");
    request.commandType = j.value("commandType", "");
    request.resultType = j.value("resultType", "");
    request.description = j.value("description", "");

    if (j.contains("basis") && j["basis"].is_array()) {
        for (const auto& basisItem : j["basis"]) {
            request.basis.push_back(basisItem.get<std::string>());
        }
    }

    if (j.contains("parameters") && j["parameters"].is_array()) {
        for (const auto& paramJson : j["parameters"]) {
            RequestParameter param;
            param.name = paramJson.at("name").get<std::string>();
            param.type = paramJson.at("type").get<std::string>();
            param.required = paramJson.value("required", false);
            param.description = paramJson.value("description", "");
            request.parameters.push_back(param);
        }
    }

    return request;
}

ContractReader::EndpointDefinition ContractReader::parseEndpoint(const json& j) {
    EndpointDefinition endpoint;
    endpoint.name = j.at("name").get<std::string>();
    endpoint.version = j.at("version").get<std::string>();
    endpoint.uri = j.at("uri").get<std::string>();
    endpoint.method = j.at("method").get<std::string>();
    endpoint.basis = j.value("basis", "");
    endpoint.authentication = j.value("authentication", "");
    endpoint.description = j.value("description", "");

    if (j.contains("parameters") && j["parameters"].is_array()) {
        for (const auto& paramJson : j["parameters"]) {
            EndpointParameter param;
            param.name = paramJson.at("name").get<std::string>();
            param.location = paramJson.at("location").get<std::string>();
            param.type = paramJson.at("type").get<std::string>();
            param.required = paramJson.value("required", false);
            param.description = paramJson.value("description", "");
            endpoint.parameters.push_back(param);
        }
    }

    if (j.contains("responses") && j["responses"].is_array()) {
        for (const auto& responseJson : j["responses"]) {
            EndpointResponse response;
            response.status = responseJson.at("status").get<int>();
            response.type = responseJson.value("type", "");
            response.description = responseJson.value("description", "");
            endpoint.responses.push_back(response);
        }
    }

    return endpoint;
}

} // namespace utils
} // namespace inventory
