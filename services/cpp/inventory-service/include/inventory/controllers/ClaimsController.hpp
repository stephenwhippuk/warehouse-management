#pragma once

#include <http-framework/ControllerBase.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace inventory {
namespace controllers {

/**
 * @brief Controller for exposing service claims and contract support
 *
 * Provides REST endpoints to discover what contracts, entities, and services
 * this service fulfills or references, with version support.
 *
 * Endpoints:
 *   GET /api/v1/claims              - Full claims document
 *   GET /api/v1/claims/fulfilments  - Fulfilled entity contracts
 *   GET /api/v1/claims/references   - Referenced entity contracts
 *   GET /api/v1/claims/services     - Fulfilled service contracts
 *   GET /api/v1/claims/supports/{type}/{name}/{version} - Check support
 *
 * The 'supports' endpoint checks if this service fulfills or references
 * a specific contract at a given version. Type can be 'entity' or 'service'.
 */
class ClaimsController : public http::ControllerBase {
public:
    ClaimsController();

private:
    nlohmann::json claims_;

    std::string handleGetAllClaims(http::HttpContext& ctx);
    std::string handleGetFulfilments(http::HttpContext& ctx);
    std::string handleGetReferences(http::HttpContext& ctx);
    std::string handleGetServices(http::HttpContext& ctx);
    std::string handleSupportsCheck(http::HttpContext& ctx);

    bool loadClaims();
    bool supportsEntity(const std::string& name, const std::string& version, bool& fulfilled);
    bool supportsService(const std::string& name, const std::string& version);
};

} // namespace controllers
} // namespace inventory
