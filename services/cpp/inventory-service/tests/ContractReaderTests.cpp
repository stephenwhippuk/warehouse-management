#include <catch2/catch_all.hpp>
#include "inventory/utils/ContractReader.hpp"
#include "inventory/utils/SwaggerGenerator.hpp"
#include <filesystem>
#include <fstream>

using namespace inventory::utils;

TEST_CASE("ContractReader loads DTOs", "[contracts][dtos]") {
    std::string contractsPath = "contracts";
    
    if (!std::filesystem::exists(contractsPath)) {
        WARN("Contracts directory not found, skipping test");
        return;
    }

    ContractReader reader(contractsPath);
    auto dtos = reader.loadDtos();

    REQUIRE(!dtos.empty());
    REQUIRE(dtos.count("InventoryItemDto") > 0);
    REQUIRE(dtos.count("ErrorDto") > 0);

    auto& inventoryDto = dtos["InventoryItemDto"];
    REQUIRE(inventoryDto.name == "InventoryItemDto");
    REQUIRE(inventoryDto.version == "1.0");
    REQUIRE(!inventoryDto.fields.empty());
}

TEST_CASE("ContractReader loads Requests", "[contracts][requests]") {
    std::string contractsPath = "contracts";
    
    if (!std::filesystem::exists(contractsPath)) {
        WARN("Contracts directory not found, skipping test");
        return;
    }

    ContractReader reader(contractsPath);
    auto requests = reader.loadRequests();

    REQUIRE(!requests.empty());
    REQUIRE(requests.count("ReserveInventoryRequest") > 0);

    auto& reserveReq = requests["ReserveInventoryRequest"];
    REQUIRE(reserveReq.name == "ReserveInventoryRequest");
    REQUIRE(reserveReq.version == "1.0");
    REQUIRE(!reserveReq.parameters.empty());
}

TEST_CASE("ContractReader loads Endpoints", "[contracts][endpoints]") {
    std::string contractsPath = "contracts";
    
    if (!std::filesystem::exists(contractsPath)) {
        WARN("Contracts directory not found, skipping test");
        return;
    }

    ContractReader reader(contractsPath);
    auto endpoints = reader.loadEndpoints();

    REQUIRE(!endpoints.empty());
    
    // Check we have expected endpoints
    bool hasGetById = false;
    bool hasListInventory = false;
    bool hasReserve = false;

    for (const auto& endpoint : endpoints) {
        if (endpoint.name == "GetInventoryById") hasGetById = true;
        if (endpoint.name == "ListInventory") hasListInventory = true;
        if (endpoint.name == "ReserveInventory") hasReserve = true;
    }

    REQUIRE(hasGetById);
    REQUIRE(hasListInventory);
    REQUIRE(hasReserve);
}

TEST_CASE("ContractReader converts types to JSON Schema", "[contracts][types]") {
    auto uuidSchema = ContractReader::contractTypeToJsonSchema("UUID");
    REQUIRE(uuidSchema["type"] == "string");
    REQUIRE(uuidSchema["format"] == "uuid");

    auto intSchema = ContractReader::contractTypeToJsonSchema("PositiveInteger");
    REQUIRE(intSchema["type"] == "integer");
    REQUIRE(intSchema["minimum"] == 1);

    auto dateTimeSchema = ContractReader::contractTypeToJsonSchema("DateTime");
    REQUIRE(dateTimeSchema["type"] == "string");
    REQUIRE(dateTimeSchema["format"] == "date-time");
}

TEST_CASE("SwaggerGenerator creates spec from contracts", "[swagger][contracts]") {
    std::string contractsPath = "contracts";
    
    if (!std::filesystem::exists(contractsPath)) {
        WARN("Contracts directory not found, skipping test");
        return;
    }

    json spec = SwaggerGenerator::generateSpecFromContracts(
        "Test API",
        "1.0.0",
        "Test Description",
        contractsPath
    );

    // Verify OpenAPI structure
    REQUIRE(spec["openapi"] == "3.0.0");
    REQUIRE(spec["info"]["title"] == "Test API");
    REQUIRE(spec["info"]["version"] == "1.0.0");
    REQUIRE(spec.contains("paths"));
    REQUIRE(spec.contains("components"));
    REQUIRE(spec["components"].contains("schemas"));

    // Verify schemas exist for DTOs
    REQUIRE(spec["components"]["schemas"].contains("InventoryItemDto"));
    REQUIRE(spec["components"]["schemas"].contains("ErrorDto"));
    REQUIRE(spec["components"]["schemas"].contains("ReserveInventoryRequest"));

    // Verify paths exist
    REQUIRE(spec["paths"].contains("/api/v1/inventory/{id}"));
    REQUIRE(spec["paths"].contains("/api/v1/inventory"));
    REQUIRE(spec["paths"].contains("/api/v1/inventory/{id}/reserve"));

    // Verify GET /api/v1/inventory/{id} endpoint
    auto getByIdPath = spec["paths"]["/api/v1/inventory/{id}"];
    REQUIRE(getByIdPath.contains("get"));
    REQUIRE(getByIdPath["get"]["operationId"] == "GetInventoryById");
    REQUIRE(getByIdPath["get"].contains("parameters"));
    REQUIRE(getByIdPath["get"].contains("responses"));
    REQUIRE(getByIdPath["get"]["responses"].contains("200"));
    REQUIRE(getByIdPath["get"]["responses"].contains("404"));

    // Verify POST /api/v1/inventory/{id}/reserve endpoint
    auto reservePath = spec["paths"]["/api/v1/inventory/{id}/reserve"];
    REQUIRE(reservePath.contains("post"));
    REQUIRE(reservePath["post"]["operationId"] == "ReserveInventory");
    REQUIRE(reservePath["post"].contains("requestBody"));
    REQUIRE(reservePath["post"].contains("responses"));
}

TEST_CASE("SwaggerGenerator handles DTO with entity-prefixed fields", "[swagger][dto]") {
    std::string contractsPath = "contracts";
    
    if (!std::filesystem::exists(contractsPath)) {
        WARN("Contracts directory not found, skipping test");
        return;
    }

    ContractReader reader(contractsPath);
    auto dtos = reader.loadDtos();
    REQUIRE(dtos.count("InventoryItemDto") > 0);

    auto& inventoryDto = dtos["InventoryItemDto"];
    auto schema = ContractReader::dtoToSchema(inventoryDto);

    // Should have entity-prefixed fields for referenced entities
    REQUIRE(schema["properties"].contains("ProductId"));
    REQUIRE(schema["properties"].contains("ProductSku"));
    REQUIRE(schema["properties"].contains("WarehouseId"));
    REQUIRE(schema["properties"].contains("LocationId"));
    
    // Should have non-prefixed fields for direct inventory fields
    REQUIRE(schema["properties"].contains("id"));
    REQUIRE(schema["properties"].contains("quantity"));
    REQUIRE(schema["properties"].contains("availableQuantity"));
}
