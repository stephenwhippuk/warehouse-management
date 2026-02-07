#include "inventory/controllers/InventoryController.hpp"
#include <Poco/URI.h>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace inventory {
namespace controllers {

InventoryController::InventoryController(std::shared_ptr<services::InventoryService> service)
    : service_(service) {}

void InventoryController::handleRequest(Poco::Net::HTTPServerRequest& request,
                                       Poco::Net::HTTPServerResponse& response) {
    std::string method = request.getMethod();
    Poco::URI uri(request.getURI());
    std::string path = uri.getPath();
    
    try {
        // TODO: Implement proper routing logic
        // Routes:
        // GET    /api/v1/inventory - Get all
        // GET    /api/v1/inventory/:id - Get by ID
        // GET    /api/v1/inventory/product/:productId - Get by product
        // GET    /api/v1/inventory/warehouse/:warehouseId - Get by warehouse
        // GET    /api/v1/inventory/location/:locationId - Get by location
        // GET    /api/v1/inventory/low-stock?threshold=N - Get low stock
        // GET    /api/v1/inventory/expired - Get expired
        // POST   /api/v1/inventory - Create
        // PUT    /api/v1/inventory/:id - Update
        // DELETE /api/v1/inventory/:id - Delete
        // POST   /api/v1/inventory/:id/reserve - Reserve quantity
        // POST   /api/v1/inventory/:id/release - Release quantity
        // POST   /api/v1/inventory/:id/allocate - Allocate quantity
        // POST   /api/v1/inventory/:id/deallocate - Deallocate quantity
        // POST   /api/v1/inventory/:id/adjust - Adjust quantity
        
        sendJsonResponse(response, R"({"message": "Inventory Controller - TODO: Implement routing"})");
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void InventoryController::handleGetAll(Poco::Net::HTTPServerResponse& response) {
    auto inventories = service_->getAll();
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    sendJsonResponse(response, j.dump());
}

void InventoryController::handleGetById(const std::string& id, Poco::Net::HTTPServerResponse& response) {
    auto inventory = service_->getById(id);
    if (!inventory) {
        sendErrorResponse(response, "Inventory not found", Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        return;
    }
    sendJsonResponse(response, inventory->toJson().dump());
}

void InventoryController::handleGetByProduct(const std::string& productId, Poco::Net::HTTPServerResponse& response) {
    auto inventories = service_->getByProductId(productId);
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    sendJsonResponse(response, j.dump());
}

void InventoryController::handleGetByWarehouse(const std::string& warehouseId, Poco::Net::HTTPServerResponse& response) {
    auto inventories = service_->getByWarehouseId(warehouseId);
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    sendJsonResponse(response, j.dump());
}

void InventoryController::handleGetByLocation(const std::string& locationId, Poco::Net::HTTPServerResponse& response) {
    auto inventories = service_->getByLocationId(locationId);
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    sendJsonResponse(response, j.dump());
}

void InventoryController::handleGetLowStock(int threshold, Poco::Net::HTTPServerResponse& response) {
    auto inventories = service_->getLowStock(threshold);
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    sendJsonResponse(response, j.dump());
}

void InventoryController::handleGetExpired(Poco::Net::HTTPServerResponse& response) {
    auto inventories = service_->getExpired();
    json j = json::array();
    for (const auto& inv : inventories) {
        j.push_back(inv.toJson());
    }
    sendJsonResponse(response, j.dump());
}

void InventoryController::handleCreate(Poco::Net::HTTPServerRequest& request, 
                                      Poco::Net::HTTPServerResponse& response) {
    // TODO: Parse request body
    // auto inventory = models::Inventory::fromJson(json::parse(request.stream()));
    // auto created = service_->create(inventory);
    // sendJsonResponse(response, created.toJson().dump(), Poco::Net::HTTPResponse::HTTP_CREATED);
}

void InventoryController::handleUpdate(const std::string& id, 
                                      Poco::Net::HTTPServerRequest& request,
                                      Poco::Net::HTTPServerResponse& response) {
    // TODO: Parse request body and update
}

void InventoryController::handleDelete(const std::string& id, Poco::Net::HTTPServerResponse& response) {
    bool deleted = service_->remove(id);
    if (deleted) {
        sendJsonResponse(response, R"({"message": "Inventory deleted"})", 
                        Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
    } else {
        sendErrorResponse(response, "Failed to delete inventory", 
                         Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void InventoryController::handleReserve(const std::string& id,
                                       Poco::Net::HTTPServerRequest& request,
                                       Poco::Net::HTTPServerResponse& response) {
    // TODO: Parse quantity from request body
    // service_->reserve(id, quantity);
}

void InventoryController::handleRelease(const std::string& id,
                                       Poco::Net::HTTPServerRequest& request,
                                       Poco::Net::HTTPServerResponse& response) {
    // TODO: Parse quantity from request body
    // service_->release(id, quantity);
}

void InventoryController::handleAllocate(const std::string& id,
                                        Poco::Net::HTTPServerRequest& request,
                                        Poco::Net::HTTPServerResponse& response) {
    // TODO: Parse quantity from request body
    // service_->allocate(id, quantity);
}

void InventoryController::handleDeallocate(const std::string& id,
                                          Poco::Net::HTTPServerRequest& request,
                                          Poco::Net::HTTPServerResponse& response) {
    // TODO: Parse quantity from request body
    // service_->deallocate(id, quantity);
}

void InventoryController::handleAdjust(const std::string& id,
                                      Poco::Net::HTTPServerRequest& request,
                                      Poco::Net::HTTPServerResponse& response) {
    // TODO: Parse quantityChange and reason from request body
    // service_->adjust(id, quantityChange, reason);
}

void InventoryController::sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                                          const std::string& json,
                                          Poco::Net::HTTPResponse::HTTPStatus status) {
    response.setStatus(status);
    response.setContentType("application/json");
    std::ostream& out = response.send();
    out << json;
}

void InventoryController::sendErrorResponse(Poco::Net::HTTPServerResponse& response,
                                           const std::string& message,
                                           Poco::Net::HTTPResponse::HTTPStatus status) {
    json error = {
        {"error", message},
        {"status", status}
    };
    sendJsonResponse(response, error.dump(), status);
}

} // namespace controllers
} // namespace inventory
