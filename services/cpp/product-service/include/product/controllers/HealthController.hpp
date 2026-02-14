#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <memory>

namespace product::controllers {

/**
 * @brief Health check endpoint handler
 */
class HealthController : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;
};

}  // namespace product::controllers
