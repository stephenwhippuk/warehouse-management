#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace inventory {
namespace dtos {

using json = nlohmann::json;

/**
 * @brief Result of an inventory operation DTO
 * 
 * Conforms to InventoryOperationResultDto contract v1.0
 * Returns result of stock operations: reserve, release, allocate, deallocate, adjust
 */
class InventoryOperationResultDto {
public:
    /**
     * @brief Construct operation result DTO with all required fields
     * @param id Inventory record identifier (UUID)
     * @param productId Product identifier (UUID)
     * @param quantity Current total quantity (NonNegativeInteger)
     * @param reservedQuantity Reserved quantity after operation (NonNegativeInteger)
     * @param allocatedQuantity Allocated quantity after operation (NonNegativeInteger)
     * @param availableQuantity Available quantity after operation (NonNegativeInteger)
     * @param operation Operation performed (reserve, release, allocate, deallocate, adjust)
     * @param operationQuantity Quantity affected by the operation (positive or negative)
     * @param success Whether the operation succeeded
     * @param message Optional message about the operation
     */
    InventoryOperationResultDto(const std::string& id,
                                const std::string& productId,
                                int quantity,
                                int reservedQuantity,
                                int allocatedQuantity,
                                int availableQuantity,
                                const std::string& operation,
                                int operationQuantity,
                                bool success,
                                const std::optional<std::string>& message = std::nullopt);

    // Getters (immutable)
    std::string getId() const { return id_; }
    std::string getProductId() const { return productId_; }
    int getQuantity() const { return quantity_; }
    int getReservedQuantity() const { return reservedQuantity_; }
    int getAllocatedQuantity() const { return allocatedQuantity_; }
    int getAvailableQuantity() const { return availableQuantity_; }
    std::string getOperation() const { return operation_; }
    int getOperationQuantity() const { return operationQuantity_; }
    bool getSuccess() const { return success_; }
    std::optional<std::string> getMessage() const { return message_; }

    // Serialization
    json toJson() const;

private:
    std::string id_;
    std::string productId_;
    int quantity_;
    int reservedQuantity_;
    int allocatedQuantity_;
    int availableQuantity_;
    std::string operation_;
    int operationQuantity_;
    bool success_;
    std::optional<std::string> message_;

    // Validation
    void validateUuid(const std::string& uuid, const std::string& fieldName) const;
    void validateNonNegativeInteger(int value, const std::string& fieldName) const;
    void validateOperation(const std::string& operation) const;
};

} // namespace dtos
} // namespace inventory
