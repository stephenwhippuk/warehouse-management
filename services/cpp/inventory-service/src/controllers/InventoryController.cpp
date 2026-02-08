#include "inventory/controllers/InventoryController.hpp"
#include "inventory/utils/Auth.hpp"
#include "inventory/models/Inventory.hpp"
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
    // Service-to-service authentication
    auto authStatus = utils::Auth::authorizeServiceRequest(request);
    if (authStatus == utils::AuthStatus::MissingToken) {
        sendErrorResponse(response, "Missing service authentication", Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
        return;
    }
    if (authStatus == utils::AuthStatus::InvalidToken) {
        sendErrorResponse(response, "Invalid service authentication", Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
        return;
    }

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
            // POST /api/v1/inventory
            if (method == "POST" && segments.size() == 3) {
                handleCreate(request, response);
                return;
            }

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

            // PUT /api/v1/inventory/:id
            if (method == "PUT" && segments.size() == 4) {
                handleUpdate(segments[3], request, response);
                return;
            }

            // DELETE /api/v1/inventory/:id
            if (method == "DELETE" && segments.size() == 4) {
                handleDelete(segments[3], response);
                return;
            }

            // POST /api/v1/inventory/:id/reserve
            if (method == "POST" && segments.size() == 5 && segments[4] == "reserve") {
                handleReserve(segments[3], request, response);
                return;
            }

            // POST /api/v1/inventory/:id/release
            if (method == "POST" && segments.size() == 5 && segments[4] == "release") {
                handleRelease(segments[3], request, response);
                return;
            }

            // POST /api/v1/inventory/:id/allocate
            if (method == "POST" && segments.size() == 5 && segments[4] == "allocate") {
                handleAllocate(segments[3], request, response);
                return;
            }

            // POST /api/v1/inventory/:id/deallocate
            if (method == "POST" && segments.size() == 5 && segments[4] == "deallocate") {
                handleDeallocate(segments[3], request, response);
                return;
            }

            // POST /api/v1/inventory/:id/adjust
            if (method == "POST" && segments.size() == 5 && segments[4] == "adjust") {
                handleAdjust(segments[3], request, response);
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
    try {
        std::istream& bodyStream = request.stream();
        json body;
        bodyStream >> body;

        auto inventory = models::Inventory::fromJson(body);
        auto created = service_->create(inventory);

        sendJsonResponse(
            response,
            created.toJson().dump(),
            Poco::Net::HTTPResponse::HTTP_CREATED
        );
    } catch (const json::exception& e) {
        sendErrorResponse(response, std::string("Invalid JSON body: ") + e.what(),
                          Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::invalid_argument& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void InventoryController::handleUpdate(const std::string& id, 
                                      Poco::Net::HTTPServerRequest& request,
                                      Poco::Net::HTTPServerResponse& response) {
    try {
        std::istream& bodyStream = request.stream();
        json body;
        bodyStream >> body;

        auto inventory = models::Inventory::fromJson(body);
        if (inventory.getId() != id) {
            sendErrorResponse(response,
                              "ID in path does not match ID in body",
                              Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }

        auto updated = service_->update(inventory);
        sendJsonResponse(response, updated.toJson().dump(), Poco::Net::HTTPResponse::HTTP_OK);
    } catch (const json::exception& e) {
        sendErrorResponse(response, std::string("Invalid JSON body: ") + e.what(),
                          Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::invalid_argument& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::runtime_error& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void InventoryController::handleDelete(const std::string& id, Poco::Net::HTTPServerResponse& response) {
    try {
        bool deleted = service_->remove(id);
        if (deleted) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
            response.setContentLength(0);
            response.send();
        } else {
            sendErrorResponse(response,
                              "Failed to delete inventory",
                              Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        }
    } catch (const std::runtime_error& e) {
        // Distinguish not found from other business rule violations
        std::string message = e.what();
        if (message.rfind("Inventory not found", 0) == 0) {
            sendErrorResponse(response, message, Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        } else {
            sendErrorResponse(response, message, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        }
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void InventoryController::handleReserve(const std::string& id,
                                       Poco::Net::HTTPServerRequest& request,
                                       Poco::Net::HTTPServerResponse& response) {
    try {
        std::istream& bodyStream = request.stream();
        json body;
        bodyStream >> body;

        if (!body.contains("quantity") || !body["quantity"].is_number_integer()) {
            sendErrorResponse(response, "Missing or invalid 'quantity' field",
                              Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }

        int quantity = body["quantity"].get<int>();
        service_->reserve(id, quantity);

        auto updated = service_->getById(id);
        if (!updated) {
            sendErrorResponse(response, "Inventory not found after reserve",
                              Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        sendJsonResponse(response, updated->toJson().dump(),
                         Poco::Net::HTTPResponse::HTTP_OK);
    } catch (const json::exception& e) {
        sendErrorResponse(response, std::string("Invalid JSON body: ") + e.what(),
                          Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::invalid_argument& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::runtime_error& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void InventoryController::handleRelease(const std::string& id,
                                       Poco::Net::HTTPServerRequest& request,
                                       Poco::Net::HTTPServerResponse& response) {
    try {
        std::istream& bodyStream = request.stream();
        json body;
        bodyStream >> body;

        if (!body.contains("quantity") || !body["quantity"].is_number_integer()) {
            sendErrorResponse(response, "Missing or invalid 'quantity' field",
                              Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }

        int quantity = body["quantity"].get<int>();
        service_->release(id, quantity);

        auto updated = service_->getById(id);
        if (!updated) {
            sendErrorResponse(response, "Inventory not found after release",
                              Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        sendJsonResponse(response, updated->toJson().dump(),
                         Poco::Net::HTTPResponse::HTTP_OK);
    } catch (const json::exception& e) {
        sendErrorResponse(response, std::string("Invalid JSON body: ") + e.what(),
                          Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::invalid_argument& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::runtime_error& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void InventoryController::handleAllocate(const std::string& id,
                                        Poco::Net::HTTPServerRequest& request,
                                        Poco::Net::HTTPServerResponse& response) {
    try {
        std::istream& bodyStream = request.stream();
        json body;
        bodyStream >> body;

        if (!body.contains("quantity") || !body["quantity"].is_number_integer()) {
            sendErrorResponse(response, "Missing or invalid 'quantity' field",
                              Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }

        int quantity = body["quantity"].get<int>();
        service_->allocate(id, quantity);

        auto updated = service_->getById(id);
        if (!updated) {
            sendErrorResponse(response, "Inventory not found after allocate",
                              Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        sendJsonResponse(response, updated->toJson().dump(),
                         Poco::Net::HTTPResponse::HTTP_OK);
    } catch (const json::exception& e) {
        sendErrorResponse(response, std::string("Invalid JSON body: ") + e.what(),
                          Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::invalid_argument& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::runtime_error& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void InventoryController::handleDeallocate(const std::string& id,
                                          Poco::Net::HTTPServerRequest& request,
                                          Poco::Net::HTTPServerResponse& response) {
    try {
        std::istream& bodyStream = request.stream();
        json body;
        bodyStream >> body;

        if (!body.contains("quantity") || !body["quantity"].is_number_integer()) {
            sendErrorResponse(response, "Missing or invalid 'quantity' field",
                              Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }

        int quantity = body["quantity"].get<int>();
        service_->deallocate(id, quantity);

        auto updated = service_->getById(id);
        if (!updated) {
            sendErrorResponse(response, "Inventory not found after deallocate",
                              Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        sendJsonResponse(response, updated->toJson().dump(),
                         Poco::Net::HTTPResponse::HTTP_OK);
    } catch (const json::exception& e) {
        sendErrorResponse(response, std::string("Invalid JSON body: ") + e.what(),
                          Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::invalid_argument& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::runtime_error& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void InventoryController::handleAdjust(const std::string& id,
                                      Poco::Net::HTTPServerRequest& request,
                                      Poco::Net::HTTPServerResponse& response) {
    try {
        std::istream& bodyStream = request.stream();
        json body;
        bodyStream >> body;

        if (!body.contains("quantityChange") || !body["quantityChange"].is_number_integer()) {
            sendErrorResponse(response, "Missing or invalid 'quantityChange' field",
                              Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }
        if (!body.contains("reason") || !body["reason"].is_string()) {
            sendErrorResponse(response, "Missing or invalid 'reason' field",
                              Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }

        int quantityChange = body["quantityChange"].get<int>();
        std::string reason = body["reason"].get<std::string>();

        service_->adjust(id, quantityChange, reason);

        auto updated = service_->getById(id);
        if (!updated) {
            sendErrorResponse(response, "Inventory not found after adjust",
                              Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        sendJsonResponse(response, updated->toJson().dump(),
                         Poco::Net::HTTPResponse::HTTP_OK);
    } catch (const json::exception& e) {
        sendErrorResponse(response, std::string("Invalid JSON body: ") + e.what(),
                          Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::invalid_argument& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::runtime_error& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
    } catch (const std::exception& e) {
        sendErrorResponse(response, e.what(), Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
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
