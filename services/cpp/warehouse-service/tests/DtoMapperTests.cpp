#include <catch2/catch_all.hpp>
#include "warehouse/utils/DtoMapper.hpp"
#include "warehouse/models/Warehouse.hpp"
#include "warehouse/models/Location.hpp"
#include "warehouse/dtos/WarehouseDto.hpp"
#include "warehouse/dtos/LocationDto.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace warehouse;

// Helper to create valid ISO 8601 timestamp
std::string createIso8601Timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// Helper to create a valid warehouse model
models::Warehouse createValidWarehouse() {
    models::Warehouse wh;
    wh.setId("550e8400-e29b-41d4-a716-446655440000");
    wh.setCode("WH-MAIN");
    wh.setName("Main Warehouse");
    wh.setStatus(models::Status::Active);
    wh.setType(models::WarehouseType::Distribution);
    
    // Set address
    models::Address addr;
    addr.street = "123 Main St";
    addr.city = "Portland";
    addr.state = "OR";
    addr.postalCode = "97201";
    addr.country = "US";
    wh.setAddress(addr);
    
    return wh;
}

// Helper to create a valid location model
models::Location createValidLocation() {
    models::Location loc;
    loc.setId("650e8400-e29b-41d4-a716-446655440001");
    loc.setWarehouseId("750e8400-e29b-41d4-a716-446655440002");
    loc.setCode("A1-B2-C3");
    loc.setType(models::LocationType::Picking);
    loc.setStatus(models::LocationStatus::Active);
    loc.setIsPickable(true);
    loc.setIsReceivable(true);
    
    return loc;
}

TEST_CASE("DtoMapper converts valid warehouse to DTO", "[dto][mapper][warehouse]") {
    auto warehouse = createValidWarehouse();
    
    SECTION("Successful warehouse conversion") {
        auto dto = utils::DtoMapper::toWarehouseDto(warehouse);
        
        REQUIRE(dto.getId() == warehouse.getId());
        REQUIRE(dto.getCode() == warehouse.getCode());
        REQUIRE(dto.getName() == warehouse.getName());
        REQUIRE(dto.getStatus() == "active");
        REQUIRE(dto.getType() == "distribution");
        
        // Check address is JSON
        auto address = dto.getAddress();
        REQUIRE(address.is_object());
        REQUIRE(address["street"] == "123 Main St");
        REQUIRE(address["city"] == "Portland");
        REQUIRE(address["postalCode"] == "97201");
    }
    
    SECTION("Warehouse with optional fields") {
        warehouse.setDescription("Primary distribution center");
        warehouse.setTotalArea(50000.0);
        warehouse.setStorageCapacity(100000.0);
        
        auto dto = utils::DtoMapper::toWarehouseDto(warehouse);
        
        REQUIRE(dto.getDescription().has_value());
        REQUIRE(dto.getDescription().value() == "Primary distribution center");
        REQUIRE(dto.getTotalArea().has_value());
        REQUIRE(dto.getTotalArea().value() == 50000.0);
    }
}

TEST_CASE("DtoMapper handles different warehouse statuses", "[dto][mapper][warehouse][status]") {
    SECTION("Active status") {
        auto wh = createValidWarehouse();
        wh.setStatus(models::Status::Active);
        
        auto dto = utils::DtoMapper::toWarehouseDto(wh);
        REQUIRE(dto.getStatus() == "active");
    }
    
    SECTION("Inactive status") {
        auto wh = createValidWarehouse();
        wh.setStatus(models::Status::Inactive);
        
        auto dto = utils::DtoMapper::toWarehouseDto(wh);
        REQUIRE(dto.getStatus() == "inactive");
    }
    
    SECTION("Archived status") {
        auto wh = createValidWarehouse();
        wh.setStatus(models::Status::Archived);
        
        auto dto = utils::DtoMapper::toWarehouseDto(wh);
        REQUIRE(dto.getStatus() == "archived");
    }
}

TEST_CASE("DtoMapper handles different warehouse types", "[dto][mapper][warehouse][type]") {
    auto wh = createValidWarehouse();
    
    SECTION("Distribution") {
        wh.setType(models::WarehouseType::Distribution);
        auto dto = utils::DtoMapper::toWarehouseDto(wh);
        REQUIRE(dto.getType() == "distribution");
    }
    
    SECTION("Fulfillment") {
        wh.setType(models::WarehouseType::Fulfillment);
        auto dto = utils::DtoMapper::toWarehouseDto(wh);
        REQUIRE(dto.getType() == "fulfillment");
    }
    
    SECTION("Cold Storage") {
        wh.setType(models::WarehouseType::ColdStorage);
        auto dto = utils::DtoMapper::toWarehouseDto(wh);
        REQUIRE(dto.getType() == "cold_storage");
    }
}

