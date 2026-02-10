#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

namespace order {
namespace controllers {

class ClaimsController : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(
        Poco::Net::HTTPServerRequest& request,
        Poco::Net::HTTPServerResponse& response
    ) override;

private:
    void handleGetClaims(Poco::Net::HTTPServerResponse& response);
    void handleGetFulfilments(Poco::Net::HTTPServerResponse& response);
    void handleGetReferences(Poco::Net::HTTPServerResponse& response);
    void sendJsonResponse(Poco::Net::HTTPServerResponse& response, const std::string& json);
    void sendNotFound(Poco::Net::HTTPServerResponse& response);
};

} // namespace controllers
} // namespace order
