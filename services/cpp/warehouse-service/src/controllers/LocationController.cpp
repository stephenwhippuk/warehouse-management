#include "warehouse/controllers/LocationController.hpp"
#include "warehouse/services/LocationService.hpp"
#include "warehouse/utils/Logger.hpp"
#include <Poco/Net/HTTPResponse.h>

namespace warehouse::controllers {

using namespace Poco::Net;

LocationController::LocationController(std::shared_ptr<services::LocationService> service)
    : service_(service) {
}

void LocationController::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    try {
        const std::string& method = request.getMethod();
        const std::string& uri = request.getURI();
        
        utils::Logger::info("Request: {} {}", method, uri);
        
        if (method == "GET") {
            if (uri.find("/warehouses/") != std::string::npos && uri.find("/locations") != std::string::npos) {
                // Extract warehouse ID for /warehouses/{id}/locations
                std::string warehouseId = ""; // TODO: Extract from path
                handleGetByWarehouse(warehouseId, request, response);
            } else if (uri == "/api/v1/locations") {
                handleGetAll(request, response);
            } else {
                std::string id = extractIdFromPath(uri);
                if (!id.empty()) {
                    handleGetById(id, request, response);
                } else {
                    sendErrorResponse(response, HTTPResponse::HTTP_BAD_REQUEST, "Invalid request");
                }
            }
        } else if (method == "POST" && uri == "/api/v1/locations") {
            handleCreate(request, response);
        } else if (method == "PUT") {
            std::string id = extractIdFromPath(uri);
            if (!id.empty()) {
                handleUpdate(id, request, response);
            } else {
                sendErrorResponse(response, HTTPResponse::HTTP_BAD_REQUEST, "Invalid request");
            }
        } else if (method == "DELETE") {
            std::string id = extractIdFromPath(uri);
            if (!id.empty()) {
                handleDelete(id, request, response);
            } else {
                sendErrorResponse(response, HTTPResponse::HTTP_BAD_REQUEST, "Invalid request");
            }
        } else {
            sendErrorResponse(response, HTTPResponse::HTTP_METHOD_NOT_ALLOWED, "Method not allowed");
        }
    } catch (const std::exception& e) {
        utils::Logger::error("Error handling request: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "Internal server error");
    }
}

void LocationController::handleGetAll(HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement location listing with filters
    sendJsonResponse(response, HTTPResponse::HTTP_OK, "[]");
}

void LocationController::handleGetById(const std::string&, HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement location retrieval by ID
    sendErrorResponse(response, HTTPResponse::HTTP_NOT_FOUND, "Location not found");
}

void LocationController::handleGetByWarehouse(const std::string&, HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement location listing by warehouse
    sendJsonResponse(response, HTTPResponse::HTTP_OK, "[]");
}

void LocationController::handleCreate(HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement location creation
    sendJsonResponse(response, HTTPResponse::HTTP_CREATED, "{\"message\": \"Location created\"}");
}

void LocationController::handleUpdate(const std::string&, HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement location update
    sendJsonResponse(response, HTTPResponse::HTTP_OK, "{\"message\": \"Location updated\"}");
}

void LocationController::handleDelete(const std::string&, HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement location deletion
    sendJsonResponse(response, HTTPResponse::HTTP_NO_CONTENT, "");
}

void LocationController::sendJsonResponse(HTTPServerResponse& response, int status, const std::string& body) {
    response.setStatus(static_cast<HTTPResponse::HTTPStatus>(status));
    response.setContentType("application/json");
    response.setContentLength(body.length());
    
    auto& out = response.send();
    out << body;
}

void LocationController::sendErrorResponse(HTTPServerResponse& response, int status, const std::string& message) {
    nlohmann::json error;
    error["error"] = message;
    error["status"] = status;
    sendJsonResponse(response, status, error.dump());
}

std::string LocationController::extractIdFromPath(const std::string& path) {
    auto pos = path.rfind('/');
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return "";
}

} // namespace warehouse::controllers
