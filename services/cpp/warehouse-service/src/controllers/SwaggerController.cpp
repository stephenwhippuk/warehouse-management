#include "warehouse/controllers/SwaggerController.hpp"
#include "warehouse/utils/SwaggerGenerator.hpp"
#include "warehouse/utils/Logger.hpp"

namespace warehouse {
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
        "Warehouse Service API",
        "1.0.0",
        "API for managing warehouse facilities and storage locations"
    );

    spec["tags"] = json::array({
        {{"name", "Warehouses"}, {"description", "Warehouse facility management"}},
        {{"name", "Locations"}, {"description", "Storage location management"}},
        {{"name", "Health"}, {"description", "Service health checks"}}
    });

    addSchemas(spec);
    addEndpoints(spec);

    return spec;
}

void SwaggerController::addSchemas(json& spec) {
    // Warehouse schema
    utils::SwaggerGenerator::addSchema(spec, "Warehouse", {
        {"type", "object"},
        {"required", json::array({"id", "code", "name", "address"})},
        {"properties", {
            {"id", {{"type", "string"}, {"format", "uuid"}, {"description", "Unique warehouse ID"}}},
            {"code", {{"type", "string"}, {"minLength", 1}, {"maxLength", 50}, {"description", "Warehouse code"}}},
            {"name", {{"type", "string"}, {"minLength", 1}, {"maxLength", 200}, {"description", "Warehouse name"}}},
            {"description", {{"type", "string"}, {"description", "Warehouse description"}}},
            {"address", {{"type", "string"}, {"description", "Physical address"}}},
            {"city", {{"type", "string"}, {"description", "City"}}},
            {"state", {{"type", "string"}, {"description", "State/Province"}}},
            {"postalCode", {{"type", "string"}, {"description", "Postal/ZIP code"}}},
            {"country", {{"type", "string"}, {"description", "Country"}}},
            {"latitude", {{"type", "number"}, {"format", "double"}, {"description", "GPS latitude"}}},
            {"longitude", {{"type", "number"}, {"format", "double"}, {"description", "GPS longitude"}}},
            {"timezone", {{"type", "string"}, {"description", "IANA timezone"}}},
            {"status", {{"type", "string"}, {"enum", json::array({"active", "inactive", "maintenance"})}}},
            {"type", {{"type", "string"}, {"description", "Warehouse type (distribution, fulfillment, etc.)"}}},
            {"capacity", {{"type", "integer"}, {"description", "Total capacity in cubic meters"}}},
            {"metadata", {{"type", "object"}, {"description", "Additional custom metadata"}}},
            {"createdAt", {{"type", "string"}, {"format", "date-time"}}},
            {"updatedAt", {{"type", "string"}, {"format", "date-time"}}}
        }},
        {"example", {
            {"id", "123e4567-e89b-12d3-a456-426614174000"},
            {"code", "WH-001"},
            {"name", "Main Distribution Center"},
            {"address", "123 Warehouse Ave"},
            {"city", "Seattle"},
            {"state", "WA"},
            {"postalCode", "98101"},
            {"country", "USA"},
            {"status", "active"},
            {"type", "distribution"},
            {"capacity", 50000}
        }}
    });

    // Location schema
    utils::SwaggerGenerator::addSchema(spec, "Location", {
        {"type", "object"},
        {"required", json::array({"id", "warehouseId", "code", "aisle", "bay", "level"})},
        {"properties", {
            {"id", {{"type", "string"}, {"format", "uuid"}}},
            {"warehouseId", {{"type", "string"}, {"format", "uuid"}}},
            {"code", {{"type", "string"}, {"description", "Location code (e.g., A-01-01)"}}},
            {"aisle", {{"type", "string"}, {"description", "Aisle identifier"}}},
            {"bay", {{"type", "string"}, {"description", "Bay number"}}},
            {"level", {{"type", "string"}, {"description", "Level/shelf number"}}},
            {"zone", {{"type", "string"}, {"description", "Zone (e.g., picking, bulk, cold)"}}},
            {"type", {{"type", "string"}, {"description", "Location type (shelf, pallet, floor, etc.)"}}},
            {"status", {{"type", "string"}, {"enum", json::array({"available", "occupied", "reserved", "damaged"})}}},
            {"capacity", {{"type", "integer"}, {"description", "Capacity in cubic centimeters"}}},
            {"currentWeight", {{"type", "integer"}, {"description", "Current weight in grams"}}},
            {"maxWeight", {{"type", "integer"}, {"description", "Maximum weight in grams"}}},
            {"metadata", {{"type", "object"}}},
            {"createdAt", {{"type", "string"}, {"format", "date-time"}}},
            {"updatedAt", {{"type", "string"}, {"format", "date-time"}}}
        }},
        {"example", {
            {"id", "223e4567-e89b-12d3-a456-426614174001"},
            {"warehouseId", "123e4567-e89b-12d3-a456-426614174000"},
            {"code", "A-01-01"},
            {"aisle", "A"},
            {"bay", "01"},
            {"level", "01"},
            {"zone", "picking"},
            {"type", "shelf"},
            {"status", "available"}
        }}
    });

    // Error schema
    utils::SwaggerGenerator::addSchema(spec, "Error", {
        {"type", "object"},
        {"properties", {
            {"error", {{"type", "string"}}},
            {"message", {{"type", "string"}}}
        }}
    });
}

