#include <catch2/catch_test_macros.hpp>
#include "warehouse/models/Location.hpp"

using namespace warehouse::models;

TEST_CASE("Location model tests", "[location]") {
    SECTION("Location can be created") {
        Location location;
        location.setCode("A-01-02-03");
        location.setWarehouseId("warehouse-uuid");
        location.setType(LocationType::Bin);
        
        REQUIRE(location.getCode() == "A-01-02-03");
        REQUIRE(location.getWarehouseId() == "warehouse-uuid");
        REQUIRE(location.getType() == LocationType::Bin);
    }
    
    SECTION("Location type conversion") {
        REQUIRE(locationTypeToString(LocationType::Bin) == "bin");
        REQUIRE(stringToLocationType("shelf") == LocationType::Shelf);
    }
    
    SECTION("Location status conversion") {
        REQUIRE(locationStatusToString(LocationStatus::Active) == "active");
        REQUIRE(stringToLocationStatus("full") == LocationStatus::Full);
    }
    
    SECTION("Location JSON serialization") {
        Location location;
        location.setId("123e4567-e89b-12d3-a456-426614174001");
        location.setWarehouseId("warehouse-uuid");
        location.setCode("A-01-02-03");
        location.setType(LocationType::Bin);
        location.setZone("A");
        location.setAisle("01");
        location.setRack("02");
        location.setShelf("03");
        location.setStatus(LocationStatus::Active);
        
        AuditInfo audit;
        audit.createdAt = std::chrono::system_clock::now();
        audit.createdBy = "test-user";
        location.setAudit(audit);
        
        // Test JSON conversion
        auto json = location.toJson();
        REQUIRE(json["code"] == "A-01-02-03");
        REQUIRE(json["type"] == "bin");
        REQUIRE(json["zone"] == "A");
        REQUIRE(json["status"] == "active");
        
        // Test deserialization
        auto deserialized = Location::fromJson(json);
        REQUIRE(deserialized.getCode() == location.getCode());
        REQUIRE(deserialized.getType() == location.getType());
    }
    
    SECTION("Location with dimensions") {
        Location location;
        
        Dimensions dims;
        dims.length = 100;
        dims.width = 50;
        dims.height = 200;
        dims.unit = "cm";
        location.setDimensions(dims);
        
        REQUIRE(location.getDimensions().has_value());
        REQUIRE(location.getDimensions()->length == 100);
        REQUIRE(location.getDimensions()->unit == "cm");
    }
}
