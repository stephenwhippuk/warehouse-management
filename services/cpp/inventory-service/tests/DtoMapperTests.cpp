#include <catch2/catch_all.hpp>
#include "inventory/utils/DtoMapper.hpp"
#include "inventory/models/Inventory.hpp"
#include "inventory/dtos/InventoryItemDto.hpp"
#include "inventory/dtos/InventoryOperationResultDto.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace inventory;

// Helper to create valid ISO 8601 timestamp
std::string createIso8601Timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// Helper to create a valid inventory model
models::Inventory createValidInventory() {
    models::Inventory inv(
        "550e8400-e29b-41d4-a716-446655440000",  // Valid UUID
        "650e8400-e29b-41d4-a716-446655440001",  // productId
        "750e8400-e29b-41d4-a716-446655440002",  // warehouseId
        "850e8400-e29b-41d4-a716-446655440003",  // locationId
        100  // quantity
    );
    
    inv.setStatus(models::InventoryStatus::AVAILABLE);
    inv.setQualityStatus(models::QualityStatus::PASSED);
    inv.setCreatedAt(createIso8601Timestamp());
    inv.setUpdatedAt(createIso8601Timestamp());
    
    return inv;
}

TEST_CASE("DtoMapper converts valid inventory to DTO", "[dto][mapper]") {
    auto inv = createValidInventory();
    
    SECTION("Successful conversion with required fields only") {
        auto dto = utils::DtoMapper::toInventoryItemDto(
            inv,
            "SKU-12345",      // productSku
            "WH-MAIN",        // warehouseCode
            "A1-B2-C3"        // locationCode
        );
        
        REQUIRE(dto.getId() == inv.getId());
        REQUIRE(dto.getProductId() == inv.getProductId());
        REQUIRE(dto.getProductSku() == "SKU-12345");
        REQUIRE(dto.getWarehouseId() == inv.getWarehouseId());
        REQUIRE(dto.getWarehouseCode() == "WH-MAIN");
        REQUIRE(dto.getLocationId() == inv.getLocationId());
        REQUIRE(dto.getLocationCode() == "A1-B2-C3");
        REQUIRE(dto.getQuantity() == 100);
        REQUIRE(dto.getAvailableQuantity() == 100);
        REQUIRE(dto.getReservedQuantity() == 0);
        REQUIRE(dto.getAllocatedQuantity() == 0);
        REQUIRE(dto.getStatus() == "available");
    }
    
    SECTION("Successful conversion with optional fields") {
        auto dto = utils::DtoMapper::toInventoryItemDto(
            inv,
            "SKU-12345",
            "WH-MAIN",
            "A1-B2-C3",
            "Widget Product",     // productName
            "Electronics",        // productCategory
            "Main Warehouse",     // warehouseName
            "A",                  // locationAisle
            "1",                  // locationBay
            "2"                   // locationLevel
        );
        
        REQUIRE(dto.getProductName().has_value());
        REQUIRE(dto.getProductName().value() == "Widget Product");
        REQUIRE(dto.getProductCategory().has_value());
        REQUIRE(dto.getProductCategory().value() == "Electronics");
        REQUIRE(dto.getWarehouseName().has_value());
        REQUIRE(dto.getWarehouseName().value() == "Main Warehouse");
        REQUIRE(dto.getLocationAisle().has_value());
        REQUIRE(dto.getLocationAisle().value() == "A");
    }
}

TEST_CASE("DtoMapper handles different inventory statuses", "[dto][mapper][status]") {
    SECTION("AVAILABLE status") {
        auto inv = createValidInventory();
        inv.setStatus(models::InventoryStatus::AVAILABLE);
        
        auto dto = utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1");
        REQUIRE(dto.getStatus() == "available");
    }
    
    SECTION("RESERVED status") {
        auto inv = createValidInventory();
        inv.setStatus(models::InventoryStatus::RESERVED);
        
        auto dto = utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1");
        REQUIRE(dto.getStatus() == "reserved");
    }
    
    SECTION("ALLOCATED status") {
        auto inv = createValidInventory();
        inv.setStatus(models::InventoryStatus::ALLOCATED);
        
        auto dto = utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1");
        REQUIRE(dto.getStatus() == "allocated");
    }
    
    SECTION("QUARANTINE status") {
        auto inv = createValidInventory();
        inv.setStatus(models::InventoryStatus::QUARANTINE);
        
        auto dto = utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1");
        REQUIRE(dto.getStatus() == "quarantine");
    }
    
    SECTION("DAMAGED status") {
        auto inv = createValidInventory();
        inv.setStatus(models::InventoryStatus::DAMAGED);
        
        auto dto = utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1");
        REQUIRE(dto.getStatus() == "damaged");
    }
}

