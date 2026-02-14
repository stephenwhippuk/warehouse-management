#include "inventory/dtos/InventoryOperationResultDto.hpp"
#include <stdexcept>
#include <regex>

namespace inventory {
namespace dtos {

InventoryOperationResultDto::InventoryOperationResultDto(
    const std::string& id,
    const std::string& productId,
    int quantity,
    int reservedQuantity,
    int allocatedQuantity,
    int availableQuantity,
    const std::string& operation,
    int operationQuantity,
    bool success,
    const std::optional<std::string>& message)
    : id_(id)
    , productId_(productId)
    , quantity_(quantity)
    , reservedQuantity_(reservedQuantity)
    , allocatedQuantity_(allocatedQuantity)
    , availableQuantity_(availableQuantity)
    , operation_(operation)
    , operationQuantity_(operationQuantity)
    , success_(success)
    , message_(message) {
    
    // Validate all fields
    validateUuid(id_, "id");
    validateUuid(productId_, "productId");
    validateNonNegativeInteger(quantity_, "quantity");
    validateNonNegativeInteger(reservedQuantity_, "reservedQuantity");
    validateNonNegativeInteger(allocatedQuantity_, "allocatedQuantity");
    validateNonNegativeInteger(availableQuantity_, "availableQuantity");
    validateOperation(operation_);
}

void InventoryOperationResultDto::validateUuid(const std::string& uuid, const std::string& fieldName) const {
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    if (!std::regex_match(uuid, uuidRegex)) {
        throw std::invalid_argument(fieldName + " must be a valid UUID");
    }
}

void InventoryOperationResultDto::validateNonNegativeInteger(int value, const std::string& fieldName) const {
    if (value < 0) {
        throw std::invalid_argument(fieldName + " must be non-negative");
    }
}

void InventoryOperationResultDto::validateOperation(const std::string& operation) const {
    static const std::vector<std::string> validOperations = {
        "reserve", "release", "allocate", "deallocate", "adjust"
    };
    
    if (std::find(validOperations.begin(), validOperations.end(), operation) == validOperations.end()) {
        throw std::invalid_argument("Operation must be one of: reserve, release, allocate, deallocate, adjust");
    }
}

json InventoryOperationResultDto::toJson() const {
    json j = {
        {"id", id_},
        {"ProductId", productId_},
        {"quantity", quantity_},
        {"reservedQuantity", reservedQuantity_},
        {"allocatedQuantity", allocatedQuantity_},
        {"availableQuantity", availableQuantity_},
        {"operation", operation_},
        {"operationQuantity", operationQuantity_},
        {"success", success_}
    };
    
    if (message_) {
        j["message"] = *message_;
    }
    
    return j;
}

} // namespace dtos
} // namespace inventory
