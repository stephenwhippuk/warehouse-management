#include "inventory/controllers/SwaggerController.hpp"
#include "inventory/utils/SwaggerGenerator.hpp"
#include "inventory/utils/Logger.hpp"

namespace inventory {
namespace controllers {

void SwaggerController::handleRequest(Poco::Net::HTTPServerRequest& request,
                                      Poco::Net::HTTPServerResponse& response) {
    utils::Logger::info("Swagger documentation requested");

    try {
        json spec = generateSpecification();
        sendJsonResponse(response, spec.dump(2), 200);
    } catch (const std::exception& e) {
        utils::Logger::error("Error generating Swagger documentation: {}", e.what());
        json error = {
            {"error", "Internal Server Error"},
            {"message", "Failed to generate API documentation"}
        };
        sendJsonResponse(response, error.dump(), 500);
    }
}

json SwaggerController::generateSpecification() {
    json spec = utils::SwaggerGenerator::generateSpec(
        "Inventory Service API",
        "1.0.0",
        "API for managing warehouse inventory, stock levels, and inventory operations"
    );

    // Add tags
    spec["tags"] = json::array({
        {{"name", "Inventory"}, {"description", "Inventory management operations"}},
        {{"name", "Health"}, {"description", "Service health checks"}}
    });

    // Add schemas and endpoints
    addSchemas(spec);
    addEndpoints(spec);

    return spec;
}

void SwaggerController::addSchemas(json& spec) {
    // Inventory schema
    utils::SwaggerGenerator::addSchema(spec, "Inventory", {
        {"type", "object"},
        {"required", json::array({"id", "productId", "warehouseId", "locationId", "quantity", "availableQuantity"})},
        {"properties", {
            {"id", {{"type", "string"}, {"format", "uuid"}, {"description", "Unique inventory record ID"}}},
            {"productId", {{"type", "string"}, {"format", "uuid"}, {"description", "Product ID"}}},
            {"warehouseId", {{"type", "string"}, {"format", "uuid"}, {"description", "Warehouse ID"}}},
            {"locationId", {{"type", "string"}, {"format", "uuid"}, {"description", "Storage location ID"}}},
            {"quantity", {{"type", "integer"}, {"minimum", 0}, {"description", "Total quantity"}}},
            {"availableQuantity", {{"type", "integer"}, {"minimum", 0}, {"description", "Available quantity (not reserved or allocated)"}}},
            {"reservedQuantity", {{"type", "integer"}, {"minimum", 0}, {"description", "Reserved quantity"}}},
            {"allocatedQuantity", {{"type", "integer"}, {"minimum", 0}, {"description", "Allocated quantity"}}},
            {"batchNumber", {{"type", "string"}, {"description", "Batch or lot number"}}},
            {"serialNumber", {{"type", "string"}, {"description", "Serial number for serialized items"}}},
            {"expiryDate", {{"type", "string"}, {"format", "date-time"}, {"description", "Expiration date"}}},
            {"receivedDate", {{"type", "string"}, {"format", "date-time"}, {"description", "Date received"}}},
            {"lastCountDate", {{"type", "string"}, {"format", "date-time"}, {"description", "Last physical count date"}}},
            {"status", {{"type", "string"}, {"enum", json::array({"available", "reserved", "allocated", "damaged", "expired", "quarantine"})}, {"description", "Inventory status"}}},
            {"metadata", {{"type", "object"}, {"description", "Additional custom metadata"}}},
            {"createdAt", {{"type", "string"}, {"format", "date-time"}}},
            {"updatedAt", {{"type", "string"}, {"format", "date-time"}}}
        }},
        {"example", {
            {"id", "123e4567-e89b-12d3-a456-426614174000"},
            {"productId", "223e4567-e89b-12d3-a456-426614174001"},
            {"warehouseId", "323e4567-e89b-12d3-a456-426614174002"},
            {"locationId", "423e4567-e89b-12d3-a456-426614174003"},
            {"quantity", 100},
            {"availableQuantity", 70},
            {"reservedQuantity", 20},
            {"allocatedQuantity", 10},
            {"batchNumber", "BATCH-2026-001"},
            {"status", "available"},
            {"receivedDate", "2026-02-01T10:00:00Z"},
            {"createdAt", "2026-02-01T10:00:00Z"},
            {"updatedAt", "2026-02-07T15:30:00Z"}
        }}
    });

    // Inventory adjustment request schema
    utils::SwaggerGenerator::addSchema(spec, "InventoryAdjustment", {
        {"type", "object"},
        {"required", json::array({"quantity", "reason"})},
        {"properties", {
            {"quantity", {{"type", "integer"}, {"description", "Adjustment amount (positive or negative)"}}},
            {"reason", {{"type", "string"}, {"description", "Reason for adjustment"}}},
            {"notes", {{"type", "string"}, {"description", "Additional notes"}}}
        }}
    });

    // Reserve request schema
    utils::SwaggerGenerator::addSchema(spec, "ReserveRequest", {
        {"type", "object"},
        {"required", json::array({"quantity"})},
        {"properties", {
            {"quantity", {{"type", "integer"}, {"minimum", 1}, {"description", "Quantity to reserve"}}}
        }}
    });

    // Error response schema
    utils::SwaggerGenerator::addSchema(spec, "Error", {
        {"type", "object"},
        {"properties", {
            {"error", {{"type", "string"}}},
            {"message", {{"type", "string"}}}
        }}
    });
}

void SwaggerController::addEndpoints(json& spec) {
    // GET /api/v1/inventory
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory",
        "get",
        "List all inventory records",
        "Retrieve a list of all inventory records across all warehouses",
        json::array({
            utils::SwaggerGenerator::createQueryParameter("warehouseId", "Filter by warehouse ID"),
            utils::SwaggerGenerator::createQueryParameter("productId", "Filter by product ID"),
            utils::SwaggerGenerator::createQueryParameter("status", "Filter by status")
        }),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Inventory")},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // GET /api/v1/inventory/{id}
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/{id}",
        "get",
        "Get inventory by ID",
        "Retrieve a specific inventory record by its unique identifier",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Inventory ID")
        }),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Inventory")},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // POST /api/v1/inventory
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory",
        "post",
        "Create new inventory record",
        "Create a new inventory record for a product in a specific location",
        json(nullptr),
        utils::SwaggerGenerator::createRequestBody(
            "#/components/schemas/Inventory",
            "Inventory data"
        ),
        {
            {"201", utils::SwaggerGenerator::createResponse("Created", "#/components/schemas/Inventory")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // PUT /api/v1/inventory/{id}
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/{id}",
        "put",
        "Update inventory record",
        "Update an existing inventory record",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Inventory ID")
        }),
        utils::SwaggerGenerator::createRequestBody(
            "#/components/schemas/Inventory",
            "Updated inventory data"
        ),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Inventory")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // DELETE /api/v1/inventory/{id}
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/{id}",
        "delete",
        "Delete inventory record",
        "Delete an inventory record by ID",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Inventory ID")
        }),
        json(nullptr),
        {
            {"204", utils::SwaggerGenerator::createResponse("Successfully deleted")},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // POST /api/v1/inventory/{id}/reserve
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/{id}/reserve",
        "post",
        "Reserve inventory",
        "Reserve a quantity of inventory for an order",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Inventory ID")
        }),
        utils::SwaggerGenerator::createRequestBody(
            "#/components/schemas/ReserveRequest",
            "Reserve quantity"
        ),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Inventory")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // POST /api/v1/inventory/{id}/release
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/{id}/release",
        "post",
        "Release reserved inventory",
        "Release a reserved quantity back to available",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Inventory ID")
        }),
        utils::SwaggerGenerator::createRequestBody(
            "#/components/schemas/ReserveRequest",
            "Release quantity"
        ),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Inventory")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // POST /api/v1/inventory/{id}/adjust
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/{id}/adjust",
        "post",
        "Adjust inventory quantity",
        "Manually adjust inventory quantity (for cycle counts, corrections, etc.)",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Inventory ID")
        }),
        utils::SwaggerGenerator::createRequestBody(
            "#/components/schemas/InventoryAdjustment",
            "Adjustment details"
        ),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Inventory")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // GET /health
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/health",
        "get",
        "Health check",
        "Check if the service is running and healthy",
        json(nullptr),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Service is healthy")},
            {"503", utils::SwaggerGenerator::createResponse("Service unavailable")}
        },
        {"Health"}
    );
}

void SwaggerController::sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                                        const std::string& jsonContent,
                                        int statusCode) {
    response.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(statusCode));
    response.setContentType("application/json");
    response.setChunkedTransferEncoding(true);

    std::ostream& out = response.send();
    out << jsonContent;
}

} // namespace controllers
} // namespace inventory
