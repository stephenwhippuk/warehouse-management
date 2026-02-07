#include <catch2/catch_test_macros.hpp>
#include "warehouse/models/Warehouse.hpp"

using namespace warehouse::models;

TEST_CASE("Warehouse model tests", "[warehouse]") {
    SECTION("Warehouse can be created") {
        Warehouse warehouse;
        warehouse.setCode("WH-001");
        warehouse.setName("Test Warehouse");
        
        REQUIRE(warehouse.getCode() == "WH-001");
        REQUIRE(warehouse.getName() == "Test Warehouse");
    }
    
    SECTION("Warehouse type conversion") {
        REQUIRE(warehouseTypeToString(WarehouseType::Distribution) == "distribution");
        REQUIRE(stringToWarehouseType("fulfillment") == WarehouseType::Fulfillment);
    }
    
    SECTION("Warehouse JSON serialization") {
        Warehouse warehouse;
        warehouse.setId("123e4567-e89b-12d3-a456-426614174000");
        warehouse.setCode("WH-001");
        warehouse.setName("Test Warehouse");
        
        Address addr;
        addr.street = "123 Test St";
        addr.city = "Chicago";
        addr.postalCode = "60601";
        addr.country = "US";
        warehouse.setAddress(addr);
        
        warehouse.setType(WarehouseType::Distribution);
        warehouse.setStatus(Status::Active);
        
        AuditInfo audit;
        audit.createdAt = std::chrono::system_clock::now();
        audit.createdBy = "test-user";
        warehouse.setAudit(audit);
        
        // Test JSON conversion
        auto json = warehouse.toJson();
        REQUIRE(json["code"] == "WH-001");
        REQUIRE(json["name"] == "Test Warehouse");
        REQUIRE(json["type"] == "distribution");
        REQUIRE(json["status"] == "active");
        
        // Test deserialization
        auto deserialized = Warehouse::fromJson(json);
        REQUIRE(deserialized.getCode() == warehouse.getCode());
        REQUIRE(deserialized.getName() == warehouse.getName());
    }
}
