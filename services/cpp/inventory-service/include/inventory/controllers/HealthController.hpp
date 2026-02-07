#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

namespace inventory {
namespace controllers {

/**
 * @brief Simple health check controller
 *
 * Exposes a lightweight /health endpoint that reports basic
 * service status along with authentication metrics.
 */
class HealthController : public Poco::Net::HTTPRequestHandler {
public:
    HealthController() = default;

    void handleRequest(Poco::Net::HTTPServerRequest& request,
                       Poco::Net::HTTPServerResponse& response) override;

private:
    void sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                          const std::string& jsonContent,
                          int statusCode = 200);
};

} // namespace controllers
} // namespace inventory
