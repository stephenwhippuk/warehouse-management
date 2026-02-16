#pragma once

#include <http-framework/ControllerBase.hpp>
#include <http-framework/HttpContext.hpp>
#include "ContractConfig.hpp"
#include "IClaimsService.hpp"
#include <memory>

namespace contract {

/**
 * @brief Controller that serves claims information
 * 
 * Endpoints:
 * - GET /api/v1/claims - Returns full claims.json content
 * - GET /api/v1/claims/fulfilments - Returns fulfilments section
 * - GET /api/v1/claims/references - Returns references section
 * - GET /api/v1/claims/services - Returns service contracts section
 * - GET /api/v1/claims/supports/{type}/{name}/{version} - Check contract support
 */
class ClaimsController : public http::ControllerBase {
public:
    /**
     * @brief Create claims controller
     * @param config Configuration specifying claims file path
     * @param claimsService Service to handle claims operations
     */
    explicit ClaimsController(const ContractConfig& config,
                             std::shared_ptr<IClaimsService> claimsService);

private:
    ContractConfig config_;
    std::shared_ptr<IClaimsService> claimsService_;
    
    /**
     * @brief Handle GET /api/v1/claims
     */
    std::string handleGetAllClaims(http::HttpContext& ctx);
    
    /**
     * @brief Handle GET /api/v1/claims/fulfilments
     */
    std::string handleGetFulfilments(http::HttpContext& ctx);
    
    /**
     * @brief Handle GET /api/v1/claims/references
     */
    std::string handleGetReferences(http::HttpContext& ctx);
    
    /**
     * @brief Handle GET /api/v1/claims/services
     */
    std::string handleGetServices(http::HttpContext& ctx);
    
    /**
     * @brief Handle GET /api/v1/claims/supports/{type}/{name}/{version}
     */
    std::string handleSupportsCheck(http::HttpContext& ctx);
};

} // namespace contract
