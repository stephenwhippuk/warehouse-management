#include <catch2/catch_all.hpp>

#include "inventory/repositories/InventoryRepository.hpp"
#include "inventory/utils/Database.hpp"

#include <cstdlib>

using inventory::repositories::InventoryRepository;

TEST_CASE("InventoryRepository validates UUID format", "[inventory][repository][validation]") {
    // For invalid UUIDs, the repository should throw before touching the DB
    std::shared_ptr<pqxx::connection> nullConn; // not used when validation fails
    InventoryRepository repo(nullConn);

    SECTION("findById rejects invalid UUID") {
        REQUIRE_THROWS_AS(repo.findById("not-a-uuid"), std::invalid_argument);
    }

    SECTION("findByProductId rejects invalid UUID") {
        REQUIRE_THROWS_AS(repo.findByProductId("bad-product-id"), std::invalid_argument);
    }

    SECTION("findByWarehouseId rejects invalid UUID") {
        REQUIRE_THROWS_AS(repo.findByWarehouseId("bad-warehouse-id"), std::invalid_argument);
    }

    SECTION("findByLocationId rejects invalid UUID") {
        REQUIRE_THROWS_AS(repo.findByLocationId("bad-location-id"), std::invalid_argument);
    }

    SECTION("findByProductAndLocation rejects invalid UUIDs") {
        REQUIRE_THROWS_AS(repo.findByProductAndLocation("bad-product-id", "11111111-1111-1111-1111-111111111111"), std::invalid_argument);
        REQUIRE_THROWS_AS(repo.findByProductAndLocation("11111111-1111-1111-1111-111111111111", "bad-location-id"), std::invalid_argument);
    }
}

TEST_CASE("InventoryRepository basic DB-backed operations", "[inventory][repository][db]") {
    const char* connStr = std::getenv("INVENTORY_TEST_DATABASE_URL");
    if (!connStr) {
        WARN("INVENTORY_TEST_DATABASE_URL not set; skipping DB-backed InventoryRepository tests");
        return;
    }

    auto conn = inventory::utils::Database::connect(connStr);
    InventoryRepository repo(conn);

    const std::string inventoryId       = "11111111-1111-1111-1111-111111111111";
    const std::string productId         = "22222222-2222-2222-2222-222222222222";
    const std::string warehouseId       = "33333333-3333-3333-3333-333333333333";
    const std::string locationId        = "44444444-4444-4444-4444-444444444444";
    const std::string lowStockInventory = "55555555-5555-5555-5555-555555555555";
    const std::string expiredInventory  = "66666666-6666-6666-6666-666666666666";

    // Clean up any existing test row first
    {
        pqxx::work cleanup(*conn);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", inventoryId);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", lowStockInventory);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", expiredInventory);
        cleanup.commit();
    }

    // Insert a known inventory row
    {
        pqxx::work txn(*conn);
        txn.exec_params(
            "INSERT INTO inventory (id, product_id, warehouse_id, location_id, quantity, available_quantity, reserved_quantity, allocated_quantity, status, quality_status) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)",
            inventoryId,
            productId,
            warehouseId,
            locationId,
            100,   // quantity
            100,   // available_quantity
            0,     // reserved_quantity
            0,     // allocated_quantity
            "available",
            "not_tested"
        );

        // Low stock row (available_quantity below typical threshold like 50)
        txn.exec_params(
            "INSERT INTO inventory (id, product_id, warehouse_id, location_id, quantity, available_quantity, reserved_quantity, allocated_quantity, status, quality_status) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)",
            lowStockInventory,
            productId,
            warehouseId,
            locationId,
            5,   // quantity (must equal available + reserved + allocated)
            5,   // available_quantity (low)
            0,
            0,
            "available",
            "not_tested"
        );

        // Expired row (expiration_date in the past)
        txn.exec_params(
            "INSERT INTO inventory (id, product_id, warehouse_id, location_id, quantity, available_quantity, reserved_quantity, allocated_quantity, status, quality_status, expiration_date) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)",
            expiredInventory,
            productId,
            warehouseId,
            locationId,
            10,
            10,
            0,
            0,
            "expired",
            "not_tested",
            "2000-01-01"
        );
        txn.commit();
    }

    SECTION("findById returns inserted row") {
        auto result = repo.findById(inventoryId);
        REQUIRE(result.has_value());
        REQUIRE(result->getId() == inventoryId);
        REQUIRE(result->getProductId() == productId);
        REQUIRE(result->getWarehouseId() == warehouseId);
        REQUIRE(result->getLocationId() == locationId);
        REQUIRE(result->getQuantity() == 100);
        REQUIRE(result->getAvailableQuantity() == 100);
    }

    SECTION("findByProductAndLocation finds the row") {
        auto result = repo.findByProductAndLocation(productId, locationId);
        REQUIRE(result.has_value());
        REQUIRE(result->getId() == inventoryId);
    }

    SECTION("findByProductId returns the row") {
        auto list = repo.findByProductId(productId);
        REQUIRE_FALSE(list.empty());
        bool found = false;
        for (const auto& inv : list) {
            if (inv.getId() == inventoryId) {
                found = true;
                break;
            }
        }
        REQUIRE(found);
    }

    SECTION("findByWarehouseId returns the row") {
        auto list = repo.findByWarehouseId(warehouseId);
        REQUIRE_FALSE(list.empty());
        bool found = false;
        for (const auto& inv : list) {
            if (inv.getId() == inventoryId) {
                found = true;
                break;
            }
        }
        REQUIRE(found);
    }

    SECTION("findByLocationId returns the row") {
        auto list = repo.findByLocationId(locationId);
        REQUIRE_FALSE(list.empty());
        bool found = false;
        for (const auto& inv : list) {
            if (inv.getId() == inventoryId) {
                found = true;
                break;
            }
        }
        REQUIRE(found);
    }

    SECTION("findLowStock returns low stock items") {
        auto list = repo.findLowStock(50);
        bool foundLow = false;
        for (const auto& inv : list) {
            if (inv.getId() == lowStockInventory) {
                foundLow = true;
                break;
            }
        }
        REQUIRE(foundLow);
    }

    SECTION("findExpired returns expired items") {
        auto list = repo.findExpired();
        bool foundExpired = false;
        for (const auto& inv : list) {
            if (inv.getId() == expiredInventory) {
                foundExpired = true;
                break;
            }
        }
        REQUIRE(foundExpired);
    }

    // Clean up test data
    {
        pqxx::work cleanup(*conn);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", inventoryId);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", lowStockInventory);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", expiredInventory);
        cleanup.commit();
    }
}