TEST_CASE("DtoMapper converts valid location to DTO", "[dto][mapper][location]") {
    auto location = createValidLocation();
    
    SECTION("Successful location conversion") {
        auto dto = utils::DtoMapper::toLocationDto(
            location,
            "WH-MAIN",  // warehouseCode
            "Main Warehouse"  // warehouseName (optional)
        );
        
        REQUIRE(dto.getId() == location.getId());
        REQUIRE(dto.getWarehouseId() == location.getWarehouseId());
        REQUIRE(dto.getWarehouseCode() == "WH-MAIN");
        REQUIRE(dto.getCode() == location.getCode());
        REQUIRE(dto.getType() == "picking");
        REQUIRE(dto.getStatus() == "active");
        REQUIRE(dto.getIsPickable() == true);
        REQUIRE(dto.getIsReceivable() == true);
        
        // Optional warehouse name
        REQUIRE(dto.getWarehouseName().has_value());
        REQUIRE(dto.getWarehouseName().value() == "Main Warehouse");
    }
    
    SECTION("Location with optional fields") {
        location.setAisle("A");
        location.setBay("1");
        location.setLevel("2");
        location.setZone("PICKING");
        location.setName("Pick Location A1-2");
        
        auto dto = utils::DtoMapper::toLocationDto(location, "WH-MAIN");
        
        REQUIRE(dto.getAisle().has_value());
        REQUIRE(dto.getAisle().value() == "A");
        REQUIRE(dto.getBay().has_value());
        REQUIRE(dto.getBay().value() == "1");
        REQUIRE(dto.getLevel().has_value());
        REQUIRE(dto.getLevel().value() == "2");
        REQUIRE(dto.getZone().has_value());
        REQUIRE(dto.getZone().value() == "PICKING");
    }
}

TEST_CASE("DtoMapper handles different location types", "[dto][mapper][location][type]") {
    auto loc = createValidLocation();
    
    SECTION("Picking") {
        loc.setType(models::LocationType::Picking);
        auto dto = utils::DtoMapper::toLocationDto(loc, "WH-1");
        REQUIRE(dto.getType() == "picking");
    }
    
    SECTION("Staging") {
        loc.setType(models::LocationType::Staging);
        auto dto = utils::DtoMapper::toLocationDto(loc, "WH-1");
        REQUIRE(dto.getType() == "staging");
    }
    
    SECTION("Receiving") {
        loc.setType(models::LocationType::Receiving);
        auto dto = utils::DtoMapper::toLocationDto(loc, "WH-1");
        REQUIRE(dto.getType() == "receiving");
    }
    
    SECTION("Shipping") {
        loc.setType(models::LocationType::Shipping);
        auto dto = utils::DtoMapper::toLocationDto(loc, "WH-1");
        REQUIRE(dto.getType() == "shipping");
    }
}

TEST_CASE("DtoMapper handles different location statuses", "[dto][mapper][location][status]") {
    auto loc = createValidLocation();
    
    SECTION("Active") {
        loc.setStatus(models::LocationStatus::Active);
        auto dto = utils::DtoMapper::toLocationDto(loc, "WH-1");
        REQUIRE(dto.getStatus() == "active");
    }
    
    SECTION("Reserved") {
        loc.setStatus(models::LocationStatus::Reserved);
        auto dto = utils::DtoMapper::toLocationDto(loc, "WH-1");
        REQUIRE(dto.getStatus() == "reserved");
    }
    
    SECTION("Full") {
        loc.setStatus(models::LocationStatus::Full);
        auto dto = utils::DtoMapper::toLocationDto(loc, "WH-1");
        REQUIRE(dto.getStatus() == "full");
    }
    
    SECTION("Damaged") {
        loc.setStatus(models::LocationStatus::Damaged);
        auto dto = utils::DtoMapper::toLocationDto(loc, "WH-1");
        REQUIRE(dto.getStatus() == "damaged");
    }
}

