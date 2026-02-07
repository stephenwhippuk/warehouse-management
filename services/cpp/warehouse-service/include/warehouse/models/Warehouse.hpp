#pragma once

#include "Common.hpp"
#include <string>
#include <vector>
#include <optional>
#include <map>

namespace warehouse::models {

/**
 * @brief Warehouse entity (matching warehouse.schema.json)
 */
enum class WarehouseType {
    Distribution,
    Fulfillment,
    Storage,
    ColdStorage,
    CrossDock
};

enum class WarehouseCapability {
    Refrigeration,
    Freezer,
    Hazmat,
    ClimateControlled,
    HighBay,
    DockDoors,
    RailAccess,
    CrossDocking
};

struct ContactPerson {
    std::string name;
    std::string email;
    std::optional<std::string> phone;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ContactPerson, name, email, phone)
};

struct OperatingHoursDay {
    std::string dayOfWeek; // monday, tuesday, etc.
    std::string openTime;  // HH:MM
    std::string closeTime; // HH:MM

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OperatingHoursDay, dayOfWeek, openTime, closeTime)
};

struct OperatingHours {
    std::string timezone; // IANA timezone
    std::vector<OperatingHoursDay> schedule;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OperatingHours, timezone, schedule)
};

class Warehouse {
public:
    Warehouse() = default;
    
    // Getters
    const std::string& getId() const { return id_; }
    const std::string& getCode() const { return code_; }
    const std::string& getName() const { return name_; }
    const std::optional<std::string>& getDescription() const { return description_; }
    const Address& getAddress() const { return address_; }
    const std::optional<Coordinates>& getCoordinates() const { return coordinates_; }
    WarehouseType getType() const { return type_; }
    const std::optional<double>& getTotalArea() const { return totalArea_; }
    const std::optional<double>& getStorageCapacity() const { return storageCapacity_; }
    const std::optional<ContactPerson>& getContactPerson() const { return contactPerson_; }
    const std::optional<OperatingHours>& getOperatingHours() const { return operatingHours_; }
    const std::vector<WarehouseCapability>& getCapabilities() const { return capabilities_; }
    Status getStatus() const { return status_; }
    const std::optional<std::map<std::string, json>>& getMetadata() const { return metadata_; }
    const AuditInfo& getAudit() const { return audit_; }

    // Setters
    void setId(const std::string& id) { id_ = id; }
    void setCode(const std::string& code) { code_ = code; }
    void setName(const std::string& name) { name_ = name; }
    void setDescription(const std::optional<std::string>& desc) { description_ = desc; }
    void setAddress(const Address& addr) { address_ = addr; }
    void setCoordinates(const std::optional<Coordinates>& coords) { coordinates_ = coords; }
    void setType(WarehouseType type) { type_ = type; }
    void setTotalArea(const std::optional<double>& area) { totalArea_ = area; }
    void setStorageCapacity(const std::optional<double>& capacity) { storageCapacity_ = capacity; }
    void setContactPerson(const std::optional<ContactPerson>& contact) { contactPerson_ = contact; }
    void setOperatingHours(const std::optional<OperatingHours>& hours) { operatingHours_ = hours; }
    void setCapabilities(const std::vector<WarehouseCapability>& caps) { capabilities_ = caps; }
    void setStatus(Status status) { status_ = status; }
    void setMetadata(const std::optional<std::map<std::string, json>>& meta) { metadata_ = meta; }
    void setAudit(const AuditInfo& audit) { audit_ = audit; }

    // JSON serialization
    json toJson() const;
    static Warehouse fromJson(const json& j);

private:
    std::string id_;
    std::string code_;
    std::string name_;
    std::optional<std::string> description_;
    Address address_;
    std::optional<Coordinates> coordinates_;
    WarehouseType type_;
    std::optional<double> totalArea_;
    std::optional<double> storageCapacity_;
    std::optional<ContactPerson> contactPerson_;
    std::optional<OperatingHours> operatingHours_;
    std::vector<WarehouseCapability> capabilities_;
    Status status_;
    std::optional<std::map<std::string, json>> metadata_;
    AuditInfo audit_;
};

// Helper functions
std::string warehouseTypeToString(WarehouseType type);
WarehouseType stringToWarehouseType(const std::string& str);
std::string warehouseCapabilityToString(WarehouseCapability cap);
WarehouseCapability stringToWarehouseCapability(const std::string& str);

} // namespace warehouse::models
