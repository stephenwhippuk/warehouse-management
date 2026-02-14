#include "warehouse/controllers/HealthController.hpp"
#include "warehouse/utils/Auth.hpp"
#include "warehouse/utils/Logger.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace warehouse {
namespace controllers {

void HealthController::handleRequest(Poco::Net::HTTPServerRequest& request,
                                     Poco::Net::HTTPServerResponse& response) {
    (void)request; // Unused for now

    utils::Logger::debug("Health check requested");

    json payload;
    payload["status"] = "ok";
    payload["service"] = "warehouse-service";

    json authMetrics;
    authMetrics["authorized"] = utils::Auth::authorizedCount();
    authMetrics["missingToken"] = utils::Auth::missingTokenCount();
    authMetrics["invalidToken"] = utils::Auth::invalidTokenCount();

    payload["auth"] = authMetrics;

    sendJsonResponse(response, payload.dump(), 200);
}

void HealthController::sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                                        const std::string& jsonContent,
                                        int statusCode) {
    response.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(statusCode));
    response.setContentType("application/json");
    response.setChunkedTransferEncoding(true);

    std::ostream& out = response.send();
    out << jsonContent;
}

} // namespace controllers
} // namespace warehouse
