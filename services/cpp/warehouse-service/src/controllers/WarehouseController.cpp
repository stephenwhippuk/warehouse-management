#include "warehouse/controllers/WarehouseController.hpp"
#include "warehouse/services/IWarehouseService.hpp"
#include "warehouse/models/Warehouse.hpp"
#include "warehouse/utils/Logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace warehouse::controllers {

WarehouseController::WarehouseController()
    : http::ControllerBase("/api/v1/warehouses") {
    
    // GET /api/v1/warehouses (list all)
    Get("/", [this](http::HttpContext& ctx) {
        return this->handleGetAll(ctx);
    });
    
    // GET /api/v1/warehouses/{id} (get by ID)
    Get("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->handleGetById(ctx);
    });
    
    // POST /api/v1/warehouses (create)
    Post("/", [this](http::HttpContext& ctx) {
        return this->handleCreate(ctx);
    });
    
    // PUT /api/v1/warehouses/{id} (update)
    Put("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->handleUpdate(ctx);
    });
    
    // DELETE /api/v1/warehouses/{id} (delete)
    Delete("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->handleDelete(ctx);
    });
}

std::string WarehouseController::handleGetAll(http::HttpContext& ctx) {
    auto service = ctx.getService<services::IWarehouseService>();
    auto dtos = service->getAll();
    json j = json::array();
    for (const auto& dto : dtos) {
        j.push_back(dto.toJson());
    }
    return j.dump();
}

std::string WarehouseController::handleGetById(http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    auto service = ctx.getService<services::IWarehouseService>();
    auto dto = service->getById(id);
    
    if (!dto) {
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        throw std::runtime_error("Warehouse not found");
    }
    
    return dto->toJson().dump();
}

std::string WarehouseController::handleCreate(http::HttpContext& ctx) {
    json body = ctx.getBodyAsJson();
    auto service = ctx.getService<services::IWarehouseService>();
    
    auto warehouse = models::Warehouse::fromJson(body);
    auto dto = service->createWarehouse(warehouse);
    
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
    return dto.toJson().dump();
}

std::string WarehouseController::handleUpdate(http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    json body = ctx.getBodyAsJson();
    body["id"] = id;  // Ensure ID matches route parameter
    
    auto service = ctx.getService<services::IWarehouseService>();
    auto warehouse = models::Warehouse::fromJson(body);
    auto dto = service->updateWarehouse(warehouse);
    
    return dto.toJson().dump();
}

std::string WarehouseController::handleDelete(http::HttpContext& ctx) {
    std::string id = ctx.routeParams["id"];
    auto service = ctx.getService<services::IWarehouseService>();
    
    bool deleted = service->deleteWarehouse(id);
    if (!deleted) {
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        throw std::runtime_error("Warehouse not found");
    }
    
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
    return "";  // No content for 204
}

}  // namespace warehouse::controllers
