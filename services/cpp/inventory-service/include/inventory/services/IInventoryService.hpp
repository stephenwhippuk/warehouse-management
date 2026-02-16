#pragma once

#include "inventory/dtos/InventoryItemDto.hpp"
#include "inventory/dtos/InventoryOperationResultDto.hpp"
#include "inventory/models/Inventory.hpp"
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace inventory {
namespace services {

/**
 * @brief Interface for inventory business logic
 */
class IInventoryService {
public:
    virtual ~IInventoryService() = default;
    
    // Inventory operations - return DTOs, not domain models
    virtual std::optional<dtos::InventoryItemDto> getById(const std::string& id) = 0;
    virtual std::vector<dtos::InventoryItemDto> getAll() = 0;
    virtual std::vector<dtos::InventoryItemDto> getByProductId(const std::string& productId) = 0;
    virtual std::vector<dtos::InventoryItemDto> getByWarehouseId(const std::string& warehouseId) = 0;
    virtual std::vector<dtos::InventoryItemDto> getByLocationId(const std::string& locationId) = 0;
    virtual std::vector<dtos::InventoryItemDto> getLowStock(int threshold) = 0;
    virtual std::vector<dtos::InventoryItemDto> getExpired() = 0;
    
    virtual dtos::InventoryItemDto create(const models::Inventory& inventory) = 0;
    virtual dtos::InventoryItemDto update(const models::Inventory& inventory) = 0;
    virtual bool remove(const std::string& id) = 0;
    
    // Stock operations - return operation result DTOs
    virtual dtos::InventoryOperationResultDto reserve(const std::string& id, int quantity) = 0;
    virtual dtos::InventoryOperationResultDto release(const std::string& id, int quantity) = 0;
    virtual dtos::InventoryOperationResultDto allocate(const std::string& id, int quantity) = 0;
    virtual dtos::InventoryOperationResultDto deallocate(const std::string& id, int quantity) = 0;
    virtual dtos::InventoryOperationResultDto adjust(const std::string& id, int quantityChange, const std::string& reason) = 0;
    
    // Validation
    virtual bool isValidInventory(const models::Inventory& inventory) const = 0;
    
    // Aggregate queries
    virtual int getTotalQuantityForProduct(const std::string& productId) = 0;
    virtual int getAvailableQuantityForProduct(const std::string& productId) = 0;
};

} // namespace services
} // namespace inventory
