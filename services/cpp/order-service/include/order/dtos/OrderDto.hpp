#pragma once

#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

namespace order {
namespace dtos {

using json = nlohmann::json;

/**
 * @brief Complete order with referenced entity data DTO
 * 
 * Conforms to OrderDto contract v1.0
 * Contains order data plus identity fields from Warehouse
 */
class OrderDto {
public:
    /**
     * @brief Construct order DTO with all required fields
     * @param id Order identifier (UUID)
     * @param orderNumber Human-readable order number
     * @param customerId Customer identifier
     * @param warehouseId Warehouse identity field (UUID)
     * @param warehouseCode Warehouse code identity field
     * @param orderDate Date order was placed (ISO 8601)
     * @param priority Order priority (low, normal, high, urgent)
     * @param type Order type
     * @param status Order status
     * @param totalItems Total number of line items (NonNegativeInteger)
     * @param totalQuantity Total quantity across all items (NonNegativeInteger)
     * @param createdAt Created timestamp (ISO 8601)
     * @param updatedAt Last updated timestamp (ISO 8601)
     * @param customerName Optional customer name
     * @param customerEmail Optional customer email
     * @param warehouseName Optional warehouse name (cached)
     * @param requestedShipDate Optional requested ship date
     * @param requestedDeliveryDate Optional requested delivery date
     * @param shippingAddress Optional shipping address (JSON object)
     * @param billingAddress Optional billing address (JSON object)
     * @param notes Optional order notes
     * @param tags Optional tags
     * @param metadata Optional metadata (JSON object)
     */
    OrderDto(const std::string& id,
             const std::string& orderNumber,
             const std::string& customerId,
             const std::string& warehouseId,
             const std::string& warehouseCode,
             const std::string& orderDate,
             const std::string& priority,
             const std::string& type,
             const std::string& status,
             int totalItems,
             int totalQuantity,
             const std::string& createdAt,
             const std::string& updatedAt,
             const std::optional<std::string>& customerName = std::nullopt,
             const std::optional<std::string>& customerEmail = std::nullopt,
             const std::optional<std::string>& warehouseName = std::nullopt,
             const std::optional<std::string>& requestedShipDate = std::nullopt,
             const std::optional<std::string>& requestedDeliveryDate = std::nullopt,
             const std::optional<json>& shippingAddress = std::nullopt,
             const std::optional<json>& billingAddress = std::nullopt,
             const std::optional<std::string>& notes = std::nullopt,
             const std::optional<std::vector<std::string>>& tags = std::nullopt,
             const std::optional<json>& metadata = std::nullopt);

    // Getters (immutable)
    std::string getId() const { return id_; }
    std::string getOrderNumber() const { return orderNumber_; }
    std::string getCustomerId() const { return customerId_; }
    std::string getWarehouseId() const { return warehouseId_; }
    std::string getWarehouseCode() const { return warehouseCode_; }
    std::string getOrderDate() const { return orderDate_; }
    std::string getPriority() const { return priority_; }
    std::string getType() const { return type_; }
    std::string getStatus() const { return status_; }
    int getTotalItems() const { return totalItems_; }
    int getTotalQuantity() const { return totalQuantity_; }
    std::string getCreatedAt() const { return createdAt_; }
    std::string getUpdatedAt() const { return updatedAt_; }
    std::optional<std::string> getCustomerName() const { return customerName_; }
    std::optional<std::string> getCustomerEmail() const { return customerEmail_; }
    std::optional<std::string> getWarehouseName() const { return warehouseName_; }
    std::optional<std::string> getRequestedShipDate() const { return requestedShipDate_; }
    std::optional<std::string> getRequestedDeliveryDate() const { return requestedDeliveryDate_; }
    std::optional<json> getShippingAddress() const { return shippingAddress_; }
    std::optional<json> getBillingAddress() const { return billingAddress_; }
    std::optional<std::string> getNotes() const { return notes_; }
    std::optional<std::vector<std::string>> getTags() const { return tags_; }
    std::optional<json> getMetadata() const { return metadata_; }

    // Serialization
    json toJson() const;

private:
    // Required fields
    std::string id_;
    std::string orderNumber_;
    std::string customerId_;
    std::string warehouseId_;
    std::string warehouseCode_;
    std::string orderDate_;
    std::string priority_;
    std::string type_;
    std::string status_;
    int totalItems_;
    int totalQuantity_;
    std::string createdAt_;
    std::string updatedAt_;
    
    // Optional fields
    std::optional<std::string> customerName_;
    std::optional<std::string> customerEmail_;
    std::optional<std::string> warehouseName_;
    std::optional<std::string> requestedShipDate_;
    std::optional<std::string> requestedDeliveryDate_;
    std::optional<json> shippingAddress_;
    std::optional<json> billingAddress_;
    std::optional<std::string> notes_;
    std::optional<std::vector<std::string>> tags_;
    std::optional<json> metadata_;

    // Validation
    void validateUuid(const std::string& uuid, const std::string& fieldName) const;
    void validateNonNegativeInteger(int value, const std::string& fieldName) const;
    void validateDateTime(const std::string& dateTime, const std::string& fieldName) const;
    void validatePriority(const std::string& priority) const;
};

} // namespace dtos
} // namespace order
