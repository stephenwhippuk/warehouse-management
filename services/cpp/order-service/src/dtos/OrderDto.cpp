#include "order/dtos/OrderDto.hpp"
#include <stdexcept>
#include <regex>
#include <algorithm>

namespace order {
namespace dtos {

OrderDto::OrderDto(
    const std::string& id,
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
    const std::optional<std::string>& customerName,
    const std::optional<std::string>& customerEmail,
    const std::optional<std::string>& warehouseName,
    const std::optional<std::string>& requestedShipDate,
    const std::optional<std::string>& requestedDeliveryDate,
    const std::optional<json>& shippingAddress,
    const std::optional<json>& billingAddress,
    const std::optional<std::string>& notes,
    const std::optional<std::vector<std::string>>& tags,
    const std::optional<json>& metadata)
    : id_(id)
    , orderNumber_(orderNumber)
    , customerId_(customerId)
    , warehouseId_(warehouseId)
    , warehouseCode_(warehouseCode)
    , orderDate_(orderDate)
    , priority_(priority)
    , type_(type)
    , status_(status)
    , totalItems_(totalItems)
    , totalQuantity_(totalQuantity)
    , createdAt_(createdAt)
    , updatedAt_(updatedAt)
    , customerName_(customerName)
    , customerEmail_(customerEmail)
    , warehouseName_(warehouseName)
    , requestedShipDate_(requestedShipDate)
    , requestedDeliveryDate_(requestedDeliveryDate)
    , shippingAddress_(shippingAddress)
    , billingAddress_(billingAddress)
    , notes_(notes)
    , tags_(tags)
    , metadata_(metadata) {
    
    // Validate all required fields
    validateUuid(id_, "id");
    validateUuid(warehouseId_, "WarehouseId");
    
    if (orderNumber_.empty()) {
        throw std::invalid_argument("orderNumber cannot be empty");
    }
    if (customerId_.empty()) {
        throw std::invalid_argument("customerId cannot be empty");
    }
    if (warehouseCode_.empty()) {
        throw std::invalid_argument("WarehouseCode cannot be empty");
    }
    if (priority_.empty()) {
        throw std::invalid_argument("priority cannot be empty");
    }
    if (type_.empty()) {
        throw std::invalid_argument("type cannot be empty");
    }
    if (status_.empty()) {
        throw std::invalid_argument("status cannot be empty");
    }
    
    validateNonNegativeInteger(totalItems_, "totalItems");
    validateNonNegativeInteger(totalQuantity_, "totalQuantity");
    
    validatePriority(priority_);
    validateDateTime(orderDate_, "orderDate");
    validateDateTime(createdAt_, "createdAt");
    validateDateTime(updatedAt_, "updatedAt");
}

void OrderDto::validateUuid(const std::string& uuid, const std::string& fieldName) const {
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    if (!std::regex_match(uuid, uuidRegex)) {
        throw std::invalid_argument(fieldName + " must be a valid UUID");
    }
}

void OrderDto::validateNonNegativeInteger(int value, const std::string& fieldName) const {
    if (value < 0) {
        throw std::invalid_argument(fieldName + " must be non-negative");
    }
}

void OrderDto::validateDateTime(const std::string& dateTime, const std::string& fieldName) const {
    if (dateTime.empty()) {
        throw std::invalid_argument(fieldName + " cannot be empty");
    }
    // ISO 8601 format validation
    static const std::regex isoRegex(
        R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})$)"
    );
    if (!std::regex_match(dateTime, isoRegex)) {
        throw std::invalid_argument(fieldName + " must be in ISO 8601 format");
    }
}

void OrderDto::validatePriority(const std::string& priority) const {
    static const std::vector<std::string> validPriorities = {
        "low", "normal", "high", "urgent"
    };
    
    if (std::find(validPriorities.begin(), validPriorities.end(), priority) == validPriorities.end()) {
        throw std::invalid_argument("priority must be one of: low, normal, high, urgent");
    }
}

json OrderDto::toJson() const {
    json j = {
        {"id", id_},
        {"orderNumber", orderNumber_},
        {"customerId", customerId_},
        {"WarehouseId", warehouseId_},
        {"WarehouseCode", warehouseCode_},
        {"orderDate", orderDate_},
        {"priority", priority_},
        {"type", type_},
        {"status", status_},
        {"totalItems", totalItems_},
        {"totalQuantity", totalQuantity_},
        {"createdAt", createdAt_},
        {"updatedAt", updatedAt_}
    };
    
    // Add optional fields if present
    if (customerName_) j["customerName"] = *customerName_;
    if (customerEmail_) j["customerEmail"] = *customerEmail_;
    if (warehouseName_) j["WarehouseName"] = *warehouseName_;
    if (requestedShipDate_) j["requestedShipDate"] = *requestedShipDate_;
    if (requestedDeliveryDate_) j["requestedDeliveryDate"] = *requestedDeliveryDate_;
    if (shippingAddress_) j["shippingAddress"] = *shippingAddress_;
    if (billingAddress_) j["billingAddress"] = *billingAddress_;
    if (notes_) j["notes"] = *notes_;
    if (tags_) j["tags"] = *tags_;
    if (metadata_) j["metadata"] = *metadata_;
    
    return j;
}

} // namespace dtos
} // namespace order