void SwaggerController::addEndpoints(json& spec) {
    // Warehouse endpoints
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/warehouses",
        "get",
        "List all warehouses",
        "Retrieve a list of all warehouse facilities",
        json::array({
            utils::SwaggerGenerator::createQueryParameter("status", "Filter by status"),
            utils::SwaggerGenerator::createQueryParameter("type", "Filter by warehouse type")
        }),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Warehouse")},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Warehouses"}
    );

    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/warehouses/{id}",
        "get",
        "Get warehouse by ID",
        "Retrieve a specific warehouse by its unique identifier",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Warehouse ID")
        }),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Warehouse")},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Warehouses"}
    );

    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/warehouses",
        "post",
        "Create new warehouse",
        "Create a new warehouse facility",
        json(nullptr),
        utils::SwaggerGenerator::createRequestBody("#/components/schemas/Warehouse", "Warehouse data"),
        {
            {"201", utils::SwaggerGenerator::createResponse("Created", "#/components/schemas/Warehouse")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Warehouses"}
    );

    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/warehouses/{id}",
        "put",
        "Update warehouse",
        "Update an existing warehouse facility",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Warehouse ID")
        }),
        utils::SwaggerGenerator::createRequestBody("#/components/schemas/Warehouse", "Updated warehouse data"),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Warehouse")},
            {"400", {{"$ref", "#/components/responses/BadRequest"}}},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Warehouses"}
    );

    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/warehouses/{id}",
        "delete",
        "Delete warehouse",
        "Delete a warehouse facility",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Warehouse ID")
        }),
        json(nullptr),
        {
            {"204", utils::SwaggerGenerator::createResponse("Successfully deleted")},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Warehouses"}
    );

    // Location endpoints
    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/locations",
        "get",
        "List all locations",
        "Retrieve storage locations across all warehouses",
        json::array({
            utils::SwaggerGenerator::createQueryParameter("warehouseId", "Filter by warehouse"),
            utils::SwaggerGenerator::createQueryParameter("zone", "Filter by zone"),
            utils::SwaggerGenerator::createQueryParameter("status", "Filter by status")
        }),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Location")},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Locations"}
    );

    utils::SwaggerGenerator::addEndpoint(
        spec,
        "/api/v1/locations/{id}",
        "get",
        "Get location by ID",
        "Retrieve a specific storage location",
        json::array({
            utils::SwaggerGenerator::createPathParameter("id", "Location ID")
        }),
        json(nullptr),
        {
            {"200", utils::SwaggerGenerator::createResponse("Success", "#/components/schemas/Location")},
            {"404", {{"$ref", "#/components/responses/NotFound"}}},
            {"500", {{"$ref", "#/components/responses/InternalError"}}}
        },
        {"Locations"}
    );

    // Health check
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
} // namespace warehouse
