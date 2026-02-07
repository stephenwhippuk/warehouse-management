#pragma once

#include "inventory/models/Inventory.hpp"
#include "inventory/repositories/InventoryRepository.hpp"
#include "inventory/utils/MessageBus.hpp"
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace inventory {
namespace services {

class InventoryService {
public:
    explicit InventoryService(std::shared_ptr<repositories::InventoryRepository> repository,
                             std::shared_ptr<utils::MessageBus> messageBus);
    
    // Inventory operations
    std::optional<models::Inventory> getById(const std::string& id);
    std::vector<models::Inventory> getAll();
    std::vector<models::Inventory> getByProductId(const std::string& productId);
    std::vector<models::Inventory> getByWarehouseId(const std::string& warehouseId);
    std::vector<models::Inventory> getByLocationId(const std::string& locationId);
    std::vector<models::Inventory> getLowStock(int threshold);
    std::vector<models::Inventory> getExpired();
    
    models::Inventory create(const models::Inventory& inventory);
    models::Inventory update(const models::Inventory& inventory);
    bool remove(const std::string& id);
    
    // Stock operations
    void reserve(const std::string& id, int quantity);
    void release(const std::string& id, int quantity);
    void allocate(const std::string& id, int quantity);
    void deallocate(const std::string& id, int quantity);
    void adjust(const std::string& id, int quantityChange, const std::string& reason);
    
    // Validation
    bool isValidInventory(const models::Inventory& inventory) const;
    
    // Aggregate queries
    int getTotalQuantityForProduct(const std::string& productId);
    int getAvailableQuantityForProduct(const std::string& productId);
    
private:
    std::shared_ptr<repositories::InventoryRepository> repository_;
    std::shared_ptr<utils::MessageBus> messageBus_;
    
    void validateQuantities(int quantity, int available, int reserved, int allocated) const;
};

} // namespace services
} // namespace inventory
