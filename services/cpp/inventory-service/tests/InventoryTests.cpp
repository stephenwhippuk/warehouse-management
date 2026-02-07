#include <catch2/catch_all.hpp>
#include "inventory/models/Inventory.hpp"

using namespace inventory::models;

TEST_CASE("Inventory model creation", "[inventory][model]") {
    SECTION("Create inventory with required fields") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        
        REQUIRE(inv.getId() == "id-123");
        REQUIRE(inv.getProductId() == "prod-456");
        REQUIRE(inv.getWarehouseId() == "wh-789");
        REQUIRE(inv.getLocationId() == "loc-012");
        REQUIRE(inv.getQuantity() == 100);
        REQUIRE(inv.getAvailableQuantity() == 100);
        REQUIRE(inv.getReservedQuantity() == 0);
        REQUIRE(inv.getAllocatedQuantity() == 0);
        REQUIRE(inv.getStatus() == InventoryStatus::AVAILABLE);
        REQUIRE(inv.getQualityStatus() == QualityStatus::NOT_TESTED);
    }
}

TEST_CASE("Inventory JSON serialization", "[inventory][json]") {
    SECTION("Serialize inventory to JSON") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        inv.setBatchNumber("BATCH001");
        inv.setStatus(InventoryStatus::AVAILABLE);
        
        auto json = inv.toJson();
        
        REQUIRE(json["id"] == "id-123");
        REQUIRE(json["productId"] == "prod-456");
        REQUIRE(json["warehouseId"] == "wh-789");
        REQUIRE(json["locationId"] == "loc-012");
        REQUIRE(json["quantity"] == 100);
        REQUIRE(json["batchNumber"] == "BATCH001");
        REQUIRE(json["status"] == "available");
    }
    
    SECTION("Deserialize inventory from JSON") {
        nlohmann::json json = {
            {"id", "id-123"},
            {"productId", "prod-456"},
            {"warehouseId", "wh-789"},
            {"locationId", "loc-012"},
            {"quantity", 100},
            {"availableQuantity", 90},
            {"reservedQuantity", 10},
            {"allocatedQuantity", 0},
            {"batchNumber", "BATCH001"},
            {"status", "available"}
        };
        
        auto inv = Inventory::fromJson(json);
        
        REQUIRE(inv.getId() == "id-123");
        REQUIRE(inv.getProductId() == "prod-456");
        REQUIRE(inv.getQuantity() == 100);
        REQUIRE(inv.getAvailableQuantity() == 90);
        REQUIRE(inv.getReservedQuantity() == 10);
        REQUIRE(inv.getBatchNumber().value() == "BATCH001");
    }
}

TEST_CASE("Inventory status conversions", "[inventory][enum]") {
    SECTION("Convert status to string") {
        REQUIRE(inventoryStatusToString(InventoryStatus::AVAILABLE) == "available");
        REQUIRE(inventoryStatusToString(InventoryStatus::RESERVED) == "reserved");
        REQUIRE(inventoryStatusToString(InventoryStatus::ALLOCATED) == "allocated");
        REQUIRE(inventoryStatusToString(InventoryStatus::QUARANTINE) == "quarantine");
        REQUIRE(inventoryStatusToString(InventoryStatus::DAMAGED) == "damaged");
        REQUIRE(inventoryStatusToString(InventoryStatus::EXPIRED) == "expired");
        REQUIRE(inventoryStatusToString(InventoryStatus::RECALLED) == "recalled");
    }
    
    SECTION("Convert string to status") {
        REQUIRE(inventoryStatusFromString("available") == InventoryStatus::AVAILABLE);
        REQUIRE(inventoryStatusFromString("reserved") == InventoryStatus::RESERVED);
        REQUIRE(inventoryStatusFromString("damaged") == InventoryStatus::DAMAGED);
    }
}

TEST_CASE("Quality status conversions", "[inventory][enum]") {
    SECTION("Convert quality status to string") {
        REQUIRE(qualityStatusToString(QualityStatus::PASSED) == "passed");
        REQUIRE(qualityStatusToString(QualityStatus::FAILED) == "failed");
        REQUIRE(qualityStatusToString(QualityStatus::PENDING) == "pending");
        REQUIRE(qualityStatusToString(QualityStatus::NOT_TESTED) == "not_tested");
    }
    
    SECTION("Convert string to quality status") {
        REQUIRE(qualityStatusFromString("passed") == QualityStatus::PASSED);
        REQUIRE(qualityStatusFromString("failed") == QualityStatus::FAILED);
        REQUIRE(qualityStatusFromString("pending") == QualityStatus::PENDING);
        REQUIRE(qualityStatusFromString("not_tested") == QualityStatus::NOT_TESTED);
    }
}

TEST_CASE("Inventory operations", "[inventory][operations]") {
    SECTION("Reserve inventory") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        inv.reserve(30);
        
        REQUIRE(inv.getAvailableQuantity() == 70);
        REQUIRE(inv.getReservedQuantity() == 30);
        REQUIRE(inv.getQuantity() == 100);
    }
    
    SECTION("Release inventory") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        inv.reserve(30);
        inv.release(10);
        
        REQUIRE(inv.getAvailableQuantity() == 80);
        REQUIRE(inv.getReservedQuantity() == 20);
    }
    
    SECTION("Allocate inventory") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        inv.reserve(30);
        inv.allocate(20);
        
        REQUIRE(inv.getAvailableQuantity() == 70);
        REQUIRE(inv.getReservedQuantity() == 10);
        REQUIRE(inv.getAllocatedQuantity() == 20);
    }
    
    SECTION("Deallocate inventory") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        inv.reserve(30);
        inv.allocate(20);
        inv.deallocate(10);
        
        REQUIRE(inv.getAvailableQuantity() == 80);
        REQUIRE(inv.getAllocatedQuantity() == 10);
    }
    
    SECTION("Adjust inventory quantity") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        inv.adjust(50, "Received new stock");
        
        REQUIRE(inv.getQuantity() == 150);
        REQUIRE(inv.getAvailableQuantity() == 150);
    }
    
    SECTION("Reserve more than available throws exception") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        REQUIRE_THROWS_AS(inv.reserve(150), std::runtime_error);
    }
}

TEST_CASE("Inventory expiry check", "[inventory][expiry]") {
    SECTION("Check expired inventory") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        inv.setExpirationDate("2020-01-01");
        
        REQUIRE(inv.isExpired() == true);
    }
    
    SECTION("Check non-expired inventory") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        inv.setExpirationDate("2099-12-31");
        
        REQUIRE(inv.isExpired() == false);
    }
    
    SECTION("No expiry date means not expired") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        REQUIRE(inv.isExpired() == false);
    }
}

TEST_CASE("Low stock check", "[inventory][low-stock]") {
    SECTION("Check low stock") {
        Inventory inv("id-123", "prod-456", "wh-789", "loc-012", 100);
        
        REQUIRE(inv.isLowStock(150) == true);
        REQUIRE(inv.isLowStock(100) == false);
        REQUIRE(inv.isLowStock(50) == false);
    }
}
