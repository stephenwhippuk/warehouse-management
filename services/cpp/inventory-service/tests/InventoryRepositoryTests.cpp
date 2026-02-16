#include <catch2/catch_all.hpp>

#include "inventory/repositories/InventoryRepository.hpp"
#include "inventory/utils/Database.hpp"
#include "inventory/models/Inventory.hpp"
#include <http-framework/IServiceProvider.hpp>

#include <cstdlib>

using inventory::repositories::InventoryRepository;
using inventory::models::Inventory;
using inventory::models::InventoryStatus;
using inventory::models::QualityStatus;

// Mock IServiceProvider for tests with optional database (can be nullptr for validation tests)
class MockServiceProvider : public http::IServiceProvider {
private:
    std::shared_ptr<pqxx::connection> connection_;

public:
    explicit MockServiceProvider(std::shared_ptr<pqxx::connection> conn = nullptr)
        : connection_(conn) {}

    std::shared_ptr<void> getServiceInternal(
        const std::type_index& type, 
        const std::string& ns) override {
        if (type == std::type_index(typeid(pqxx::connection))) {
            // For validation tests, connection might be nullptr
            // Return it anyway so getService<T>() can handle it
            if (connection_) {
                return std::static_pointer_cast<void>(connection_);
            }
            return nullptr;
        }
        return nullptr;
    }

    std::shared_ptr<http::IServiceScope> createScope() override {
        throw std::runtime_error("createScope not implemented in MockServiceProvider");
    }
};

