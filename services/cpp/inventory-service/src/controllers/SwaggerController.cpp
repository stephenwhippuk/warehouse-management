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
            {"expirationDate", {{"type", "string"}, {"format", "date"}, {"description", "Expiration date"}}},
            {"manufactureDate", {{"type", "string"}, {"format", "date"}, {"description", "Manufacture date"}}},
            {"receivedDate", {{"type", "string"}, {"format", "date-time"}, {"description", "Date received"}}},
            {"lastCountedDate", {{"type", "string"}, {"format", "date-time"}, {"description", "Last physical count date"}}},
            {"lastCountedBy", {{"type", "string"}, {"description", "User who performed last count"}}},
            {"status", {{"type", "string"}, {"enum", json::array({"available", "reserved", "allocated", "damaged", "expired", "quarantine", "recalled"})}, {"description", "Inventory status"}}},
            {"qualityStatus", {{"type", "string"}, {"enum", json::array({"passed", "failed", "pending", "not_tested"})}, {"description", "Quality control status"}}},
            {"notes", {{"type", "string"}, {"description", "Additional notes"}}},
            {"metadata", {{"type", "object"}, {"description", "Additional custom metadata"}}},
            {"audit", {{"type", "object"}, {"description", "Audit information"}, {"properties", {
                {"createdAt", {{"type", "string"}, {"format", "date-time"}}},
                {"updatedAt", {{"type", "string"}, {"format", "date-time"}}},
                {"createdBy", {{"type", "string"}}},
                {"updatedBy", {{"type", "string"}}}
            }}}}
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
            {"audit", {
                {"createdAt", "2026-02-01T10:00:00Z"},
                {"updatedAt", "2026-02-07T15:30:00Z"},
                {"createdBy", "system"},
                {"updatedBy", "system"}
            }}
        }}
    });

    // List of inventory records
    utils::SwaggerGenerator::addSchema(spec, "InventoryList", {
        {"type", "array"},
        {"items", {{"$ref", "#/components/schemas/Inventory"}}}
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
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/InventoryList")},
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

    // POST /api/v1/inventory/{id}/allocate
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/{id}/allocate",
        "post",
        "Allocate inventory",
        "Allocate reserved inventory to a pick or shipment",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Inventory ID")
        }),
        utils::SwaggerGenerator::createRequestBody(
            "#/components/schemas/ReserveRequest",
            "Allocate quantity"
        ),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Inventory")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // POST /api/v1/inventory/{id}/deallocate
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/{id}/deallocate",
        "post",
        "Deallocate inventory",
        "Move allocated quantity back to available",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Inventory ID")
        }),
        utils::SwaggerGenerator::createRequestBody(
            "#/components/schemas/ReserveRequest",
            "Deallocate quantity"
        ),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Inventory")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // GET /api/v1/inventory/low-stock
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/low-stock",
        "get",
        "List low stock inventory",
        "Retrieve inventory records with available quantity below a threshold",
        json::array({
            utils::SwaggerGenerator::createQueryParameter("threshold", "Low stock threshold", "integer", false)
        }),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/InventoryList")},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // GET /api/v1/inventory/expired
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/expired",
        "get",
        "List expired inventory",
        "Retrieve inventory records that are past their expiration date",
        json(nullptr),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/InventoryList")},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // GET /api/v1/inventory/product/{productId}
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/product/{productId}",
        "get",
        "List inventory by product",
        "Retrieve inventory records for a specific product across all locations",
        json::array({
            utils::SwaggerGenerator::createPathParameter("productId", "Product ID")
        }),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/InventoryList")},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // GET /api/v1/inventory/warehouse/{warehouseId}
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/warehouse/{warehouseId}",
        "get",
        "List inventory by warehouse",
        "Retrieve inventory records for a specific warehouse",
        json::array({
            utils::SwaggerGenerator::createPathParameter("warehouseId", "Warehouse ID")
        }),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/InventoryList")},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Inventory"}
    );

    // GET /api/v1/inventory/location/{locationId}
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/inventory/location/{locationId}",
        "get",
        "List inventory by location",
        "Retrieve inventory records for a specific storage location",
        json::array({
            utils::SwaggerGenerator::createPathParameter("locationId", "Location ID")
        }),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/InventoryList")},
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
