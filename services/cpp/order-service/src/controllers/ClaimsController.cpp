#include "order/controllers/ClaimsController.hpp"
#include "order/utils/Auth.hpp"
#include "order/utils/Logger.hpp"
#include <Poco/Net/HTTPResponse.h>
#include <fstream>
#include <sstream>

namespace order {
namespace controllers {

void ClaimsController::handleRequest(
    Poco::Net::HTTPServerRequest& request,
    Poco::Net::HTTPServerResponse& response
) {
    // Check authentication
    if (!utils::Auth::authorizeServiceRequest(request, response)) {
        return;
    }
    
    std::string uri = request.getURI();
    
    if (uri == "/api/v1/claims") {
        handleGetClaims(response);
    } else if (uri == "/api/v1/claims/fulfilments") {
        handleGetFulfilments(response);
    } else if (uri == "/api/v1/claims/references") {
        handleGetReferences(response);
    } else {
        sendNotFound(response);
    }
}

void ClaimsController::handleGetClaims(Poco::Net::HTTPServerResponse& response) {
    utils::Logger::debug("Getting claims manifest");
    
    std::ifstream file("claims.json");
    if (!file.is_open()) {
        utils::Logger::error("Failed to open claims.json");
        response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        std::ostream& out = response.send();
        out << R"({"error":"Claims file not found"})";
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    sendJsonResponse(response, buffer.str());
}

void ClaimsController::handleGetFulfilments(Poco::Net::HTTPServerResponse& response) {
    utils::Logger::debug("Getting fulfilments");
    
    // For now, return a simplified response
    // In production, parse claims.json and extract fulfilments
    std::string json = R"({
        "fulfilments": [
            {
                "contract": "Order",
                "versions": ["1.0"],
                "status": "fulfilled"
            }
        ]
    })";
    
    sendJsonResponse(response, json);
}

void ClaimsController::handleGetReferences(Poco::Net::HTTPServerResponse& response) {
    utils::Logger::debug("Getting references");
    
    // For now, return a simplified response
    // In production, parse claims.json and extract references
    std::string json = R"({
        "references": [
            {
                "contract": "Product",
                "versions": ["1.0"],
                "requiredFields": ["id", "sku"]
            },
            {
                "contract": "Warehouse",
                "versions": ["1.0"],
                "requiredFields": ["id", "code"]
            }
        ]
    })";
    
    sendJsonResponse(response, json);
}

void ClaimsController::sendJsonResponse(Poco::Net::HTTPServerResponse& response, const std::string& json) {
    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
    response.setContentType("application/json");
    std::ostream& out = response.send();
    out << json;
}

void ClaimsController::sendNotFound(Poco::Net::HTTPServerResponse& response) {
    response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
    response.setContentType("application/json");
    std::ostream& out = response.send();
    out << R"({"error":"Endpoint not found"})";
}

} // namespace controllers
} // namespace order
