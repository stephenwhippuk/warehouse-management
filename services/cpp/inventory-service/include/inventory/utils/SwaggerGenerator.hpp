#ifndef INVENTORY_UTILS_SWAGGERGENERATOR_HPP
#define INVENTORY_UTILS_SWAGGERGENERATOR_HPP

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace inventory {
namespace utils {

/**
 * @brief Generates OpenAPI 3.0 specification for API documentation
 * 
 * Utility class to build Swagger/OpenAPI JSON specifications
 * for service endpoints. Provides methods to create the base
 * specification and add endpoint definitions, or generate the
 * entire specification from contract definitions.
 */
class SwaggerGenerator {
public:
    /**
     * @brief Generate complete OpenAPI specification from contract definitions
     * @param title API title
     * @param version API version
     * @param description API description
     * @param contractsPath Path to the service's contracts directory
     * @return JSON object with complete OpenAPI specification
     */
    static json generateSpecFromContracts(const std::string& title,
                                          const std::string& version,
                                          const std::string& description,
                                          const std::string& contractsPath);

    /**
     * @brief Generate base OpenAPI 3.0 specification
     * @param title API title
     * @param version API version
     * @param description API description
     * @return JSON object with base OpenAPI structure
     */
    static json generateSpec(const std::string& title, 
                            const std::string& version,
                            const std::string& description = "");

    /**
     * @brief Add an endpoint definition to the specification
     * @param spec The OpenAPI specification object
     * @param path The endpoint path (e.g., "/api/v1/inventory/{id}")
     * @param method HTTP method (get, post, put, delete, patch)
     * @param summary Short endpoint description
     * @param description Detailed endpoint description
     * @param parameters Path/query parameters
     * @param requestBody Request body schema
     * @param responses Response definitions
     * @param tags Tags for grouping
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
     * @param spec The OpenAPI specification object
     * @param name Schema name
     * @param schema The JSON schema definition
     */
    static void addSchema(json& spec,
                         const std::string& name,
                         const json& schema);

    /**
     * @brief Create a path parameter definition
     * @param name Parameter name
     * @param description Parameter description
     * @param required Whether the parameter is required
     * @return JSON parameter definition
     */
    static json createPathParameter(const std::string& name,
                                    const std::string& description,
                                    bool required = true);

    /**
     * @brief Create a query parameter definition
     * @param name Parameter name
     * @param description Parameter description
     * @param required Whether the parameter is required
     * @param type Parameter type (string, integer, boolean)
     * @return JSON parameter definition
     */
    static json createQueryParameter(const std::string& name,
                                     const std::string& description,
                                     const std::string& type = "string",
                                     bool required = false);

    /**
     * @brief Create a request body definition
     * @param schemaRef Schema reference (e.g., "#/components/schemas/Inventory")
     * @param description Request body description
     * @param required Whether the body is required
     * @return JSON request body definition
     */
    static json createRequestBody(const std::string& schemaRef,
                                  const std::string& description,
                                  bool required = true);

    /**
     * @brief Create a response definition
     * @param description Response description
     * @param schemaRef Schema reference or empty for no body
     * @return JSON response definition
     */
    static json createResponse(const std::string& description,
                              const std::string& schemaRef = "");

    /**
     * @brief Create an error response definition
     * @param description Error description
     * @return JSON error response definition
     */
    static json createErrorResponse(const std::string& description);
};

} // namespace utils
} // namespace inventory

#endif // INVENTORY_UTILS_SWAGGERGENERATOR_HPP
