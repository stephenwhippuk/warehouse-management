#include "inventory/utils/DtoMapper.hpp"
#include <algorithm>

namespace inventory {
namespace utils {

dtos::InventoryItemDto DtoMapper::toInventoryItemDto(
    const models::Inventory& inventory,
    const std::string& productSku,
    const std::string& warehouseCode,
    const std::string& locationCode,
    const std::optional<std::string>& productName,
    const std::optional<std::string>& productCategory,
    const std::optional<std::string>& warehouseName,
    const std::optional<std::string>& locationAisle,
    const std::optional<std::string>& locationBay,
    const std::optional<std::string>& locationLevel) {
    
    std::string statusStr = inventoryStatusToLowerString(inventory);
    
    // Extract timestamps (handle optional -> string conversion)
    std::string createdAt = inventory.getCreatedAt().value_or("");
    std::string updatedAt = inventory.getUpdatedAt().value_or("");
    
    return dtos::InventoryItemDto(
        inventory.getId(),
        inventory.getProductId(),
        productSku,
        inventory.getWarehouseId(),
        warehouseCode,
        inventory.getLocationId(),
        locationCode,
        inventory.getQuantity(),
        inventory.getReservedQuantity(),
        inventory.getAllocatedQuantity(),
        inventory.getAvailableQuantity(),
        statusStr,
        createdAt,
        updatedAt,
        productName,
        productCategory,
        warehouseName,
        locationAisle,
        locationBay,
        locationLevel,
        inventory.getSerialNumber(),
        inventory.getBatchNumber(),
        inventory.getExpirationDate()
    );
}

dtos::InventoryOperationResultDto DtoMapper::toInventoryOperationResultDto(
    const models::Inventory& inventory,
    const std::string& operation,
    int operationQuantity,
    bool success,
    const std::optional<std::string>& message) {
    
    return dtos::InventoryOperationResultDto(
        inventory.getId(),
        inventory.getProductId(),
        inventory.getQuantity(),
        inventory.getReservedQuantity(),
        inventory.getAllocatedQuantity(),
        inventory.getAvailableQuantity(),
        operation,
        operationQuantity,
        success,
        message
    );
}

std::string DtoMapper::inventoryStatusToLowerString(const models::Inventory& inventory) {
    std::string statusStr = models::inventoryStatusToString(inventory.getStatus());
    std::transform(statusStr.begin(), statusStr.end(), statusStr.begin(), ::tolower);
    return statusStr;
}

} // namespace utils
} // namespace inventory
