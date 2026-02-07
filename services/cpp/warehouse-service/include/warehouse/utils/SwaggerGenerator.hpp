#ifndef WAREHOUSE_UTILS_SWAGGERGENERATOR_HPP
#define WAREHOUSE_UTILS_SWAGGERGENERATOR_HPP

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace warehouse {
namespace utils {

/**
 * @brief Generates OpenAPI 3.0 specification for API documentation
 * 
 * Utility class to build Swagger/OpenAPI JSON specifications
 * for service endpoints. Provides methods to create the base
 * specification and add endpoint definitions.
 */
class SwaggerGenerator {
public:
    /**
     * @brief Generate base OpenAPI 3.0 specification
     */
    static json generateSpec(const std::string& title, 
                            const std::string& version,
                            const std::string& description = "");

    /**
     * @brief Add an endpoint definition to the specification
     */
    static void addEndpoint(json& spec,
                           const std::string& path,
                           const std::string& method,
                           const std::string& summary,
                           const std::string& description,
                           const json& parameters,
                           const json& requestBody,
                           const json& responses,
                           const std::vector<std::string>& tags = {});

    /**
     * @brief Add a schema definition to components
     */
    static void addSchema(json& spec,
                         const std::string& name,
                         const json& schema);

    /**
     * @brief Create a path parameter definition
     */
    static json createPathParameter(const std::string& name,
                                    const std::string& description,
                                    bool required = true);

    /**
     * @brief Create a query parameter definition
     */
    static json createQueryParameter(const std::string& name,
                                     const std::string& description,
                                     const std::string& type = "string",
                                     bool required = false);

    /**
     * @brief Create a request body definition
     */
    static json createRequestBody(const std::string& schemaRef,
                                  const std::string& description,
                                  bool required = true);

    /**
     * @brief Create a response definition
     */
    static json createResponse(const std::string& description,
                              const std::string& schemaRef = "");

    /**
     * @brief Create an error response definition
     */
    static json createErrorResponse(const std::string& description);
};

} // namespace utils
} // namespace warehouse

#endif // WAREHOUSE_UTILS_SWAGGERGENERATOR_HPP
