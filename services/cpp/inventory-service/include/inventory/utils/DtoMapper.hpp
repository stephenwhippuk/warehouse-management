#pragma once

#include "inventory/models/Inventory.hpp"
#include "inventory/dtos/InventoryItemDto.hpp"
#include "inventory/dtos/InventoryOperationResultDto.hpp"
#include <string>

namespace inventory {
namespace utils {

/**
 * @brief Utility class to convert domain models to DTOs
 * 
 * Keeps domain models internal while exposing only DTOs to external layers
 */
class DtoMapper {
public:
    /**
     * @brief Convert Inventory model to InventoryItemDto
     * @param inventory The domain model
     * @param productSku Product SKU (identity field from Product reference)
     * @param warehouseCode Warehouse code (identity field from Warehouse reference)
     * @param locationCode Location code (identity field from Location reference)
     * @param productName Optional product name (cached data)
     * @param productCategory Optional product category (cached data)
     * @param warehouseName Optional warehouse name (cached data)
     * @param locationAisle Optional location aisle (cached data)
     * @param locationBay Optional location bay (cached data)
     * @param locationLevel Optional location level (cached data)
     * @return InventoryItemDto with all required and optional fields
     */
    static dtos::InventoryItemDto toInventoryItemDto(
        const models::Inventory& inventory,
        const std::string& productSku,
        const std::string& warehouseCode,
        const std::string& locationCode,
        const std::optional<std::string>& productName = std::nullopt,
        const std::optional<std::string>& productCategory = std::nullopt,
        const std::optional<std::string>& warehouseName = std::nullopt,
        const std::optional<std::string>& locationAisle = std::nullopt,
        const std::optional<std::string>& locationBay = std::nullopt,
        const std::optional<std::string>& locationLevel = std::nullopt);

    /**
     * @brief Create InventoryOperationResultDto from operation details
     * @param inventory The inventory after the operation
     * @param operation Operation type (reserve, release, allocate, deallocate, adjust)
     * @param operationQuantity Quantity affected by the operation
     * @param success Whether the operation succeeded
     * @param message Optional message about the operation
     * @return InventoryOperationResultDto with operation details
     */
    static dtos::InventoryOperationResultDto toInventoryOperationResultDto(
        const models::Inventory& inventory,
        const std::string& operation,
        int operationQuantity,
        bool success,
        const std::optional<std::string>& message = std::nullopt);

private:
    /**
     * @brief Convert InventoryStatus enum to lowercase string
     */
    static std::string inventoryStatusToLowerString(const models::Inventory& inventory);
};

} // namespace utils
} // namespace inventory
