#pragma once

#include "inventory/services/IInventoryService.hpp"
#include "inventory/models/Inventory.hpp"
#include "inventory/repositories/InventoryRepository.hpp"
#include <warehouse/messaging/EventPublisher.hpp>
#include "inventory/dtos/InventoryItemDto.hpp"
#include "inventory/dtos/InventoryOperationResultDto.hpp"
#include <http-framework/IServiceProvider.hpp>
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace inventory {
namespace services {

class InventoryService : public IInventoryService {
public:
    // Constructor receives IServiceProvider for dependency resolution
    explicit InventoryService(http::IServiceProvider& provider);
    
    // Inventory operations - return DTOs, not domain models
    std::optional<dtos::InventoryItemDto> getById(const std::string& id) override;
    std::vector<dtos::InventoryItemDto> getAll() override;
    std::vector<dtos::InventoryItemDto> getByProductId(const std::string& productId) override;
    std::vector<dtos::InventoryItemDto> getByWarehouseId(const std::string& warehouseId) override;
    std::vector<dtos::InventoryItemDto> getByLocationId(const std::string& locationId) override;
    std::vector<dtos::InventoryItemDto> getLowStock(int threshold) override;
    std::vector<dtos::InventoryItemDto> getExpired() override;
    
    dtos::InventoryItemDto create(const models::Inventory& inventory) override;
    dtos::InventoryItemDto update(const models::Inventory& inventory) override;
    bool remove(const std::string& id) override;
    
    // Stock operations - return operation result DTOs
    dtos::InventoryOperationResultDto reserve(const std::string& id, int quantity) override;
    dtos::InventoryOperationResultDto release(const std::string& id, int quantity) override;
    dtos::InventoryOperationResultDto allocate(const std::string& id, int quantity) override;
    dtos::InventoryOperationResultDto deallocate(const std::string& id, int quantity) override;
    dtos::InventoryOperationResultDto adjust(const std::string& id, int quantityChange, const std::string& reason) override;
    
    // Validation
    bool isValidInventory(const models::Inventory& inventory) const override;
    
    // Aggregate queries
    int getTotalQuantityForProduct(const std::string& productId) override;
    int getAvailableQuantityForProduct(const std::string& productId) override;
    
private:
    std::shared_ptr<repositories::InventoryRepository> repository_;
    std::shared_ptr<warehouse::messaging::EventPublisher> eventPublisher_;
    
    void validateQuantities(int quantity, int available, int reserved, int allocated) const;
    
    // DTO conversion helpers
    dtos::InventoryItemDto convertToDto(const models::Inventory& inventory) const;
    std::vector<dtos::InventoryItemDto> convertToDtos(const std::vector<models::Inventory>& inventories) const;
};

} // namespace services
} // namespace inventory
