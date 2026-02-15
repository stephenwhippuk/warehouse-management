#include "inventory/controllers/InventoryController.hpp"
#include "inventory/utils/Auth.hpp"
#include "inventory/models/Inventory.hpp"
#include <http-framework/HttpException.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace inventory {
namespace controllers {

InventoryController::InventoryController(std::shared_ptr<services::InventoryService> service)
    : ControllerBase("/api/v1/inventory"), service_(service) {
    
    // List and filter endpoints - order matters: specific routes before parameterized routes
    Get("/low-stock", [this](http::HttpContext& ctx) {
        return handleGetLowStock(ctx);
    });

    Get("/expired", [this](http::HttpContext& ctx) {
        return handleGetExpired(ctx);
    });

    Get("/product/{productId:uuid}", [this](http::HttpContext& ctx) {
        return handleGetByProduct(ctx);
    });

    Get("/warehouse/{warehouseId:uuid}", [this](http::HttpContext& ctx) {
        return handleGetByWarehouse(ctx);
    });

    Get("/location/{locationId:uuid}", [this](http::HttpContext& ctx) {
        return handleGetByLocation(ctx);
    });

    Get("/", [this](http::HttpContext& ctx) {
        return handleGetAll(ctx);
    });

    // CRUD endpoints
    Get("/{id:uuid}", [this](http::HttpContext& ctx) {
        return handleGetById(ctx);
    });

    Post("/", [this](http::HttpContext& ctx) {
        return handleCreate(ctx);
    });

    Put("/{id:uuid}", [this](http::HttpContext& ctx) {
        return handleUpdate(ctx);
    });

    Delete("/{id:uuid}", [this](http::HttpContext& ctx) {
        return handleDelete(ctx);
    });

    // Stock operation endpoints
    Post("/{id:uuid}/reserve", [this](http::HttpContext& ctx) {
        return handleReserve(ctx);
    });

    Post("/{id:uuid}/release", [this](http::HttpContext& ctx) {
        return handleRelease(ctx);
    });

    Post("/{id:uuid}/allocate", [this](http::HttpContext& ctx) {
        return handleAllocate(ctx);
    });

    Post("/{id:uuid}/deallocate", [this](http::HttpContext& ctx) {
        return handleDeallocate(ctx);
    });

    Post("/{id:uuid}/adjust", [this](http::HttpContext& ctx) {
        return handleAdjust(ctx);
    });
}

// ============================================================================
// Helper Methods
// ============================================================================

void InventoryController::requireServiceAuth(http::HttpContext& ctx) {
    auto authStatus = utils::Auth::authorizeServiceRequest(ctx.request);
    if (authStatus == utils::AuthStatus::MissingToken) {
        throw http::UnauthorizedException("Missing service authentication");
    }
    if (authStatus == utils::AuthStatus::InvalidToken) {
        throw http::ForbiddenException("Invalid service authentication");
    }
}

// ============================================================================
// Handler Methods
// ============================================================================

std::string InventoryController::handleGetAll(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    auto inventories = service_->getAll();
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    return j.dump();
}

std::string InventoryController::handleGetById(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string id = ctx.routeParams["id"];
    auto inventory = service_->getById(id);
    if (!inventory) {
        throw http::NotFoundException("Inventory not found: " + id);
    }
    return inventory->toJson().dump();
}

std::string InventoryController::handleGetByProduct(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string productId = ctx.routeParams["productId"];
    auto inventories = service_->getByProductId(productId);
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    return j.dump();
}

std::string InventoryController::handleGetByWarehouse(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string warehouseId = ctx.routeParams["warehouseId"];
    auto inventories = service_->getByWarehouseId(warehouseId);
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    return j.dump();
}

std::string InventoryController::handleGetByLocation(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string locationId = ctx.routeParams["locationId"];
    auto inventories = service_->getByLocationId(locationId);
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    return j.dump();
}

std::string InventoryController::handleGetLowStock(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    int threshold = 0;
    if (ctx.queryParams.has("threshold")) {
        threshold = std::stoi(ctx.queryParams.get("threshold"));
    }

    auto inventories = service_->getLowStock(threshold);
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    return j.dump();
}

std::string InventoryController::handleGetExpired(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    auto inventories = service_->getExpired();
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    return j.dump();
}

std::string InventoryController::handleCreate(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    auto body = ctx.getBodyAsJson();
    auto inventory = models::Inventory::fromJson(body);
    auto created = service_->create(inventory);
    
    ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
    return created.toJson().dump();
}

std::string InventoryController::handleUpdate(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string id = ctx.routeParams["id"];
    auto body = ctx.getBodyAsJson();
    auto inventory = models::Inventory::fromJson(body);
    
    if (inventory.getId() != id) {
        throw http::BadRequestException("ID in path does not match ID in body");
    }

    auto updated = service_->update(inventory);
    return updated.toJson().dump();
}

std::string InventoryController::handleDelete(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string id = ctx.routeParams["id"];
    bool deleted = service_->remove(id);
    
    if (deleted) {
        ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
        return "";
    }
    
    throw http::InternalServerErrorException("Failed to delete inventory");
}

std::string InventoryController::handleReserve(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string id = ctx.routeParams["id"];
    auto body = ctx.getBodyAsJson();
    
    if (!body.contains("quantity") || !body["quantity"].is_number_integer()) {
        throw http::BadRequestException("Missing or invalid 'quantity' field");
    }

    int quantity = body["quantity"].get<int>();
    auto result = service_->reserve(id, quantity);
    return result.toJson().dump();
}

std::string InventoryController::handleRelease(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string id = ctx.routeParams["id"];
    auto body = ctx.getBodyAsJson();
    
    if (!body.contains("quantity") || !body["quantity"].is_number_integer()) {
        throw http::BadRequestException("Missing or invalid 'quantity' field");
    }

    int quantity = body["quantity"].get<int>();
    auto result = service_->release(id, quantity);
    return result.toJson().dump();
}

std::string InventoryController::handleAllocate(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string id = ctx.routeParams["id"];
    auto body = ctx.getBodyAsJson();
    
    if (!body.contains("quantity") || !body["quantity"].is_number_integer()) {
        throw http::BadRequestException("Missing or invalid 'quantity' field");
    }

    int quantity = body["quantity"].get<int>();
    auto result = service_->allocate(id, quantity);
    return result.toJson().dump();
}

std::string InventoryController::handleDeallocate(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string id = ctx.routeParams["id"];
    auto body = ctx.getBodyAsJson();
    
    if (!body.contains("quantity") || !body["quantity"].is_number_integer()) {
        throw http::BadRequestException("Missing or invalid 'quantity' field");
    }

    int quantity = body["quantity"].get<int>();
    auto result = service_->deallocate(id, quantity);
    return result.toJson().dump();
}

std::string InventoryController::handleAdjust(http::HttpContext& ctx) {
    requireServiceAuth(ctx);
    
    std::string id = ctx.routeParams["id"];
    auto body = ctx.getBodyAsJson();
    
    if (!body.contains("quantityChange") || !body["quantityChange"].is_number_integer()) {
        throw http::BadRequestException("Missing or invalid 'quantityChange' field");
    }
    if (!body.contains("reason") || !body["reason"].is_string()) {
        throw http::BadRequestException("Missing or invalid 'reason' field");
    }

    int quantityChange = body["quantityChange"].get<int>();
    std::string reason = body["reason"].get<std::string>();
    auto result = service_->adjust(id, quantityChange, reason);
    return result.toJson().dump();
}

} // namespace controllers
} // namespace inventory