TEST_CASE("DtoMapper validates identity fields", "[dto][mapper][validation]") {
    auto loc = createValidLocation();
    
    SECTION("Empty warehouseCode throws") {
        REQUIRE_THROWS_WITH(
            utils::DtoMapper::toLocationDto(loc, "", std::nullopt),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
}

TEST_CASE("WarehouseDto validates on construction", "[dto][validation][warehouse]") {
    nlohmann::json validAddress = {
        {"street", "123 Main St"},
        {"city", "Portland"},
        {"state", "OR"},
        {"postalCode", "97201"},
        {"country", "US"}
    };
    
    SECTION("Valid warehouse DTO construction succeeds") {
        REQUIRE_NOTHROW(
            dtos::WarehouseDto(
                "550e8400-e29b-41d4-a716-446655440000",  // id
                "WH-MAIN",                                // code
                "Main Warehouse",                         // name
                "active",                                 // status
                validAddress,                             // address
                "distribution",                         // type
                createIso8601Timestamp(),                // createdAt
                createIso8601Timestamp()                 // updatedAt
            )
        );
    }
    
    SECTION("Invalid UUID throws") {
        REQUIRE_THROWS_WITH(
            dtos::WarehouseDto(
                "not-a-uuid",  // Invalid ID
                "WH-MAIN",
                "Main Warehouse",
                "active",
                validAddress,
                "distribution",
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("valid UUID")
        );
    }
    
    SECTION("Empty code throws") {
        REQUIRE_THROWS_WITH(
            dtos::WarehouseDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "",  // Empty code
                "Main Warehouse",
                "active",
                validAddress,
                "distribution",
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Invalid status throws") {
        REQUIRE_THROWS_WITH(
            dtos::WarehouseDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "WH-MAIN",
                "Main Warehouse",
                "invalid-status",  // Invalid
                validAddress,
                "distribution",
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("status")
        );
    }
    
    SECTION("Invalid timestamp throws") {
        REQUIRE_THROWS_WITH(
            dtos::WarehouseDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "WH-MAIN",
                "Main Warehouse",
                "active",
                validAddress,
                "distribution",
                "not-a-timestamp",  // Invalid
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("ISO 8601")
        );
    }
}

TEST_CASE("LocationDto validates on construction", "[dto][validation][location]") {
    nlohmann::json validAudit = {
        {"createdAt", createIso8601Timestamp()},
        {"createdBy", "system"}
    };
    
    SECTION("Valid location DTO construction succeeds") {
        REQUIRE_NOTHROW(
            dtos::LocationDto(
                "650e8400-e29b-41d4-a716-446655440001",  // id
                "750e8400-e29b-41d4-a716-446655440002",  // warehouseId
                "WH-MAIN",                                // warehouseCode
                "A1-B2-C3",                               // code
                "pick_face",                              // type
                "available",                              // status
                true,                                     // isPickable
                true,                                     // isReceivable
                validAudit                                // audit
            )
        );
    }
    
    SECTION("Invalid UUID throws") {
        REQUIRE_THROWS_WITH(
            dtos::LocationDto(
                "not-a-uuid",  // Invalid
                "750e8400-e29b-41d4-a716-446655440002",
                "WH-MAIN",
                "A1-B2-C3",
                "pick_face",
                "available",
                true, true,
                validAudit
            ),
            Catch::Matchers::ContainsSubstring("valid UUID")
        );
    }
    
    SECTION("Empty warehouseCode throws") {
        REQUIRE_THROWS_WITH(
            dtos::LocationDto(
                "650e8400-e29b-41d4-a716-446655440001",
                "750e8400-e29b-41d4-a716-446655440002",
                "",  // Empty
                "A1-B2-C3",
                "pick_face",
                "available",
                true, true,
                validAudit
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Empty code throws") {
        REQUIRE_THROWS_WITH(
            dtos::LocationDto(
                "650e8400-e29b-41d4-a716-446655440001",
                "750e8400-e29b-41d4-a716-446655440002",
                "WH-MAIN",
                "",  // Empty
                "picking",
                "active",
                true, true,
                validAudit
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Invalid type throws") {
        REQUIRE_THROWS_WITH(
            dtos::LocationDto(
                "650e8400-e29b-41d4-a716-446655440001",
                "750e8400-e29b-41d4-a716-446655440002",
                "WH-MAIN",
                "A1-B2-C3",
                "",  // Empty type
                "active",
                true, true,
                validAudit
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Invalid status throws") {
        REQUIRE_THROWS_WITH(
            dtos::LocationDto(
                "650e8400-e29b-41d4-a716-446655440001",
                "750e8400-e29b-41d4-a716-446655440002",
                "WH-MAIN",
                "A1-B2-C3",
                "picking",
                "",  // Empty status
                true, true,
                validAudit
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
}
