#include "order/controllers/OrderController.hpp"
#include "order/services/OrderService.hpp"
#include "order/utils/Auth.hpp"
#include "order/utils/Logger.hpp"
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace order::controllers {

OrderController::OrderController(std::shared_ptr<services::OrderService> service)
    : service_(service) {}

void OrderController::handleRequest(
    Poco::Net::HTTPServerRequest& request,
    Poco::Net::HTTPServerResponse& response
) {
    // Check authentication
    if (!utils::Auth::authorizeServiceRequest(request, response)) {
        return;
    }
    
    std::string method = request.getMethod();
    std::string uri = request.getURI();
    
    utils::Logger::debug("Handling {} request to {}", method, uri);
    
    try {
        // Parse URI to extract ID if present
        Poco::URI parsedUri(uri);
        std::string path = parsedUri.getPath();
        
        // Check for cancel endpoint
        if (path.find("/cancel") != std::string::npos) {
            std::string id = extractIdFromPath(path);
            if (method == "POST") {
                handleCancel(id, request, response);
            } else {
                sendErrorResponse(response, 405, "Method not allowed");
            }
            return;
        }
        
        // Check if path has ID
        bool hasId = path.find("/api/v1/orders/") == 0 && path.length() > 16;
        
        if (method == "GET") {
            if (hasId) {
                std::string id = extractIdFromPath(path);
                handleGetById(id, request, response);
            } else {
                handleGetAll(request, response);
            }
        } else if (method == "POST" && !hasId) {
            handleCreate(request, response);
        } else if (method == "PUT" && hasId) {
            std::string id = extractIdFromPath(path);
            handleUpdate(id, request, response);
        } else {
            sendErrorResponse(response, 405, "Method not allowed");
        }
    } catch (const std::exception& e) {
        utils::Logger::error("Error handling request: {}", e.what());
        sendErrorResponse(response, 500, "Internal server error");
    }
}

void OrderController::handleGetAll(
    Poco::Net::HTTPServerRequest& request,
    Poco::Net::HTTPServerResponse& response
) {
    utils::Logger::info("Listing orders");
    
    // TODO: Implement list orders
    json responseJson = {
        {"orders", json::array()},
        {"total", 0},
        {"page", 1},
        {"pageSize", 50}
    };
    
    sendJsonResponse(response, 200, responseJson.dump());
}

void OrderController::handleGetById(
    const std::string& id,
    Poco::Net::HTTPServerRequest& request,
    Poco::Net::HTTPServerResponse& response
) {
    utils::Logger::info("Getting order by ID: {}", id);
    
    // TODO: Implement get order by ID
    sendErrorResponse(response, 501, "Not implemented");
}

void OrderController::handleCreate(
    Poco::Net::HTTPServerRequest& request,
    Poco::Net::HTTPServerResponse& response
) {
    utils::Logger::info("Creating new order");
    
    try {
        std::istream& input = request.stream();
        json requestBody = json::parse(input);
        
        // TODO: Implement order creation
        utils::Logger::debug("Order creation request: {}", requestBody.dump());
        
        sendErrorResponse(response, 501, "Not implemented");
    } catch (const json::exception& e) {
        utils::Logger::error("JSON parse error: {}", e.what());
        sendErrorResponse(response, 400, "Invalid JSON");
    }
}

void OrderController::handleUpdate(
    const std::string& id,
    Poco::Net::HTTPServerRequest& request,
    Poco::Net::HTTPServerResponse& response
) {
    utils::Logger::info("Updating order: {}", id);
    
    try {
        std::istream& input = request.stream();
        json requestBody = json::parse(input);
        
        // TODO: Implement order update
        utils::Logger::debug("Order update request: {}", requestBody.dump());
        
        sendErrorResponse(response, 501, "Not implemented");
    } catch (const json::exception& e) {
        utils::Logger::error("JSON parse error: {}", e.what());
        sendErrorResponse(response, 400, "Invalid JSON");
    }
}

void OrderController::handleCancel(
    const std::string& id,
    Poco::Net::HTTPServerRequest& request,
    Poco::Net::HTTPServerResponse& response
) {
    utils::Logger::info("Cancelling order: {}", id);
    
    try {
        std::istream& input = request.stream();
        json requestBody = json::parse(input);
        
        // TODO: Implement order cancellation
        utils::Logger::debug("Order cancellation request: {}", requestBody.dump());
        
        sendErrorResponse(response, 501, "Not implemented");
    } catch (const json::exception& e) {
        utils::Logger::error("JSON parse error: {}", e.what());
        sendErrorResponse(response, 400, "Invalid JSON");
    }
}

void OrderController::sendJsonResponse(
    Poco::Net::HTTPServerResponse& response,
    int status,
    const std::string& body
) {
    response.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(status));
    response.setContentType("application/json");
    response.setContentLength(body.length());
    
    std::ostream& out = response.send();
    out << body;
}

void OrderController::sendErrorResponse(
    Poco::Net::HTTPServerResponse& response,
    int status,
    const std::string& message
) {
    json errorJson = {
        {"error", message},
        {"status", status}
    };
    
    sendJsonResponse(response, status, errorJson.dump());
}

std::string OrderController::extractIdFromPath(const std::string& path) {
    // Extract UUID from path like /api/v1/orders/{id} or /api/v1/orders/{id}/cancel
    size_t lastSlash = path.find_last_of('/');
    size_t secondLastSlash = path.find_last_of('/', lastSlash - 1);
    
    // Check if last part is "cancel"
    if (path.substr(lastSlash + 1) == "cancel") {
        size_t thirdLastSlash = path.find_last_of('/', secondLastSlash - 1);
        return path.substr(thirdLastSlash + 1, secondLastSlash - thirdLastSlash - 1);
    }
    
    return path.substr(secondLastSlash + 1, lastSlash - secondLastSlash - 1);
}

} // namespace order::controllers
