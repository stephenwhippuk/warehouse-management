#ifndef CONTRACT_PLUGIN_SWAGGER_SERVICE_HPP
#define CONTRACT_PLUGIN_SWAGGER_SERVICE_HPP

#include "contract-plugin/ISwaggerService.hpp"
#include "contract-plugin/IClaimsLoader.hpp"
#include "contract-plugin/ContractConfig.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <map>
#include <vector>
#include <memory>

namespace contract {

/**
 * @brief Implementation of OpenAPI/Swagger specification generation
 * 
 * Loads service claims and contract definitions to generate OpenAPI 3.0 spec.
 * Uses ClaimsLoader for claims.json and reads contract files from filesystem.
 */
class SwaggerService : public ISwaggerService {
public:
    /**
     * @brief Construct service with configuration and claims loader
     * @param config Configuration with contract paths and swagger settings
     * @param claimsLoader Loader for claims.json (mockable for testing)
     */
    SwaggerService(const ContractConfig& config,
                   std::shared_ptr<IClaimsLoader> claimsLoader);

    nlohmann::json generateSpec() override;

private:
    ContractConfig config_;
    std::shared_ptr<IClaimsLoader> claimsLoader_;
    nlohmann::json claims_;
    bool claimsLoaded_ = false;

    /**
     * @brief Create base OpenAPI structure
     */
    nlohmann::json createBaseSpec();

    /**
     * @brief Load DTO contract files and convert to OpenAPI schemas
     * @return Map of DTO name to OpenAPI schema
     */
    std::map<std::string, nlohmann::json> loadDtoSchemas();

    /**
     * @brief Load endpoint contract files
     * @return Vector of endpoint definitions
     */
    std::vector<nlohmann::json> loadEndpoints();

    /**
     * @brief Convert endpoint contract to OpenAPI path item
     * @param endpoint Endpoint contract JSON
     * @return OpenAPI operation object
     */
    nlohmann::json endpointToOperation(const nlohmann::json& endpoint);

    /**
     * @brief Convert contract type to OpenAPI schema
     * @param contractType Contract type (e.g., "UUID", "PositiveInteger")
     * @return OpenAPI schema
     */
    nlohmann::json contractTypeToSchema(const std::string& contractType);
};

} // namespace contract

#endif // CONTRACT_PLUGIN_SWAGGER_SERVICE_HPP
