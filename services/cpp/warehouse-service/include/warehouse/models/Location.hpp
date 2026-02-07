#pragma once

#include "Common.hpp"
#include <string>
#include <optional>
#include <map>

namespace warehouse::models {

/**
 * @brief Location entity (matching location.schema.json)
 */
enum class LocationType {
    Bin,
    Shelf,
    Rack,
    Pallet,
    Floor,
    Staging,
    Receiving,
    Shipping,
    Picking,
    Returns
};

enum class LocationStatus {
    Active,
    Inactive,
    Full,
    Reserved,
    Damaged,
    Maintenance
};

enum class RequiredEquipment {
    None,
    Forklift,
    Ladder,
    CherryPicker,
    PalletJack
};

struct TemperatureRange {
    double min; // Celsius
    double max; // Celsius

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TemperatureRange, min, max)
};

class Location {
public:
    Location() = default;

    // Getters
    const std::string& getId() const { return id_; }
    const std::string& getWarehouseId() const { return warehouseId_; }
    const std::string& getCode() const { return code_; }
    const std::optional<std::string>& getName() const { return name_; }
    LocationType getType() const { return type_; }
    const std::optional<std::string>& getZone() const { return zone_; }
    const std::optional<std::string>& getAisle() const { return aisle_; }
    const std::optional<std::string>& getRack() const { return rack_; }
    const std::optional<std::string>& getShelf() const { return shelf_; }
    const std::optional<std::string>& getBin() const { return bin_; }
    const std::optional<std::string>& getParentLocationId() const { return parentLocationId_; }
    const std::optional<Dimensions>& getDimensions() const { return dimensions_; }
    const std::optional<Weight>& getMaxWeight() const { return maxWeight_; }
    const std::optional<double>& getMaxVolume() const { return maxVolume_; }
    bool isPickable() const { return isPickable_; }
    bool isReceivable() const { return isReceivable_; }
    RequiredEquipment getRequiresEquipment() const { return requiresEquipment_; }
    bool isTemperatureControlled() const { return temperatureControlled_; }
    const std::optional<TemperatureRange>& getTemperatureRange() const { return temperatureRange_; }
    const std::optional<std::string>& getBarcode() const { return barcode_; }
    LocationStatus getStatus() const { return status_; }
    const std::optional<std::map<std::string, json>>& getMetadata() const { return metadata_; }
    const AuditInfo& getAudit() const { return audit_; }

    // Setters
    void setId(const std::string& id) { id_ = id; }
    void setWarehouseId(const std::string& warehouseId) { warehouseId_ = warehouseId; }
    void setCode(const std::string& code) { code_ = code; }
    void setName(const std::optional<std::string>& name) { name_ = name; }
    void setType(LocationType type) { type_ = type; }
    void setZone(const std::optional<std::string>& zone) { zone_ = zone; }
    void setAisle(const std::optional<std::string>& aisle) { aisle_ = aisle; }
    void setRack(const std::optional<std::string>& rack) { rack_ = rack; }
    void setShelf(const std::optional<std::string>& shelf) { shelf_ = shelf; }
    void setBin(const std::optional<std::string>& bin) { bin_ = bin; }
    void setParentLocationId(const std::optional<std::string>& parentId) { parentLocationId_ = parentId; }
    void setDimensions(const std::optional<Dimensions>& dims) { dimensions_ = dims; }
    void setMaxWeight(const std::optional<Weight>& weight) { maxWeight_ = weight; }
    void setMaxVolume(const std::optional<double>& volume) { maxVolume_ = volume; }
    void setIsPickable(bool pickable) { isPickable_ = pickable; }
    void setIsReceivable(bool receivable) { isReceivable_ = receivable; }
    void setRequiresEquipment(RequiredEquipment equipment) { requiresEquipment_ = equipment; }
    void setTemperatureControlled(bool controlled) { temperatureControlled_ = controlled; }
    void setTemperatureRange(const std::optional<TemperatureRange>& range) { temperatureRange_ = range; }
    void setBarcode(const std::optional<std::string>& barcode) { barcode_ = barcode; }
    void setStatus(LocationStatus status) { status_ = status; }
    void setMetadata(const std::optional<std::map<std::string, json>>& meta) { metadata_ = meta; }
    void setAudit(const AuditInfo& audit) { audit_ = audit; }

    // JSON serialization
    json toJson() const;
    static Location fromJson(const json& j);

private:
    std::string id_;
    std::string warehouseId_;
    std::string code_;
    std::optional<std::string> name_;
    LocationType type_;
    std::optional<std::string> zone_;
    std::optional<std::string> aisle_;
    std::optional<std::string> rack_;
    std::optional<std::string> shelf_;
    std::optional<std::string> bin_;
    std::optional<std::string> parentLocationId_;
    std::optional<Dimensions> dimensions_;
    std::optional<Weight> maxWeight_;
    std::optional<double> maxVolume_;
    bool isPickable_ = true;
    bool isReceivable_ = true;
    RequiredEquipment requiresEquipment_ = RequiredEquipment::None;
    bool temperatureControlled_ = false;
    std::optional<TemperatureRange> temperatureRange_;
    std::optional<std::string> barcode_;
    LocationStatus status_;
    std::optional<std::map<std::string, json>> metadata_;
    AuditInfo audit_;
};

// Helper functions
std::string locationTypeToString(LocationType type);
LocationType stringToLocationType(const std::string& str);
std::string locationStatusToString(LocationStatus status);
LocationStatus stringToLocationStatus(const std::string& str);
std::string requiredEquipmentToString(RequiredEquipment equipment);
RequiredEquipment stringToRequiredEquipment(const std::string& str);

} // namespace warehouse::models
