#include "warehouse/utils/DtoMapper.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

namespace warehouse {
namespace utils {

namespace {
    /**
     * @brief Convert timestamp to ISO 8601 string
     */
    std::string timestampToIso8601(const models::timestamp& t) {
        auto timeT = std::chrono::system_clock::to_time_t(t);
        std::tm tm = *std::gmtime(&timeT);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S") << "Z";
        return oss.str();
    }

    /**
     * @brief Convert Status enum to lowercase string
     */
    std::string statusToLowerString(models::Status status) {
        std::string str = models::statusToString(status);
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }
    
    /**
     * @brief Convert WarehouseType enum to lowercase string
     */
    std::string warehouseTypeToLowerString(models::WarehouseType type) {
        switch (type) {
            case models::WarehouseType::Distribution: return "distribution";
            case models::WarehouseType::Fulfillment: return "fulfillment";
            case models::WarehouseType::Storage: return "storage";
            case models::WarehouseType::ColdStorage: return "cold_storage";
            case models::WarehouseType::CrossDock: return "cross_dock";
            default: return "storage";
        }
    }
    
    /**
     * @brief Convert LocationType enum to lowercase string
     */
    std::string locationTypeToLowerString(models::LocationType type) {
        switch (type) {
            case models::LocationType::Bin: return "bin";
            case models::LocationType::Shelf: return "shelf";
            case models::LocationType::Rack: return "rack";
            case models::LocationType::Pallet: return "pallet";
            case models::LocationType::Floor: return "floor";
            case models::LocationType::Staging: return "staging";
            case models::LocationType::Receiving: return "receiving";
            case models::LocationType::Shipping: return "shipping";
            case models::LocationType::Picking: return "picking";
            case models::LocationType::Returns: return "returns";
            default: return "bin";
        }
    }
    
    /**
     * @brief Convert LocationStatus enum to lowercase string
     */
    std::string locationStatusToLowerString(models::LocationStatus status) {
        switch (status) {
            case models::LocationStatus::Active: return "active";
            case models::LocationStatus::Inactive: return "inactive";
            case models::LocationStatus::Full: return "full";
            case models::LocationStatus::Reserved: return "reserved";
            case models::LocationStatus::Damaged: return "damaged";
            case models::LocationStatus::Maintenance: return "maintenance";
            default: return "active";
        }
    }
}

dtos::WarehouseDto DtoMapper::toWarehouseDto(const models::Warehouse& warehouse) {
    // Convert timestamps
    std::string createdAt = timestampToIso8601(warehouse.getAudit().createdAt);
    std::string updatedAt = warehouse.getAudit().updatedAt 
        ? timestampToIso8601(*warehouse.getAudit().updatedAt) 
        : createdAt;
    
    // Convert enums to strings
    std::string statusStr = statusToLowerString(warehouse.getStatus());
    std::string typeStr = warehouseTypeToLowerString(warehouse.getType());
    
    // Convert address to JSON
    json addressJson = json::object();
    const auto& addr = warehouse.getAddress();
    addressJson["street"] = addr.street;
    if (addr.street2) addressJson["street2"] = *addr.street2;
    addressJson["city"] = addr.city;
    if (addr.state) addressJson["state"] = *addr.state;
    addressJson["postalCode"] = addr.postalCode;
    addressJson["country"] = addr.country;
    
    // Create DTO
    return dtos::WarehouseDto(
        warehouse.getId(),
        warehouse.getCode(),
        warehouse.getName(),
        statusStr,
        addressJson,
        typeStr,
        createdAt,
        updatedAt,
        warehouse.getDescription(),
        warehouse.getCoordinates() 
            ? std::make_optional(json{
                {"latitude", warehouse.getCoordinates()->latitude},
                {"longitude", warehouse.getCoordinates()->longitude}
              })
            : std::nullopt,
        warehouse.getTotalArea(),
        warehouse.getCapabilities().empty() 
            ? std::nullopt 
            : std::make_optional(json(warehouse.getCapabilities()))
    );
}

dtos::LocationDto DtoMapper::toLocationDto(
    const models::Location& location,
    const std::string& warehouseCode,
    const std::optional<std::string>& warehouseName) {
    
    // Convert timestamps
    std::string createdAt = timestampToIso8601(location.getAudit().createdAt);
    std::string updatedAt = location.getAudit().updatedAt 
        ? timestampToIso8601(*location.getAudit().updatedAt) 
        : createdAt;
    
    // Convert enums to strings
    std::string typeStr = locationTypeToLowerString(location.getType());
    std::string statusStr = locationStatusToLowerString(location.getStatus());
    
    // Create audit JSON
    json auditJson = json::object();
    auditJson["createdAt"] = createdAt;
    auditJson["createdBy"] = location.getAudit().createdBy;
    auditJson["updatedAt"] = updatedAt;
    if (location.getAudit().updatedBy) {
        auditJson["updatedBy"] = *location.getAudit().updatedBy;
    }
    
    // Convert dimensions to JSON if present
    std::optional<json> dimensionsJson = std::nullopt;
    if (location.getDimensions()) {
        const auto& dims = *location.getDimensions();
        dimensionsJson = json{
            {"length", dims.length},
            {"width", dims.width},
            {"height", dims.height},
            {"unit", dims.unit}
        };
    }
    
    // Convert maxWeight to JSON if present
    std::optional<json> maxWeightJson = std::nullopt;
    if (location.getMaxWeight()) {
        const auto& weight = *location.getMaxWeight();
        maxWeightJson = json{
            {"value", weight.value},
            {"unit", weight.unit}
        };
    }
    
    // Create DTO with correct parameter order
    return dtos::LocationDto(
        location.getId(),
        location.getWarehouseId(),
        warehouseCode,
        location.getCode(),
        typeStr,
        statusStr,
        location.isPickable(),
        location.isReceivable(),
        auditJson,
        warehouseName,              // Optional: warehouse name
        location.getAisle(),        // Optional: aisle
        location.getBay(),          // Optional: bay (was rack)
        location.getLevel(),        // Optional: level (was shelf)
        location.getName(),         // Optional: name
        location.getZone(),         // Optional: zone
        dimensionsJson,             // Optional: dimensions
        maxWeightJson,              // Optional: maxWeight
        location.getMaxVolume()     // Optional: maxVolume
    );
}

} // namespace utils
} // namespace warehouse
