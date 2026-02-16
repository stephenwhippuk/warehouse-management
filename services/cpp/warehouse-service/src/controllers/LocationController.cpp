#include "warehouse/controllers/LocationController.hpp"
#include "warehouse/services/ILocationService.hpp"
#include "warehouse/models/Location.hpp"
#include "warehouse/utils/Logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace warehouse::controllers {

LocationController::LocationController()
    : http::ControllerBase("/api/v1/locations") {
    
    // GET /api/v1/locations (list all)
    Get("/", [this](http::HttpContext& ctx) {
        return this->handleGetAll(ctx);
    });
    
    // GET /api/v1/locations/{id} (get by ID)
    Get("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->handleGetById(ctx);
    });
    
    // POST /api/v1/locations (create)
    Post("/", [this](http::HttpContext& ctx) {
        return this->handleCreate(ctx);
    });
    
    // PUT /api/v1/locations/{id} (update)
    Put("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->handleUpdate(ctx);
    });
    
    // DELETE /api/v1/locations/{id} (delete)
    Delete("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->handleDelete(ctx);
    });
}

std::string LocationController::handleGetAll(http::HttpContext& ctx) {
    auto service = ctx.getService<services::ILocationService>();
    auto dtos = service->getAll();
    json j = json::array();
    for (const auto& dto : dtos) {
        j.push_back(dto.toJson());
    }
    return j.dump();
}

std::string LocationController::handleGetById(http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    auto service = ctx.getService<services::ILocationService>();
    auto dto = service->getById(id);
    
    if (!dto) {
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        throw std::runtime_error("Location not found");
    }
    
    return dto->toJson().dump();
}

std::string LocationController::handleCreate(http::HttpContext& ctx) {
    json body = ctx.getBodyAsJson();
    auto service = ctx.getService<services::ILocationService>();
    
    auto location = models::Location::fromJson(body);
    auto dto = service->createLocation(location);
    
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
    return dto.toJson().dump();
}

std::string LocationController::handleUpdate(http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    json body = ctx.getBodyAsJson();
    body["id"] = id;  // Ensure ID matches route parameter
    
    auto service = ctx.getService<services::ILocationService>();
    auto location = models::Location::fromJson(body);
    auto dto = service->updateLocation(location);
    
    return dto.toJson().dump();
}

std::string LocationController::handleDelete(http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    auto service = ctx.getService<services::ILocationService>();
    
    bool deleted = service->deleteLocation(id);
    if (!deleted) {
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        throw std::runtime_error("Location not found");
    }
    
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
    return "";  // No content for 204
}

}  // namespace warehouse::controllers
