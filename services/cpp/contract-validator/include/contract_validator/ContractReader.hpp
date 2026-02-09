#ifndef CONTRACT_VALIDATOR_CONTRACTREADER_HPP
#define CONTRACT_VALIDATOR_CONTRACTREADER_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace contract_validator {

/**
 * @brief Reads and processes contract definitions for OpenAPI generation
 * 
 * This utility loads contract definitions (DTOs, Requests, Endpoints)
 * from JSON files and provides them in a format suitable for generating
 * OpenAPI specifications.
 */
class ContractReader {
public:
    struct DtoField {
        std::string name;
        std::string type;
        bool required;
        std::string description;
        std::string source;
    };

    struct DtoDefinition {
        std::string name;
        std::string version;
        std::string description;
        std::vector<DtoField> fields;
        json basis;
    };

    struct RequestParameter {
        std::string name;
        std::string type;
        bool required;
        std::string description;
    };

    struct RequestDefinition {
        std::string name;
        std::string version;
        std::string type; // command, query
        std::string commandType; // Create, Update, Delete, Process
        std::vector<std::string> basis;
        std::string resultType;
        std::vector<RequestParameter> parameters;
        std::string description;
    };

    struct EndpointParameter {
        std::string name;
        std::string location; // Route, Query, Body, Header
        std::string type;
        bool required;
        std::string description;
    };

    struct EndpointResponse {
        int status;
        std::string type;
        std::string description;
    };

    struct EndpointDefinition {
        std::string name;
        std::string version;
        std::string uri;
        std::string method;
        std::string basis;
        std::string authentication;
        std::string description;
        std::vector<EndpointParameter> parameters;
        std::vector<EndpointResponse> responses;
    };

    /**
     * @brief Construct a ContractReader with the contracts base directory
     * @param contractsPath Path to the service's contracts directory
     */
    explicit ContractReader(const std::string& contractsPath);

    /**
     * @brief Load all DTOs from the contracts/dtos directory
     * @return Map of DTO name to definition
     */
    std::map<std::string, DtoDefinition> loadDtos();

    /**
     * @brief Load all Requests from the contracts/requests directory
     * @return Map of Request name to definition
     */
    std::map<std::string, RequestDefinition> loadRequests();

    /**
     * @brief Load all Endpoints from the contracts/endpoints directory
     * @return List of endpoint definitions
     */
    std::vector<EndpointDefinition> loadEndpoints();

    /**
     * @brief Convert a contract type to JSON Schema type
     * @param contractType Contract type (UUID, PositiveInteger, DateTime, etc.)
     * @return JSON Schema type definition
     */
    static json contractTypeToJsonSchema(const std::string& contractType);

    /**
     * @brief Convert a DTO definition to OpenAPI schema
     * @param dto The DTO definition
     * @return OpenAPI schema object
     */
    static json dtoToSchema(const DtoDefinition& dto);

    /**
     * @brief Convert a Request definition to OpenAPI schema
     * @param request The Request definition
     * @return OpenAPI schema object
     */
    static json requestToSchema(const RequestDefinition& request);

private:
    std::string contractsPath_;

    /**
     * @brief Load and parse a JSON file
     * @param filePath Path to the JSON file
     * @return Parsed JSON object
     */
    static json loadJsonFile(const std::string& filePath);

    /**
     * @brief Parse a DTO from JSON
     * @param j JSON object
     * @return DTO definition
     */
    static DtoDefinition parseDto(const json& j);

    /**
     * @brief Parse a Request from JSON
     * @param j JSON object
     * @return Request definition
     */
    static RequestDefinition parseRequest(const json& j);

    /**
     * @brief Parse an Endpoint from JSON
     * @param j JSON object
     * @return Endpoint definition
     */
    static EndpointDefinition parseEndpoint(const json& j);
};

} // namespace contract_validator

#endif // CONTRACT_VALIDATOR_CONTRACTREADER_HPP
