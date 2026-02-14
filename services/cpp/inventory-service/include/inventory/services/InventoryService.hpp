#pragma once

#include "inventory/models/Inventory.hpp"
#include "inventory/repositories/InventoryRepository.hpp"
#include "inventory/utils/MessageBus.hpp"
#include "inventory/dtos/InventoryItemDto.hpp"
#include "inventory/dtos/InventoryOperationResultDto.hpp"
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
    
    // Inventory operations - return DTOs, not domain models
    std::optional<dtos::InventoryItemDto> getById(const std::string& id);
    std::vector<dtos::InventoryItemDto> getAll();
    std::vector<dtos::InventoryItemDto> getByProductId(const std::string& productId);
    std::vector<dtos::InventoryItemDto> getByWarehouseId(const std::string& warehouseId);
    std::vector<dtos::InventoryItemDto> getByLocationId(const std::string& locationId);
    std::vector<dtos::InventoryItemDto> getLowStock(int threshold);
    std::vector<dtos::InventoryItemDto> getExpired();
    
    dtos::InventoryItemDto create(const models::Inventory& inventory);
    dtos::InventoryItemDto update(const models::Inventory& inventory);
    bool remove(const std::string& id);
    
    // Stock operations - return operation result DTOs
    dtos::InventoryOperationResultDto reserve(const std::string& id, int quantity);
    dtos::InventoryOperationResultDto release(const std::string& id, int quantity);
    dtos::InventoryOperationResultDto allocate(const std::string& id, int quantity);
    dtos::InventoryOperationResultDto deallocate(const std::string& id, int quantity);
    dtos::InventoryOperationResultDto adjust(const std::string& id, int quantityChange, const std::string& reason);
    
    // Validation
    bool isValidInventory(const models::Inventory& inventory) const;
    
    // Aggregate queries
    int getTotalQuantityForProduct(const std::string& productId);
    int getAvailableQuantityForProduct(const std::string& productId);
    
private:
    std::shared_ptr<repositories::InventoryRepository> repository_;
    std::shared_ptr<utils::MessageBus> messageBus_;
    
    void validateQuantities(int quantity, int available, int reserved, int allocated) const;
    
    // DTO conversion helpers
    dtos::InventoryItemDto convertToDto(const models::Inventory& inventory) const;
    std::vector<dtos::InventoryItemDto> convertToDtos(const std::vector<models::Inventory>& inventories) const;
};

} // namespace services
} // namespace inventory
