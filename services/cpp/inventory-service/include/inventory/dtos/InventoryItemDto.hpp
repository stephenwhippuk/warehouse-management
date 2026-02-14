#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace inventory {
namespace dtos {

using json = nlohmann::json;

/**
 * @brief Complete inventory item with referenced entity data DTO
 * 
 * Conforms to InventoryItemDto contract v1.0
 * Contains inventory data plus identity fields from Product, Warehouse, and Location
 */
class InventoryItemDto {
public:
    /**
     * @brief Construct inventory item DTO with all required fields
     * @param id Inventory record identifier (UUID)
     * @param productId Product identity field (UUID) 
     * @param productSku Product SKU identity field
     * @param warehouseId Warehouse identity field (UUID)
     * @param warehouseCode Warehouse code identity field
     * @param locationId Location identity field (UUID)
     * @param locationCode Location code identity field
     * @param quantity Current quantity on hand (NonNegativeInteger)
     * @param reservedQuantity Quantity reserved for orders (NonNegativeInteger)
     * @param allocatedQuantity Quantity allocated to shipments (NonNegativeInteger) 
     * @param availableQuantity Available quantity (computed, NonNegativeInteger)
     * @param status Inventory status
     * @param createdAt Created timestamp (ISO 8601)
     * @param updatedAt Last updated timestamp (ISO 8601)
     * @param productName Optional product name (cached)
     * @param productCategory Optional product category (cached)
     * @param warehouseName Optional warehouse name (cached)
     * @param locationAisle Optional location aisle (cached)
     * @param locationBay Optional location bay (cached)
     * @param locationLevel Optional location level (cached)
     * @param serialNumber Optional serial number
     * @param batchNumber Optional batch or lot number
     * @param expirationDate Optional expiration date (ISO 8601)
     */
    InventoryItemDto(const std::string& id,
                     const std::string& productId,
                     const std::string& productSku,
                     const std::string& warehouseId,
                     const std::string& warehouseCode,
                     const std::string& locationId,
                     const std::string& locationCode,
                     int quantity,
                     int reservedQuantity,
                     int allocatedQuantity,
                     int availableQuantity,
                     const std::string& status,
                     const std::string& createdAt,
                     const std::string& updatedAt,
                     const std::optional<std::string>& productName = std::nullopt,
                     const std::optional<std::string>& productCategory = std::nullopt,
                     const std::optional<std::string>& warehouseName = std::nullopt,
                     const std::optional<std::string>& locationAisle = std::nullopt,
                     const std::optional<std::string>& locationBay = std::nullopt,
                     const std::optional<std::string>& locationLevel = std::nullopt,
                     const std::optional<std::string>& serialNumber = std::nullopt,
                     const std::optional<std::string>& batchNumber = std::nullopt,
                     const std::optional<std::string>& expirationDate = std::nullopt);

    // Getters (immutable)
    std::string getId() const { return id_; }
    std::string getProductId() const { return productId_; }
    std::string getProductSku() const { return productSku_; }
    std::string getWarehouseId() const { return warehouseId_; }
    std::string getWarehouseCode() const { return warehouseCode_; }
    std::string getLocationId() const { return locationId_; }
    std::string getLocationCode() const { return locationCode_; }
    int getQuantity() const { return quantity_; }
    int getReservedQuantity() const { return reservedQuantity_; }
    int getAllocatedQuantity() const { return allocatedQuantity_; }
    int getAvailableQuantity() const { return availableQuantity_; }
    std::string getStatus() const { return status_; }
    std::string getCreatedAt() const { return createdAt_; }
    std::string getUpdatedAt() const { return updatedAt_; }
    std::optional<std::string> getProductName() const { return productName_; }
    std::optional<std::string> getProductCategory() const { return productCategory_; }
    std::optional<std::string> getWarehouseName() const { return warehouseName_; }
    std::optional<std::string> getLocationAisle() const { return locationAisle_; }
    std::optional<std::string> getLocationBay() const { return locationBay_; }
    std::optional<std::string> getLocationLevel() const { return locationLevel_; }
    std::optional<std::string> getSerialNumber() const { return serialNumber_; }
    std::optional<std::string> getBatchNumber() const { return batchNumber_; }
    std::optional<std::string> getExpirationDate() const { return expirationDate_; }

    // Serialization
    json toJson() const;

private:
    // Required fields
    std::string id_;
    std::string productId_;
    std::string productSku_;
    std::string warehouseId_;
    std::string warehouseCode_;
    std::string locationId_;
    std::string locationCode_;
    int quantity_;
    int reservedQuantity_;
    int allocatedQuantity_;
    int availableQuantity_;
    std::string status_;
    std::string createdAt_;
    std::string updatedAt_;
    
    // Optional fields
    std::optional<std::string> productName_;
    std::optional<std::string> productCategory_;
    std::optional<std::string> warehouseName_;
    std::optional<std::string> locationAisle_;
    std::optional<std::string> locationBay_;
    std::optional<std::string> locationLevel_;
    std::optional<std::string> serialNumber_;
    std::optional<std::string> batchNumber_;
    std::optional<std::string> expirationDate_;

    // Validation
    void validateUuid(const std::string& uuid, const std::string& fieldName) const;
    void validateNonNegativeInteger(int value, const std::string& fieldName) const;
    void validateDateTime(const std::string& dateTime, const std::string& fieldName) const;
    void validateInventoryStatus(const std::string& status) const;
};

} // namespace dtos
} // namespace inventory
