#include "warehouse/dtos/WarehouseDto.hpp"
#include <stdexcept>
#include <regex>

namespace warehouse {
namespace dtos {

WarehouseDto::WarehouseDto(
    const std::string& id,
    const std::string& code,
    const std::string& name,
    const std::string& status,
    const json& address,
    const std::string& type,
    const std::string& createdAt,
    const std::string& updatedAt,
    const std::optional<std::string>& description,
    const std::optional<json>& coordinates,
    const std::optional<double>& totalArea,
    const std::optional<double>& storageCapacity,
    const std::optional<json>& contactPerson,
    const std::optional<json>& operatingHours,
    const std::optional<std::vector<std::string>>& capabilities,
    const std::optional<int>& zones,
    const std::optional<int>& dockDoors,
    const std::optional<bool>& isActive)
    : id_(id)
    , code_(code)
    , name_(name)
    , status_(status)
    , address_(address)
    , type_(type)
    , createdAt_(createdAt)
    , updatedAt_(updatedAt)
    , description_(description)
    , coordinates_(coordinates)
    , totalArea_(totalArea)
    , storageCapacity_(storageCapacity)
    , contactPerson_(contactPerson)
    , operatingHours_(operatingHours)
    , capabilities_(capabilities)
    , zones_(zones)
    , dockDoors_(dockDoors)
    , isActive_(isActive) {
    
    // Validate required fields
    validateUuid(id_, "id");
    
    if (code_.empty()) {
        throw std::invalid_argument("code cannot be empty");
    }
    if (name_.empty()) {
        throw std::invalid_argument("name cannot be empty");
    }
    
    validateStatus(status_);
    
    if (address_.is_null()) {
        throw std::invalid_argument("address cannot be null");
    }
    
    if (type_.empty()) {
        throw std::invalid_argument("type cannot be empty");
    }
    
    validateDateTime(createdAt_, "createdAt");
    validateDateTime(updatedAt_, "updatedAt");
}

void WarehouseDto::validateUuid(const std::string& uuid, const std::string& fieldName) const {
    static const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    if (!std::regex_match(uuid, uuidRegex)) {
        throw std::invalid_argument(fieldName + " must be a valid UUID");
    }
}

void WarehouseDto::validateDateTime(const std::string& dateTime, const std::string& fieldName) const {
    if (dateTime.empty()) {
        throw std::invalid_argument(fieldName + " cannot be empty");
    }
    static const std::regex isoRegex(
        R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})$)"
    );
    if (!std::regex_match(dateTime, isoRegex)) {
        throw std::invalid_argument(fieldName + " must be in ISO 8601 format");
    }
}

void WarehouseDto::validateStatus(const std::string& status) const {
    static const std::vector<std::string> validStatuses = {
        "active", "inactive", "archived"
    };
    
    if (std::find(validStatuses.begin(), validStatuses.end(), status) == validStatuses.end()) {
        throw std::invalid_argument("status must be one of: active, inactive, archived");
    }
}

json WarehouseDto::toJson() const {
    json j = {
        {"id", id_},
        {"code", code_},
        {"name", name_},
        {"status", status_},
        {"address", address_},
        {"type", type_},
        {"createdAt", createdAt_},
        {"updatedAt", updatedAt_}
    };
    
    // Add optional fields if present
    if (description_) j["description"] = *description_;
    if (coordinates_) j["coordinates"] = *coordinates_;
    if (totalArea_) j["totalArea"] = *totalArea_;
    if (storageCapacity_) j["storageCapacity"] = *storageCapacity_;
    if (contactPerson_) j["contactPerson"] = *contactPerson_;
    if (operatingHours_) j["operatingHours"] = *operatingHours_;
    if (capabilities_) j["capabilities"] = *capabilities_;
    if (zones_) j["zones"] = *zones_;
    if (dockDoors_) j["dockDoors"] = *dockDoors_;
    if (isActive_) j["isActive"] = *isActive_;
    
    return j;
}

} // namespace dtos
} // namespace warehouse
