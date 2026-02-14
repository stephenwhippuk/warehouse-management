#include "product/controllers/ProductController.hpp"
#include "product/utils/Logger.hpp"
#include "product/dtos/ErrorDto.hpp"
#include <nlohmann/json.hpp>
#include <Poco/URI.h>
#include <Poco/StringTokenizer.h>
#include <sstream>
#include <regex>

using json = nlohmann::json;

namespace product::controllers {

ProductController::ProductController(std::shared_ptr<services::ProductService> service)
    : service_(service) {
    if (!service_) {
        throw std::invalid_argument("Service cannot be null");
    }
}

void ProductController::handleRequest(Poco::Net::HTTPServerRequest& request,
                                     Poco::Net::HTTPServerResponse& response) {
    std::string method = request.getMethod();
    std::string uri = request.getURI();
    
    if (auto logger = utils::Logger::getLogger()) logger->debug("Request: {} {}", method, uri);
    
    try {
        // Parse URI path
        Poco::URI parsed(uri);
        std::string path = parsed.getPath();
        
        // Extract query parameters
        int page = 1;
        int pageSize = 50;
        if (!parsed.getQuery().empty()) {
            std::string query = parsed.getQuery();
            if (query.find("page=") != std::string::npos) {
                size_t pos = query.find("page=") + 5;
                page = std::stoi(query.substr(pos, query.find("&", pos) - pos));
            }
            if (query.find("pageSize=") != std::string::npos) {
                size_t pos = query.find("pageSize=") + 9;
                pageSize = std::stoi(query.substr(pos, query.find("&", pos) - pos));
            }
        }
        
        // Route handling
        if (method == "GET") {
            if (path == "/api/v1/products") {
                handleGetAll(page, pageSize, request, response);
            } else if (path.find("/api/v1/products/") == 0) {
                std::string id = path.substr(17);  // "/api/v1/products/".length() == 17
                handleGetById(id, request, response);
            } else {
                sendErrorResponse(response, "NotFound", "Endpoint not found", 404);
            }
        } else if (method == "POST") {
            if (path == "/api/v1/products") {
                handleCreate(request, response);
            } else {
                sendErrorResponse(response, "NotFound", "Endpoint not found", 404);
            }
        } else if (method == "PUT") {
            if (path.find("/api/v1/products/") == 0) {
                std::string id = path.substr(17);
                handleUpdate(id, request, response);
            } else {
                sendErrorResponse(response, "NotFound", "Endpoint not found", 404);
            }
        } else if (method == "DELETE") {
            if (path.find("/api/v1/products/") == 0) {
                std::string id = path.substr(17);
                handleDelete(id, request, response);
            } else {
                sendErrorResponse(response, "NotFound", "Endpoint not found", 404);
            }
        } else {
            sendErrorResponse(response, "MethodNotAllowed", "HTTP method not allowed", 405);
        }
    } catch (const std::exception& e) {
        if (auto logger = utils::Logger::getLogger()) logger->error("Request handling error: {}", e.what());
        sendErrorResponse(response, "InternalServerError", e.what(), 500);
    }
}

void ProductController::handleGetAll(int page, int pageSize, 
                                    Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response) {
    auto list = service_->getAll(page, pageSize);
    sendJsonResponse(response, list.toJson().dump(), 200);
}

void ProductController::handleGetById(const std::string& id,
                                     Poco::Net::HTTPServerRequest& request,
                                     Poco::Net::HTTPServerResponse& response) {
    auto product = service_->getById(id);
    
    if (!product) {
        sendErrorResponse(response, "NotFound", "Product not found: " + id, 404);
        return;
    }
    
    sendJsonResponse(response, product->toJson().dump(), 200);
}

void ProductController::handleCreate(Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response) {
    try {
        json body;
        request.stream() >> body;
        
        std::string sku = body.at("sku").get<std::string>();
        std::string name = body.at("name").get<std::string>();
        std::optional<std::string> description = std::nullopt;
        std::optional<std::string> category = std::nullopt;
        
        if (body.contains("description") && !body["description"].is_null()) {
            description = body["description"].get<std::string>();
        }
        if (body.contains("category") && !body["category"].is_null()) {
            category = body["category"].get<std::string>();
        }
        
        auto product = service_->create(sku, name, description, category);
        sendJsonResponse(response, product.toJson().dump(), 201);
    } catch (const std::invalid_argument& e) {
        sendErrorResponse(response, "BadRequest", e.what(), 400);
    } catch (const std::exception& e) {
        sendErrorResponse(response, "InternalServerError", e.what(), 500);
    }
}

void ProductController::handleUpdate(const std::string& id,
                                    Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response) {
    try {
        json body;
        request.stream() >> body;
        
        std::string name = body.at("name").get<std::string>();
        std::string status = body.at("status").get<std::string>();
        std::optional<std::string> description = std::nullopt;
        std::optional<std::string> category = std::nullopt;
        
        if (body.contains("description") && !body["description"].is_null()) {
            description = body["description"].get<std::string>();
        }
        if (body.contains("category") && !body["category"].is_null()) {
            category = body["category"].get<std::string>();
        }
        
        auto product = service_->update(id, name, description, category, status);
        sendJsonResponse(response, product.toJson().dump(), 200);
    } catch (const std::invalid_argument& e) {
        sendErrorResponse(response, "BadRequest", e.what(), 400);
    } catch (const std::runtime_error& e) {
        if (e.what() == std::string("Product not found: " + id)) {
            sendErrorResponse(response, "NotFound", e.what(), 404);
        } else {
            sendErrorResponse(response, "InternalServerError", e.what(), 500);
        }
    } catch (const std::exception& e) {
        sendErrorResponse(response, "InternalServerError", e.what(), 500);
    }
}

void ProductController::handleDelete(const std::string& id,
                                    Poco::Net::HTTPServerRequest& request,
                                    Poco::Net::HTTPServerResponse& response) {
    try {
        bool deleted = service_->deleteById(id);
        
        if (!deleted) {
            sendErrorResponse(response, "NotFound", "Product not found: " + id, 404);
            return;
        }
        
        response.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
        response.send();
    } catch (const std::exception& e) {
        sendErrorResponse(response, "InternalServerError", e.what(), 500);
    }
}

void ProductController::sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                                        const std::string& json, int status) {
    response.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(status));
    response.setContentType("application/json");
    response.send() << json;
}

void ProductController::sendErrorResponse(Poco::Net::HTTPServerResponse& response,
                                         const std::string& error,
                                         const std::string& message,
                                         int status) {
    try {
        dtos::ErrorDto errorDto(error, message);
        sendJsonResponse(response, errorDto.toJson().dump(), status);
    } catch (...) {
        response.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(status));
        response.setContentType("application/json");
        response.send() << "{\"error\": \"" << error << "\", \"message\": \"" << message << "\"}";
    }
}

std::string ProductController::extractPathParameter(const std::string& uri,
                                                   const std::string& paramName) {
    std::regex pattern("\\{" + paramName + "\\}");
    size_t pos = uri.find("{" + paramName + "}");
    if (pos != std::string::npos) {
        return uri.substr(pos + paramName.length() + 2);
    }
    return "";
}

}  // namespace product::controllers
