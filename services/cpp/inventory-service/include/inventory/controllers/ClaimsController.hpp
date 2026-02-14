#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
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
class ClaimsController : public Poco::Net::HTTPRequestHandler {
public:
    ClaimsController();

    void handleRequest(Poco::Net::HTTPServerRequest& request,
                       Poco::Net::HTTPServerResponse& response) override;

private:
    nlohmann::json claims_;

    void handleGetAllClaims(Poco::Net::HTTPServerResponse& response);
    void handleGetFulfilments(Poco::Net::HTTPServerResponse& response);
    void handleGetReferences(Poco::Net::HTTPServerResponse& response);
    void handleGetServices(Poco::Net::HTTPServerResponse& response);
    void handleSupportsCheck(const std::string& type,
                            const std::string& name,
                            const std::string& version,
                            Poco::Net::HTTPServerResponse& response);

    bool loadClaims();
    bool supportsEntity(const std::string& name, const std::string& version, bool& fulfilled);
    bool supportsService(const std::string& name, const std::string& version);

    void sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                          const nlohmann::json& json,
                          int statusCode = 200);

    void sendErrorResponse(Poco::Net::HTTPServerResponse& response,
                           const std::string& message,
                           int statusCode);
};

} // namespace controllers
} // namespace inventory
