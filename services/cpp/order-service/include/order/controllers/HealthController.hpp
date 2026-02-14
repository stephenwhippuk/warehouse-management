#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

namespace order {
namespace controllers {

class HealthController : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(
        Poco::Net::HTTPServerRequest& request,
        Poco::Net::HTTPServerResponse& response
    ) override;

private:
    void sendHealthResponse(Poco::Net::HTTPServerResponse& response);
};

} // namespace controllers
} // namespace order
