#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace warehouse {
namespace dtos {

using json = nlohmann::json;

/**
 * @brief Storage location data transfer object
 * 
 * Conforms to LocationDto contract v1.0
 * Contains location data plus identity fields from Warehouse
 */
class LocationDto {
public:
    /**
     * @brief Construct location DTO with all required fields
     * @param id Location identifier (UUID)
     * @param warehouseId Warehouse identity field (UUID)
     * @param warehouseCode Warehouse code identity field
     * @param code Location code
     * @param type Location type
     * @param status Location status
     * @param isPickable Whether location is pickable
     * @param isReceivable Whether location can receive inventory
     * @param audit Audit information (JSON object)
     * @param warehouseName Optional warehouse name (cached)
     * @param aisle Optional aisle identifier
     * @param bay Optional bay identifier
     * @param level Optional level/shelf identifier
     * @param name Optional location name
     * @param zone Optional zone identifier
     * @param dimensions Optional physical dimensions (JSON object)
     * @param maxWeight Optional maximum weight capacity (JSON object)
     * @param maxVolume Optional maximum volume capacity
     */
    LocationDto(const std::string& id,
                const std::string& warehouseId,
                const std::string& warehouseCode,
                const std::string& code,
                const std::string& type,
                const std::string& status,
                bool isPickable,
                bool isReceivable,
                const json& audit,
                const std::optional<std::string>& warehouseName = std::nullopt,
                const std::optional<std::string>& aisle = std::nullopt,
                const std::optional<std::string>& bay = std::nullopt,
                const std::optional<std::string>& level = std::nullopt,
                const std::optional<std::string>& name = std::nullopt,
                const std::optional<std::string>& zone = std::nullopt,
                const std::optional<json>& dimensions = std::nullopt,
                const std::optional<json>& maxWeight = std::nullopt,
                const std::optional<double>& maxVolume = std::nullopt);

    // Immutable getters
    std::string getId() const { return id_; }
    std::string getWarehouseId() const { return warehouseId_; }
    std::string getWarehouseCode() const { return warehouseCode_; }
    std::string getCode() const { return code_; }
    std::string getType() const { return type_; }
    std::string getStatus() const { return status_; }
    bool getIsPickable() const { return isPickable_; }
    bool getIsReceivable() const { return isReceivable_; }
    json getAudit() const { return audit_; }
    std::optional<std::string> getWarehouseName() const { return warehouseName_; }
    std::optional<std::string> getAisle() const { return aisle_; }
    std::optional<std::string> getBay() const { return bay_; }
    std::optional<std::string> getLevel() const { return level_; }
    std::optional<std::string> getName() const { return name_; }
    std::optional<std::string> getZone() const { return zone_; }
    std::optional<json> getDimensions() const { return dimensions_; }
    std::optional<json> getMaxWeight() const { return maxWeight_; }
    std::optional<double> getMaxVolume() const { return maxVolume_; }

    // Serialization
    json toJson() const;

private:
    // Required fields
    std::string id_;
    std::string warehouseId_;
    std::string warehouseCode_;
    std::string code_;
    std::string type_;
    std::string status_;
    bool isPickable_;
    bool isReceivable_;
    json audit_;
    
    // Optional fields
    std::optional<std::string> warehouseName_;
    std::optional<std::string> aisle_;
    std::optional<std::string> bay_;
    std::optional<std::string> level_;
    std::optional<std::string> name_;
    std::optional<std::string> zone_;
    std::optional<json> dimensions_;
    std::optional<json> maxWeight_;
    std::optional<double> maxVolume_;

    // Validation
    void validateUuid(const std::string& uuid, const std::string& fieldName) const;
};

} // namespace dtos
} // namespace warehouse
