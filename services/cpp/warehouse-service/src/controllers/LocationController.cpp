#include "warehouse/controllers/LocationController.hpp"
#include "warehouse/services/LocationService.hpp"
#include "warehouse/dtos/ErrorDto.hpp"
#include "warehouse/models/Location.hpp"
#include "warehouse/utils/Auth.hpp"
#include "warehouse/utils/Logger.hpp"
#include <Poco/Net/HTTPResponse.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace warehouse::controllers {

using namespace Poco::Net;

LocationController::LocationController(std::shared_ptr<services::LocationService> service)
    : service_(service) {
}

void LocationController::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    // Service-to-service authentication
    auto authStatus = utils::Auth::authorizeServiceRequest(request);
    if (authStatus == utils::AuthStatus::MissingToken) {
        sendErrorResponse(response, HTTPResponse::HTTP_UNAUTHORIZED, "Missing service authentication");
        return;
    }
    if (authStatus == utils::AuthStatus::InvalidToken) {
        sendErrorResponse(response, HTTPResponse::HTTP_FORBIDDEN, "Invalid service authentication");
        return;
    }

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
    try {
        auto dtos = service_->getAll();
        json j = json::array();
        for (const auto& dto : dtos) {
            j.push_back(dto.toJson());
        }
        sendJsonResponse(response, HTTPResponse::HTTP_OK, j.dump());
    } catch (const std::exception& e) {
        utils::Logger::error("Error in handleGetAll: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "Failed to retrieve locations");
    }
}

void LocationController::handleGetById(const std::string& id, HTTPServerRequest&, HTTPServerResponse& response) {
    try {
        auto dto = service_->getById(id);
        if (!dto) {
            sendErrorResponse(response, HTTPResponse::HTTP_NOT_FOUND, "Location not found");
            return;
        }
        sendJsonResponse(response, HTTPResponse::HTTP_OK, dto->toJson().dump());
    } catch (const std::exception& e) {
        utils::Logger::error("Error in handleGetById: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "Failed to retrieve location");
    }
}

void LocationController::handleGetByWarehouse(const std::string& warehouseId, HTTPServerRequest&, HTTPServerResponse& response) {
    try {
        auto dtos = service_->getByWarehouseId(warehouseId);
        json j = json::array();
        for (const auto& dto : dtos) {
            j.push_back(dto.toJson());
        }
        sendJsonResponse(response, HTTPResponse::HTTP_OK, j.dump());
    } catch (const std::exception& e) {
        utils::Logger::error("Error in handleGetByWarehouse: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "Failed to retrieve locations");
    }
}

void LocationController::handleCreate(HTTPServerRequest& request, HTTPServerResponse& response) {
    try {
        std::istream& input = request.stream();
        json requestBody = json::parse(input);
        
        // Create Location model from JSON
        auto location = models::Location::fromJson(requestBody);
        
        // Call service which returns LocationDto
        auto dto = service_->createLocation(location);
        
        sendJsonResponse(response, HTTPResponse::HTTP_CREATED, dto.toJson().dump());
    } catch (const json::exception& e) {
        utils::Logger::error("JSON parse error in handleCreate: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_BAD_REQUEST, "Invalid JSON");
    } catch (const std::invalid_argument& e) {
        utils::Logger::error("Validation error in handleCreate: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_BAD_REQUEST, e.what());
    } catch (const std::exception& e) {
        utils::Logger::error("Error in handleCreate: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "Failed to create location");
    }
}

void LocationController::handleUpdate(const std::string& id, HTTPServerRequest& request, HTTPServerResponse& response) {
    try {
        std::istream& input = request.stream();
        json requestBody = json::parse(input);
        
        // Ensure id is set in the JSON
        requestBody["id"] = id;
        
        // Create Location model from JSON
        auto location = models::Location::fromJson(requestBody);
        
        // Call service which returns LocationDto
        auto dto = service_->updateLocation(location);
        
        sendJsonResponse(response, HTTPResponse::HTTP_OK, dto.toJson().dump());
    } catch (const json::exception& e) {
        utils::Logger::error("JSON parse error in handleUpdate: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_BAD_REQUEST, "Invalid JSON");
    } catch (const std::invalid_argument& e) {
        utils::Logger::error("Validation error in handleUpdate: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_BAD_REQUEST, e.what());
    } catch (const std::runtime_error& e) {
        utils::Logger::error("Error in handleUpdate: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_NOT_FOUND, e.what());
    } catch (const std::exception& e) {
        utils::Logger::error("Error in handleUpdate: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "Failed to update location");
    }
}

void LocationController::handleDelete(const std::string& id, HTTPServerRequest&, HTTPServerResponse& response) {
    try {
        bool deleted = service_->deleteLocation(id);
        if (deleted) {
            response.setStatus(HTTPResponse::HTTP_NO_CONTENT);
            response.send();
        } else {
            sendErrorResponse(response, HTTPResponse::HTTP_NOT_FOUND, "Location not found");
        }
    } catch (const std::exception& e) {
        utils::Logger::error("Error in handleDelete: {}", e.what());
        sendErrorResponse(response, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, "Failed to delete location");
    }
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
