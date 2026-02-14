#include "inventory/dtos/InventoryItemDto.hpp"
#include <stdexcept>
#include <regex>

namespace inventory {
namespace dtos {

InventoryItemDto::InventoryItemDto(
    const std::string& id,
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
    const std::optional<std::string>& productName,
    const std::optional<std::string>& productCategory,
    const std::optional<std::string>& warehouseName,
    const std::optional<std::string>& locationAisle,
    const std::optional<std::string>& locationBay,
    const std::optional<std::string>& locationLevel,
    const std::optional<std::string>& serialNumber,
    const std::optional<std::string>& batchNumber,
    const std::optional<std::string>& expirationDate)
    : id_(id)
    , productId_(productId)
    , productSku_(productSku)
    , warehouseId_(warehouseId)
    , warehouseCode_(warehouseCode)
    , locationId_(locationId)
    , locationCode_(locationCode)
    , quantity_(quantity)
    , reservedQuantity_(reservedQuantity)
    , allocatedQuantity_(allocatedQuantity)
    , availableQuantity_(availableQuantity)
    , status_(status)
    , createdAt_(createdAt)
    , updatedAt_(updatedAt)
    , productName_(productName)
    , productCategory_(productCategory)
    , warehouseName_(warehouseName)
    , locationAisle_(locationAisle)
    , locationBay_(locationBay)
    , locationLevel_(locationLevel)
    , serialNumber_(serialNumber)
    , batchNumber_(batchNumber)
    , expirationDate_(expirationDate) {
    
    // Validate all required fields
    validateUuid(id_, "id");
    validateUuid(productId_, "ProductId");
    validateUuid(warehouseId_, "WarehouseId");
    validateUuid(locationId_, "LocationId");
    
    if (productSku_.empty()) {
        throw std::invalid_argument("ProductSku cannot be empty");
    }
    if (warehouseCode_.empty()) {
        throw std::invalid_argument("WarehouseCode cannot be empty");
    }
    if (locationCode_.empty()) {
        throw std::invalid_argument("LocationCode cannot be empty");
    }
    
    validateNonNegativeInteger(quantity_, "quantity");
    validateNonNegativeInteger(reservedQuantity_, "reservedQuantity");
    validateNonNegativeInteger(allocatedQuantity_, "allocatedQuantity");
    validateNonNegativeInteger(availableQuantity_, "availableQuantity");
    
    validateInventoryStatus(status_);
    validateDateTime(createdAt_, "createdAt");
    validateDateTime(updatedAt_, "updatedAt");
    
    // Validate optional DateTime fields
    if (expirationDate_) {
        validateDateTime(*expirationDate_, "expirationDate");
    }
}

void InventoryItemDto::validateUuid(const std::string& uuid, const std::string& fieldName) const {
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    if (!std::regex_match(uuid, uuidRegex)) {
        throw std::invalid_argument(fieldName + " must be a valid UUID");
    }
}

void InventoryItemDto::validateNonNegativeInteger(int value, const std::string& fieldName) const {
    if (value < 0) {
        throw std::invalid_argument(fieldName + " must be non-negative");
    }
}

void InventoryItemDto::validateDateTime(const std::string& dateTime, const std::string& fieldName) const {
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

void InventoryItemDto::validateInventoryStatus(const std::string& status) const {
    static const std::vector<std::string> validStatuses = {
        "available", "reserved", "allocated", "quarantine", "damaged", "expired", "recalled"
    };
    
    if (std::find(validStatuses.begin(), validStatuses.end(), status) == validStatuses.end()) {
        throw std::invalid_argument("Status must be a valid InventoryStatus value");
    }
}

json InventoryItemDto::toJson() const {
    json j = {
        {"id", id_},
        {"ProductId", productId_},
        {"ProductSku", productSku_},
        {"WarehouseId", warehouseId_},
        {"WarehouseCode", warehouseCode_},
        {"LocationId", locationId_},
        {"LocationCode", locationCode_},
        {"quantity", quantity_},
        {"reservedQuantity", reservedQuantity_},
        {"allocatedQuantity", allocatedQuantity_},
        {"availableQuantity", availableQuantity_},
        {"status", status_},
        {"createdAt", createdAt_},
        {"updatedAt", updatedAt_}
    };
    
    // Add optional fields if present
    if (productName_) j["ProductName"] = *productName_;
    if (productCategory_) j["ProductCategory"] = *productCategory_;
    if (warehouseName_) j["WarehouseName"] = *warehouseName_;
    if (locationAisle_) j["LocationAisle"] = *locationAisle_;
    if (locationBay_) j["LocationBay"] = *locationBay_;
    if (locationLevel_) j["LocationLevel"] = *locationLevel_;
    if (serialNumber_) j["serialNumber"] = *serialNumber_;
    if (batchNumber_) j["batchNumber"] = *batchNumber_;
    if (expirationDate_) j["expirationDate"] = *expirationDate_;
    
    return j;
}

} // namespace dtos
} // namespace inventory
