#pragma once

#include "inventory/models/Inventory.hpp"
#include <http-framework/IServiceProvider.hpp>
#include <pqxx/pqxx>
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace inventory {
namespace repositories {

class InventoryRepository {
public:
    explicit InventoryRepository(http::IServiceProvider& provider);
    
    // CRUD operations
    std::optional<models::Inventory> findById(const std::string& id);
    std::vector<models::Inventory> findAll();
    std::vector<models::Inventory> findByProductId(const std::string& productId);
    std::vector<models::Inventory> findByWarehouseId(const std::string& warehouseId);
    std::vector<models::Inventory> findByLocationId(const std::string& locationId);
    std::vector<models::Inventory> findLowStock(int threshold);
    std::vector<models::Inventory> findExpired();
    std::optional<models::Inventory> findByProductAndLocation(
        const std::string& productId, 
        const std::string& locationId
    );
    
    models::Inventory create(const models::Inventory& inventory);
    models::Inventory update(const models::Inventory& inventory);
    bool deleteById(const std::string& id);
    
    // Aggregate queries
    int getTotalQuantityByProduct(const std::string& productId);
    int getAvailableQuantityByProduct(const std::string& productId);
    
private:
    std::shared_ptr<pqxx::connection> db_;
};

} // namespace repositories
} // namespace inventory
