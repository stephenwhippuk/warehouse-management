#include "warehouse/controllers/WarehouseController.hpp"
#include "warehouse/services/WarehouseService.hpp"
#include "warehouse/utils/Logger.hpp"
#include <Poco/Net/HTTPResponse.h>
#include <sstream>

namespace warehouse::controllers {

using namespace Poco::Net;

WarehouseController::WarehouseController(std::shared_ptr<services::WarehouseService> service)
    : service_(service) {
}

void WarehouseController::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    try {
        const std::string& method = request.getMethod();
        const std::string& uri = request.getURI();
        
        utils::Logger::info("Request: {} {}", method, uri);
        
        if (method == "GET") {
            if (uri == "/api/v1/warehouses") {
                handleGetAll(request, response);
            } else {
                std::string id = extractIdFromPath(uri);
                if (!id.empty()) {
                    handleGetById(id, request, response);
                } else {
                    sendErrorResponse(response, HTTPResponse::HTTP_BAD_REQUEST, "Invalid request");
                }
            }
        } else if (method == "POST" && uri == "/api/v1/warehouses") {
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

void WarehouseController::handleGetAll(HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement filtering, pagination, sorting
    sendJsonResponse(response, HTTPResponse::HTTP_OK, "[]");
}

void WarehouseController::handleGetById(const std::string&, HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement warehouse retrieval by ID
    sendErrorResponse(response, HTTPResponse::HTTP_NOT_FOUND, "Warehouse not found");
}

void WarehouseController::handleCreate(HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement warehouse creation
    sendJsonResponse(response, HTTPResponse::HTTP_CREATED, "{\"message\": \"Warehouse created\"}");
}

void WarehouseController::handleUpdate(const std::string&, HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement warehouse update
    sendJsonResponse(response, HTTPResponse::HTTP_OK, "{\"message\": \"Warehouse updated\"}");
}

void WarehouseController::handleDelete(const std::string&, HTTPServerRequest&, HTTPServerResponse& response) {
    // TODO: Implement warehouse deletion
    sendJsonResponse(response, HTTPResponse::HTTP_NO_CONTENT, "");
}

void WarehouseController::sendJsonResponse(HTTPServerResponse& response, int status, const std::string& body) {
    response.setStatus(static_cast<HTTPResponse::HTTPStatus>(status));
    response.setContentType("application/json");
    response.setContentLength(body.length());
    
    auto& out = response.send();
    out << body;
}

void WarehouseController::sendErrorResponse(HTTPServerResponse& response, int status, const std::string& message) {
    nlohmann::json error;
    error["error"] = message;
    error["status"] = status;
    sendJsonResponse(response, status, error.dump());
}

std::string WarehouseController::extractIdFromPath(const std::string& path) {
    // Extract UUID from path like /api/v1/warehouses/{id}
    auto pos = path.rfind('/');
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return "";
}

} // namespace warehouse::controllers
