#include "inventory/utils/SwaggerGenerator.hpp"
#include "contract_validator/ContractReader.hpp"
#include "inventory/utils/Logger.hpp"
#include <algorithm>
#include <set>

namespace inventory {
namespace utils {

using contract_validator::ContractReader;

json SwaggerGenerator::generateSpecFromContracts(const std::string& title,
                                                 const std::string& version,
                                                 const std::string& description,
                                                 const std::string& contractsPath) {
    Logger::info("Generating OpenAPI specification from contracts at {}", contractsPath);

    // Create base spec
    json spec = generateSpec(title, version, description);

    // Add security schemes for API key authentication
    spec["components"]["securitySchemes"] = {
        {"ApiKeyHeader", {
            {"type", "apiKey"},
            {"in", "header"},
            {"name", "X-Service-Api-Key"},
            {"description", "Service-to-service API key authentication"}
        }},
        {"ApiKeyAuth", {
            {"type", "apiKey"},
            {"in", "header"},
            {"name", "Authorization"},
            {"description", "API key authentication using 'ApiKey <key>' format"}
        }}
    };

    // Load contracts
    ContractReader reader(contractsPath);
    
    auto dtos = reader.loadDtos();
    auto requests = reader.loadRequests();
    auto endpoints = reader.loadEndpoints();

    Logger::info("Loaded {} DTOs, {} Requests, {} Endpoints", dtos.size(), requests.size(), endpoints.size());

    // Add schemas for DTOs
    for (const auto& [name, dto] : dtos) {
        json schema = ContractReader::dtoToSchema(dto);
        spec["components"]["schemas"][name] = schema;
        Logger::debug("Added schema for DTO: {}", name);
    }

    // Add schemas for Requests
    for (const auto& [name, request] : requests) {
        json schema = ContractReader::requestToSchema(request);
        spec["components"]["schemas"][name] = schema;
        Logger::debug("Added schema for Request: {}", name);
    }

    // Track unique tags
    std::set<std::string> tags;

    // Add endpoints
    for (const auto& endpoint : endpoints) {
        // Determine tag from URI (e.g., /api/v1/inventory -> Inventory)
        std::string tag = "API";
        if (endpoint.uri.find("/inventory") != std::string::npos) {
            tag = "Inventory";
        } else if (endpoint.uri.find("/health") != std::string::npos) {
            tag = "Health";
        }
        tags.insert(tag);

        // Build parameters array
        json parameters = json::array();
        json requestBody = json(nullptr);

        for (const auto& param : endpoint.parameters) {
            json paramJson;
            
            if (param.location == "Route") {
                paramJson = {
                    {"name", param.name},
                    {"in", "path"},
                    {"description", param.description},
                    {"required", param.required},
                    {"schema", ContractReader::contractTypeToJsonSchema(param.type)}
                };
                parameters.push_back(paramJson);
            } else if (param.location == "Query") {
                paramJson = {
                    {"name", param.name},
                    {"in", "query"},
                    {"description", param.description},
                    {"required", param.required},
                    {"schema", ContractReader::contractTypeToJsonSchema(param.type)}
                };
                parameters.push_back(paramJson);
            } else if (param.location == "Body") {
                // Body parameter becomes requestBody
                requestBody = {
                    {"description", param.description},
                    {"required", param.required},
                    {"content", {
                        {"application/json", {
                            {"schema", {{"$ref", "#/components/schemas/" + param.type}}}
                        }}
                    }}
                };
            } else if (param.location == "Header") {
                paramJson = {
                    {"name", param.name},
                    {"in", "header"},
                    {"description", param.description},
                    {"required", param.required},
                    {"schema", ContractReader::contractTypeToJsonSchema(param.type)}
                };
                parameters.push_back(paramJson);
            }
        }

        // Build responses object
        json responses = json::object();
        for (const auto& response : endpoint.responses) {
            std::string statusStr = std::to_string(response.status);
            
            json responseJson = {
                {"description", response.description}
            };

            // Add content if there's a response type
            if (!response.type.empty()) {
                responseJson["content"] = {
                    {"application/json", {
                        {"schema", {{"$ref", "#/components/schemas/" + response.type}}}
                    }}
                };
            }

            responses[statusStr] = responseJson;
        }

        // Convert method to lowercase for OpenAPI
        std::string method = endpoint.method;
        std::transform(method.begin(), method.end(), method.begin(), ::tolower);

        // Build operation object
        json operation = {
            {"summary", endpoint.name},
            {"description", endpoint.description},
            {"operationId", endpoint.name},
            {"tags", json::array({tag})}
        };

        if (!parameters.empty()) {
            operation["parameters"] = parameters;
        }

        if (!requestBody.is_null()) {
            operation["requestBody"] = requestBody;
        }

        operation["responses"] = responses;

        // Add security if authentication is required
        if (endpoint.authentication == "ApiKey") {
            operation["security"] = json::array({
                {{"ApiKeyHeader", json::array()}},
                {{"ApiKeyAuth", json::array()}}
            });
        }

        // Ensure path exists in spec
        if (!spec["paths"].contains(endpoint.uri)) {
            spec["paths"][endpoint.uri] = json::object();
        }

        spec["paths"][endpoint.uri][method] = operation;
        Logger::debug("Added endpoint: {} {} ({})", method, endpoint.uri, endpoint.name);
    }

    // Add tags to spec
    spec["tags"] = json::array();
    for (const auto& tag : tags) {
        std::string tagDescription;
        if (tag == "Inventory") {
            tagDescription = "Inventory management operations";
        } else if (tag == "Health") {
            tagDescription = "Service health checks";
        } else {
            tagDescription = tag + " operations";
        }

        json tagObj = {
            {"name", tag},
            {"description", tagDescription}
        };
        spec["tags"].push_back(tagObj);
    }

    Logger::info("Generated OpenAPI specification with {} paths", spec["paths"].size());
    return spec;
}

json SwaggerGenerator::generateSpec(const std::string& title,
                                    const std::string& version,
                                    const std::string& description) {
    json spec = {
        {"openapi", "3.0.0"},
        {"info", {
            {"title", title},
            {"version", version},
            {"description", description.empty() ? title : description}
        }},
        {"servers", json::array({
            {{"url", "http://localhost:8081"}, {"description", "Development server"}},
            {{"url", "http://inventory-service:8081"}, {"description", "Docker container"}}
        })},
        {"paths", json::object()},
        {"components", {
            {"schemas", json::object()},
            {"responses", {
                {"NotFound", {
                    {"description", "Resource not found"},
                    {"content", {
                        {"application/json", {
                            {"schema", {
                                {"type", "object"},
                                {"properties", {
                                    {"error", {"type", "string"}},
                                    {"message", {"type", "string"}}
                                }}
                            }}
                        }}
                    }}
                }},
                {"BadRequest", {
                    {"description", "Invalid request"},
                    {"content", {
                        {"application/json", {
                            {"schema", {
                                {"type", "object"},
                                {"properties", {
                                    {"error", {"type", "string"}},
                                    {"message", {"type", "string"}}
                                }}
                            }}
                        }}
                    }}
                }},
                {"InternalError", {
                    {"description", "Internal server error"},
                    {"content", {
                        {"application/json", {
                            {"schema", {
                                {"type", "object"},
                                {"properties", {
                                    {"error", {"type", "string"}},
                                    {"message", {"type", "string"}}
                                }}
                            }}
                        }}
                    }}
                }}
            }}
        }},
        {"tags", json::array()}
    };

    return spec;
}

void SwaggerGenerator::addEndpoint(json& spec,
                                   const std::string& path,
                                   const std::string& method,
                                   const std::string& summary,
                                   const std::string& description,
                                   const json& parameters,
                                   const json& requestBody,
                                   const json& responses,
                                   const std::vector<std::string>& tags) {
    // Ensure path exists
    if (!spec["paths"].contains(path)) {
        spec["paths"][path] = json::object();
    }

    // Create endpoint definition
    json endpoint = {
        {"summary", summary},
        {"description", description.empty() ? summary : description}
    };

    // Add tags if provided
    if (!tags.empty()) {
        endpoint["tags"] = tags;
    }

    // Add parameters if provided
    if (!parameters.is_null() && !parameters.empty()) {
        endpoint["parameters"] = parameters;
    }

    // Add request body if provided
    if (!requestBody.is_null()) {
        endpoint["requestBody"] = requestBody;
    }

    // Add responses
    endpoint["responses"] = responses;

    // Add endpoint to spec
    spec["paths"][path][method] = endpoint;
}

void SwaggerGenerator::addSchema(json& spec,
                                const std::string& name,
                                const json& schema) {
    spec["components"]["schemas"][name] = schema;
}

json SwaggerGenerator::createPathParameter(const std::string& name,
                                          const std::string& description,
                                          bool required) {
    return {
        {"name", name},
        {"in", "path"},
        {"description", description},
        {"required", required},
        {"schema", {
            {"type", "string"}
        }}
    };
}

json SwaggerGenerator::createQueryParameter(const std::string& name,
                                           const std::string& description,
                                           const std::string& type,
                                           bool required) {
    return {
        {"name", name},
        {"in", "query"},
        {"description", description},
        {"required", required},
        {"schema", {
            {"type", type}
        }}
    };
}

json SwaggerGenerator::createRequestBody(const std::string& schemaRef,
                                        const std::string& description,
                                        bool required) {
    return {
        {"description", description},
        {"required", required},
        {"content", {
            {"application/json", {
                {"schema", {
                    {"$ref", schemaRef}
                }}
            }}
        }}
    };
}

json SwaggerGenerator::createResponse(const std::string& description,
                                     const std::string& schemaRef) {
    json response = {
        {"description", description}
    };

    if (!schemaRef.empty()) {
        response["content"] = {
            {"application/json", {
                {"schema", {
                    {"$ref", schemaRef}
                }}
            }}
        };
    }

    return response;
}

json SwaggerGenerator::createErrorResponse(const std::string& description) {
    return {
        {"description", description},
        {"content", {
            {"application/json", {
                {"schema", {
                    {"type", "object"},
                    {"properties", {
                        {"error", {"type", "string"}},
                        {"message", {"type", "string"}}
                    }}
                }}
            }}
        }}
    };
}

} // namespace utils
} // namespace inventory
