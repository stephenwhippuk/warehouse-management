#include "inventory/controllers/InventoryController.hpp"
#include <Poco/URI.h>
#include <Poco/StringTokenizer.h>
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
    auto queryParams = uri.getQueryParameters();
    
    try {
        // Split path into segments
        std::vector<std::string> segments;
        Poco::StringTokenizer tokenizer(path, "/", Poco::StringTokenizer::TOK_IGNORE_EMPTY);
        for (const auto& token : tokenizer) {
            segments.push_back(token);
        }

        // Expect paths starting with /api/v1/inventory
        if (segments.size() >= 3 && segments[0] == "api" && segments[1] == "v1" && segments[2] == "inventory") {
            // GET /api/v1/inventory
            if (method == "GET" && segments.size() == 3) {
                handleGetAll(response);
                return;
            }

            // GET /api/v1/inventory/low-stock?threshold=N
            if (method == "GET" && segments.size() == 4 && segments[3] == "low-stock") {
                int threshold = 0;
                for (const auto& param : queryParams) {
                    if (param.first == "threshold") {
                        threshold = std::stoi(param.second);
                        break;
                    }
                }
                handleGetLowStock(threshold, response);
                return;
            }

            // GET /api/v1/inventory/expired
            if (method == "GET" && segments.size() == 4 && segments[3] == "expired") {
                handleGetExpired(response);
                return;
            }

            // GET /api/v1/inventory/product/:productId
            if (method == "GET" && segments.size() == 5 && segments[3] == "product") {
                handleGetByProduct(segments[4], response);
                return;
            }

            // GET /api/v1/inventory/warehouse/:warehouseId
            if (method == "GET" && segments.size() == 5 && segments[3] == "warehouse") {
                handleGetByWarehouse(segments[4], response);
                return;
            }

            // GET /api/v1/inventory/location/:locationId
            if (method == "GET" && segments.size() == 5 && segments[3] == "location") {
                handleGetByLocation(segments[4], response);
                return;
            }

            // GET /api/v1/inventory/:id
            if (method == "GET" && segments.size() == 4) {
                handleGetById(segments[3], response);
                return;
            }

            // Other methods (create/update/delete/operations) will be wired later
        }

        sendErrorResponse(response, "Not Found", Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
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
