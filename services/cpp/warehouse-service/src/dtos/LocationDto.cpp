#include "warehouse/dtos/LocationDto.hpp"
#include <stdexcept>
#include <regex>

namespace warehouse {
namespace dtos {

LocationDto::LocationDto(
    const std::string& id,
    const std::string& warehouseId,
    const std::string& warehouseCode,
    const std::string& code,
    const std::string& type,
    const std::string& status,
    bool isPickable,
    bool isReceivable,
    const json& audit,
    const std::optional<std::string>& warehouseName,
    const std::optional<std::string>& aisle,
    const std::optional<std::string>& bay,
    const std::optional<std::string>& level,
    const std::optional<std::string>& name,
    const std::optional<std::string>& zone,
    const std::optional<json>& dimensions,
    const std::optional<json>& maxWeight,
    const std::optional<double>& maxVolume)
    : id_(id)
    , warehouseId_(warehouseId)
    , warehouseCode_(warehouseCode)
    , code_(code)
    , type_(type)
    , status_(status)
    , isPickable_(isPickable)
    , isReceivable_(isReceivable)
    , audit_(audit)
    , warehouseName_(warehouseName)
    , aisle_(aisle)
    , bay_(bay)
    , level_(level)
    , name_(name)
    , zone_(zone)
    , dimensions_(dimensions)
    , maxWeight_(maxWeight)
    , maxVolume_(maxVolume) {
    
    // Validate all required fields
    validateUuid(id_, "id");
    validateUuid(warehouseId_, "WarehouseId");
    
    if (warehouseCode_.empty()) {
        throw std::invalid_argument("WarehouseCode cannot be empty");
    }
    if (code_.empty()) {
        throw std::invalid_argument("code cannot be empty");
    }
    if (type_.empty()) {
        throw std::invalid_argument("type cannot be empty");
    }
    if (status_.empty()) {
        throw std::invalid_argument("status cannot be empty");
    }
    if (!audit_.is_object()) {
        throw std::invalid_argument("audit must be a JSON object");
    }
    
    // Validate optional numeric fields
    if (maxVolume_ && *maxVolume_ < 0) {
        throw std::invalid_argument("maxVolume must be non-negative");
    }
}

void LocationDto::validateUuid(const std::string& uuid, const std::string& fieldName) const {
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    if (!std::regex_match(uuid, uuidRegex)) {
        throw std::invalid_argument(fieldName + " must be a valid UUID");
    }
}

json LocationDto::toJson() const {
    json j = {
        {"id", id_},
        {"WarehouseId", warehouseId_},
        {"WarehouseCode", warehouseCode_},
        {"code", code_},
        {"type", type_},
        {"status", status_},
        {"isPickable", isPickable_},
        {"isReceivable", isReceivable_},
        {"audit", audit_}
    };
    
    // Add optional fields if present
    if (warehouseName_) j["WarehouseName"] = *warehouseName_;
    if (aisle_) j["aisle"] = *aisle_;
    if (bay_) j["bay"] = *bay_;
    if (level_) j["level"] = *level_;
    if (name_) j["name"] = *name_;
    if (zone_) j["zone"] = *zone_;
    if (dimensions_) j["dimensions"] = *dimensions_;
    if (maxWeight_) j["maxWeight"] = *maxWeight_;
    if (maxVolume_) j["maxVolume"] = *maxVolume_;
    
    return j;
}

} // namespace dtos
} // namespace warehouse