TEST_CASE("InventoryRepository validates UUID format", "[inventory][repository][validation]") {
    // For invalid UUIDs, the repository should throw before touching the DB
    // Use a real connection if available (for this test we need one since we call repository methods)
    const char* connStr = std::getenv("INVENTORY_TEST_DATABASE_URL");
    std::shared_ptr<pqxx::connection> conn;
    
    if (connStr) {
        conn = inventory::utils::Database::connect(connStr);
    } else {
        WARN("INVENTORY_TEST_DATABASE_URL not set; skipping UUID validation tests");
        return;
    }
    
    MockServiceProvider provider(conn);
    InventoryRepository repo(provider);

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
    MockServiceProvider provider(conn);
    InventoryRepository repo(provider);

    const std::string inventoryId       = "11111111-1111-1111-1111-111111111111";
    const std::string productId         = "22222222-2222-2222-2222-222222222222";
    const std::string warehouseId       = "33333333-3333-3333-3333-333333333333";
    const std::string locationId        = "44444444-4444-4444-4444-444444444444";
    const std::string lowStockInventory = "55555555-5555-5555-5555-555555555555";
    const std::string expiredInventory  = "66666666-6666-6666-6666-666666666666";
    const std::string tempInventoryId   = "77777777-7777-7777-7777-777777777777";

    // Clean up any existing test row first
    {
        pqxx::work cleanup(*conn);
        // Remove any existing rows for the test product to ensure a clean fixture
        cleanup.exec_params("DELETE FROM inventory WHERE product_id = $1", productId);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", inventoryId);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", lowStockInventory);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", expiredInventory);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", tempInventoryId);
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

    SECTION("aggregate queries return correct totals for product") {
        int total = repo.getTotalQuantityByProduct(productId);
        int available = repo.getAvailableQuantityByProduct(productId);

        // 100 (main) + 5 (low stock) + 10 (expired)
        REQUIRE(total == 115);
        REQUIRE(available == 115);
    }

    SECTION("create inserts a new row that can be fetched") {
        Inventory toCreate;
        toCreate.setId(tempInventoryId);
        toCreate.setProductId(productId);
        toCreate.setWarehouseId(warehouseId);
        toCreate.setLocationId(locationId);
        toCreate.setQuantity(20);
        toCreate.setAvailableQuantity(20);
        toCreate.setReservedQuantity(0);
        toCreate.setAllocatedQuantity(0);
        toCreate.setStatus(InventoryStatus::AVAILABLE);
        toCreate.setQualityStatus(QualityStatus::NOT_TESTED);
        toCreate.setNotes(std::optional<std::string>("created via repository test"));
        toCreate.setCreatedBy(std::optional<std::string>("test-user"));
        toCreate.setUpdatedBy(std::optional<std::string>("test-user"));

        auto created = repo.create(toCreate);

        REQUIRE(created.getId() == tempInventoryId);
        REQUIRE(created.getProductId() == productId);
        REQUIRE(created.getWarehouseId() == warehouseId);
        REQUIRE(created.getLocationId() == locationId);
        REQUIRE(created.getQuantity() == 20);
        REQUIRE(created.getAvailableQuantity() == 20);

        auto fetched = repo.findById(tempInventoryId);
        REQUIRE(fetched.has_value());
        REQUIRE(fetched->getId() == tempInventoryId);
        REQUIRE(fetched->getQuantity() == 20);
        REQUIRE(fetched->getAvailableQuantity() == 20);
    }

    SECTION("update modifies an existing row") {
        Inventory toCreate;
        toCreate.setId(tempInventoryId);
        toCreate.setProductId(productId);
        toCreate.setWarehouseId(warehouseId);
        toCreate.setLocationId(locationId);
        toCreate.setQuantity(10);
        toCreate.setAvailableQuantity(10);
        toCreate.setReservedQuantity(0);
        toCreate.setAllocatedQuantity(0);
        toCreate.setStatus(InventoryStatus::AVAILABLE);
        toCreate.setQualityStatus(QualityStatus::NOT_TESTED);
        toCreate.setCreatedBy(std::optional<std::string>("test-user"));
        toCreate.setUpdatedBy(std::optional<std::string>("test-user"));

        auto created = repo.create(toCreate);

        created.setQuantity(25);
        created.setAvailableQuantity(25);
        created.setStatus(InventoryStatus::RESERVED);
        created.setNotes(std::optional<std::string>("updated via repository test"));
        created.setUpdatedBy(std::optional<std::string>("updater"));

        auto updated = repo.update(created);

        REQUIRE(updated.getId() == tempInventoryId);
        REQUIRE(updated.getQuantity() == 25);
        REQUIRE(updated.getAvailableQuantity() == 25);
        REQUIRE(updated.getStatus() == InventoryStatus::RESERVED);
        REQUIRE(updated.getUpdatedBy().has_value());
        REQUIRE(updated.getUpdatedBy().value() == "updater");

        auto fetched = repo.findById(tempInventoryId);
        REQUIRE(fetched.has_value());
        REQUIRE(fetched->getQuantity() == 25);
        REQUIRE(fetched->getAvailableQuantity() == 25);
        REQUIRE(fetched->getStatus() == InventoryStatus::RESERVED);
    }

    SECTION("deleteById removes the row") {
        Inventory toCreate;
        toCreate.setId(tempInventoryId);
        toCreate.setProductId(productId);
        toCreate.setWarehouseId(warehouseId);
        toCreate.setLocationId(locationId);
        toCreate.setQuantity(5);
        toCreate.setAvailableQuantity(5);
        toCreate.setReservedQuantity(0);
        toCreate.setAllocatedQuantity(0);
        toCreate.setStatus(InventoryStatus::AVAILABLE);
        toCreate.setQualityStatus(QualityStatus::NOT_TESTED);

        repo.create(toCreate);

        auto beforeDelete = repo.findById(tempInventoryId);
        REQUIRE(beforeDelete.has_value());

        bool firstDelete = repo.deleteById(tempInventoryId);
        REQUIRE(firstDelete);

        auto afterDelete = repo.findById(tempInventoryId);
        REQUIRE_FALSE(afterDelete.has_value());

        bool secondDelete = repo.deleteById(tempInventoryId);
        REQUIRE_FALSE(secondDelete);
    }

    // Clean up test data
    {
        pqxx::work cleanup(*conn);
        cleanup.exec_params("DELETE FROM inventory WHERE product_id = $1", productId);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", inventoryId);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", lowStockInventory);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", expiredInventory);
        cleanup.exec_params("DELETE FROM inventory WHERE id = $1", tempInventoryId);
        cleanup.commit();
    }
}
