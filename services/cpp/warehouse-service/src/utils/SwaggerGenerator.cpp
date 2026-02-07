#include "warehouse/utils/SwaggerGenerator.hpp"

namespace warehouse {
namespace utils {

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
            {{"url", "http://localhost:8080"}, {"description", "Development server"}},
            {{"url", "http://warehouse-service:8080"}, {"description", "Docker container"}}
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
    if (!spec["paths"].contains(path)) {
        spec["paths"][path] = json::object();
    }

    json endpoint = {
        {"summary", summary},
        {"description", description.empty() ? summary : description}
    };

    if (!tags.empty()) {
        endpoint["tags"] = tags;
    }

    if (!parameters.is_null() && !parameters.empty()) {
        endpoint["parameters"] = parameters;
    }

    if (!requestBody.is_null()) {
        endpoint["requestBody"] = requestBody;
    }

    endpoint["responses"] = responses;
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
        {"schema", {{"type", "string"}}}
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
        {"schema", {{"type", type}}}
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
                {"schema", {{"$ref", schemaRef}}}
            }}
        }}
    };
}

json SwaggerGenerator::createResponse(const std::string& description,
                                     const std::string& schemaRef) {
    json response = {{"description", description}};

    if (!schemaRef.empty()) {
        response["content"] = {
            {"application/json", {
                {"schema", {{"$ref", schemaRef}}}
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
} // namespace warehouse