TEST_CASE("DtoMapper throws on invalid model data", "[dto][mapper][validation]") {
    SECTION("Invalid UUID in model ID") {
        auto inv = createValidInventory();
        // Can't easily change ID after construction, but we can test with malformed references
        
        // This should throw when DTO constructor validates the UUID
        REQUIRE_THROWS_AS(
            utils::DtoMapper::toInventoryItemDto(
                models::Inventory("not-a-uuid", "prod-1", "wh-1", "loc-1", 100),
                "SKU-1", "WH-1", "LOC-1"
            ),
            std::invalid_argument
        );
    }
    
    SECTION("Invalid productId UUID") {
        REQUIRE_THROWS_AS(
            utils::DtoMapper::toInventoryItemDto(
                models::Inventory(
                    "550e8400-e29b-41d4-a716-446655440000",
                    "invalid-uuid",  // Bad productId
                    "750e8400-e29b-41d4-a716-446655440002",
                    "850e8400-e29b-41d4-a716-446655440003",
                    100
                ),
                "SKU-1", "WH-1", "LOC-1"
            ),
            std::invalid_argument
        );
    }
    
    SECTION("Empty timestamp throws") {
        auto inv = createValidInventory();
        inv.setCreatedAt("");  // Invalid empty timestamp
        
        REQUIRE_THROWS_WITH(
            utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1"),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Invalid timestamp format throws") {
        auto inv = createValidInventory();
        inv.setCreatedAt("2024-13-99 25:99:99");  // Invalid date format
        
        REQUIRE_THROWS_WITH(
            utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1"),
            Catch::Matchers::ContainsSubstring("ISO 8601")
        );
    }
    
    SECTION("Missing createdAt throws") {
        auto inv = createValidInventory();
        // If createdAt is optional in model and returns empty string
        inv.setCreatedAt("");
        
        REQUIRE_THROWS(
            utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1")
        );
    }
}

TEST_CASE("DtoMapper handles quantity edge cases", "[dto][mapper][quantities]") {
    SECTION("Zero quantities are valid") {
        auto inv = createValidInventory();
        inv.setQuantity(0);
        inv.setAvailableQuantity(0);
        inv.setReservedQuantity(0);
        inv.setAllocatedQuantity(0);
        
        auto dto = utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1");
        REQUIRE(dto.getQuantity() == 0);
        REQUIRE(dto.getAvailableQuantity() == 0);
    }
    
    SECTION("Large quantities are valid") {
        auto inv = createValidInventory();
        inv.setQuantity(1000000);
        inv.setAvailableQuantity(1000000);
        
        auto dto = utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1");
        REQUIRE(dto.getQuantity() == 1000000);
    }
    
    SECTION("Negative quantities throw (if validation enabled)") {
        auto inv = createValidInventory();
        inv.setQuantity(-10);
        
        // DTO should reject negative quantities
        REQUIRE_THROWS(
            utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", "LOC-1")
        );
    }
}

TEST_CASE("DtoMapper validates identity fields", "[dto][mapper][identity]") {
    auto inv = createValidInventory();
    
    SECTION("Empty productSku throws") {
        REQUIRE_THROWS_WITH(
            utils::DtoMapper::toInventoryItemDto(inv, "", "WH-1", "LOC-1"),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Empty warehouseCode throws") {
        REQUIRE_THROWS_WITH(
            utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "", "LOC-1"),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Empty locationCode throws") {
        REQUIRE_THROWS_WITH(
            utils::DtoMapper::toInventoryItemDto(inv, "SKU-1", "WH-1", ""),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
}

TEST_CASE("DtoMapper creates operation result DTOs", "[dto][mapper][operation]") {
    auto inv = createValidInventory();
    
    SECTION("Reserve operation result") {
        auto result = utils::DtoMapper::toInventoryOperationResultDto(
            inv, "reserve", 10, true, std::nullopt
        );
        
        REQUIRE(result.getOperation() == "reserve");
        REQUIRE(result.getOperationQuantity() == 10);
        REQUIRE(result.getSuccess() == true);
        REQUIRE_FALSE(result.getMessage().has_value());
        
        // Should contain inventory details
        REQUIRE(result.getId() == inv.getId());
        REQUIRE(result.getProductId() == inv.getProductId());
        REQUIRE(result.getQuantity() == inv.getQuantity());
    }
    
    SECTION("Adjust operation with reason") {
        auto result = utils::DtoMapper::toInventoryOperationResultDto(
            inv, "adjust", -5, true, "Damaged goods"
        );
        
        REQUIRE(result.getOperation() == "adjust");
        REQUIRE(result.getOperationQuantity() == -5);
        REQUIRE(result.getMessage().has_value());
        REQUIRE(result.getMessage().value() == "Damaged goods");
    }
    
    SECTION("Failed operation") {
        auto result = utils::DtoMapper::toInventoryOperationResultDto(
            inv, "allocate", 50, false, "Insufficient available quantity"
        );
        
        REQUIRE(result.getSuccess() == false);
        REQUIRE(result.getMessage().has_value());
        REQUIRE(result.getMessage().value() == "Insufficient available quantity");
    }
}

TEST_CASE("InventoryItemDto validates on construction", "[dto][validation]") {
    SECTION("Valid DTO construction succeeds") {
        REQUIRE_NOTHROW(
            dtos::InventoryItemDto(
                "550e8400-e29b-41d4-a716-446655440000",  // id
                "650e8400-e29b-41d4-a716-446655440001",  // productId
                "SKU-12345",                             // productSku
                "750e8400-e29b-41d4-a716-446655440002",  // warehouseId
                "WH-MAIN",                               // warehouseCode
                "850e8400-e29b-41d4-a716-446655440003",  // locationId
                "A1-B2-C3",                              // locationCode
                100,                                      // quantity
                100,                                      // availableQuantity
                0,                                        // reservedQuantity
                0,                                        // allocatedQuantity
                "available",                              // status
                createIso8601Timestamp(),                // createdAt
                createIso8601Timestamp()                 // updatedAt
            )
        );
    }
    
    SECTION("Invalid UUID throws") {
        REQUIRE_THROWS_WITH(
            dtos::InventoryItemDto(
                "not-a-uuid",  // Invalid ID
                "650e8400-e29b-41d4-a716-446655440001",
                "SKU-12345",
                "750e8400-e29b-41d4-a716-446655440002",
                "WH-MAIN",
                "850e8400-e29b-41d4-a716-446655440003",
                "A1-B2-C3",
                100, 100, 0, 0,
                "available",
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("valid UUID")
        );
    }
    
    SECTION("Invalid status throws") {
        REQUIRE_THROWS_WITH(
            dtos::InventoryItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "650e8400-e29b-41d4-a716-446655440001",
                "SKU-12345",
                "750e8400-e29b-41d4-a716-446655440002",
                "WH-MAIN",
                "850e8400-e29b-41d4-a716-446655440003",
                "A1-B2-C3",
                100, 100, 0, 0,
                "invalid-status",  // Invalid
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("status")
        );
    }
    
    SECTION("Negative quantity throws") {
        REQUIRE_THROWS_WITH(
            dtos::InventoryItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "650e8400-e29b-41d4-a716-446655440001",
                "SKU-12345",
                "750e8400-e29b-41d4-a716-446655440002",
                "WH-MAIN",
                "850e8400-e29b-41d4-a716-446655440003",
                "A1-B2-C3",
                -10,  // Negative
                -10, 0, 0,
                "available",
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("non-negative")
        );
    }
    
    SECTION("Empty identity field throws") {
        REQUIRE_THROWS_WITH(
            dtos::InventoryItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "650e8400-e29b-41d4-a716-446655440001",
                "",  // Empty productSku
                "750e8400-e29b-41d4-a716-446655440002",
                "WH-MAIN",
                "850e8400-e29b-41d4-a716-446655440003",
                "A1-B2-C3",
                100, 100, 0, 0,
                "available",
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Invalid ISO 8601 timestamp throws") {
        REQUIRE_THROWS_WITH(
            dtos::InventoryItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "650e8400-e29b-41d4-a716-446655440001",
                "SKU-12345",
                "750e8400-e29b-41d4-a716-446655440002",
                "WH-MAIN",
                "850e8400-e29b-41d4-a716-446655440003",
                "A1-B2-C3",
                100, 100, 0, 0,
                "available",
                "not-a-timestamp",  // Invalid
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("ISO 8601")
        );
    }
}

TEST_CASE("InventoryOperationResultDto validates on construction", "[dto][validation]") {
    SECTION("Valid operation result construction") {
        REQUIRE_NOTHROW(
            dtos::InventoryOperationResultDto(
                "550e8400-e29b-41d4-a716-446655440000",  // id
                "650e8400-e29b-41d4-a716-446655440001",  // productId
                100,                                      // quantity
                10,                                       // reservedQuantity
                0,                                        // allocatedQuantity
                90,                                       // availableQuantity
                "reserve",                                // operation
                10,                                       // operationQuantity
                true,                                     // success
                std::nullopt
            )
        );
    }
    
    SECTION("Empty operation throws") {
        REQUIRE_THROWS_WITH(
            dtos::InventoryOperationResultDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "650e8400-e29b-41d4-a716-446655440001",
                100, 10, 0, 90,
                "",  // Empty operation
                10,
                true,
                std::nullopt
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Invalid operation type throws") {
        REQUIRE_THROWS_WITH(
            dtos::InventoryOperationResultDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "650e8400-e29b-41d4-a716-446655440001",
                100, 10, 0, 90,
                "invalid-operation",  // Not in allowed list
                10,
                true,
                std::nullopt
            ),
            Catch::Matchers::ContainsSubstring("operation")
        );
    }
}
