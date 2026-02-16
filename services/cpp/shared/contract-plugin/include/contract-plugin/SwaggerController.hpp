#pragma once

#include <http-framework/ControllerBase.hpp>
#include <http-framework/HttpContext.hpp>
#include "ContractConfig.hpp"
#include "ISwaggerService.hpp"
#include <nlohmann/json.hpp>
#include <memory>

namespace contract {

/**
 * @brief Controller that generates OpenAPI 3.0 specification from contracts
 * 
 * Endpoints:
 * - GET /api/swagger.json - Returns OpenAPI spec generated from contract files
 * 
 * Delegates to SwaggerService for spec generation.
 */
class SwaggerController : public http::ControllerBase {
public:
    /**
     * @brief Create swagger controller with service dependency
     * @param config Configuration specifying contract paths and metadata
     * @param swaggerService Service for generating OpenAPI spec (injected, mockable)
     */
    explicit SwaggerController(const ContractConfig& config,
                              std::shared_ptr<ISwaggerService> swaggerService);

private:
    ContractConfig config_;
    std::shared_ptr<ISwaggerService> swaggerService_;
    
    /**
     * @brief Handle GET /api/swagger.json
     */
    std::string getSwagger(http::HttpContext& ctx);
};

} // namespace contract
