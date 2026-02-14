#pragma once

#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

namespace warehouse {
namespace dtos {

using json = nlohmann::json;

/**
 * @brief Warehouse data transfer object
 * 
 * Conforms to WarehouseDto contract v1.0
 */
class WarehouseDto {
public:
    /**
     * @brief Construct warehouse DTO with all required fields
     * @param id Warehouse identifier (UUID)
     * @param code Warehouse code
     * @param name Warehouse name
     * @param status Warehouse status (active, inactive, maintenance)
     * @param address Physical address (JSON object)
     * @param type Warehouse type
     * @param createdAt Created timestamp (ISO 8601)
     * @param updatedAt Last updated timestamp (ISO 8601)
     * @param description Optional warehouse description
     * @param coordinates Optional GPS coordinates (JSON object)
     * @param totalArea Optional total area in square meters
     * @param storageCapacity Optional storage capacity in cubic meters
     * @param contactPerson Optional contact person details (JSON object)
     * @param operatingHours Optional operating hours schedule (JSON object)
     * @param capabilities Optional warehouse capabilities (array of strings)
     * @param zones Optional number of zones
     * @param dockDoors Optional number of dock doors
     * @param isActive Optional active status boolean
     */
    WarehouseDto(const std::string& id,
                 const std::string& code,
                 const std::string& name,
                 const std::string& status,
                 const json& address,
                 const std::string& type,
                 const std::string& createdAt,
                 const std::string& updatedAt,
                 const std::optional<std::string>& description = std::nullopt,
                 const std::optional<json>& coordinates = std::nullopt,
                 const std::optional<double>& totalArea = std::nullopt,
                 const std::optional<double>& storageCapacity = std::nullopt,
                 const std::optional<json>& contactPerson = std::nullopt,
                 const std::optional<json>& operatingHours = std::nullopt,
                 const std::optional<std::vector<std::string>>& capabilities = std::nullopt,
                 const std::optional<int>& zones = std::nullopt,
                 const std::optional<int>& dockDoors = std::nullopt,
                 const std::optional<bool>& isActive = std::nullopt);

    // Immutable getters
    std::string getId() const { return id_; }
    std::string getCode() const { return code_; }
    std::string getName() const { return name_; }
    std::string getStatus() const { return status_; }
    json getAddress() const { return address_; }
    std::string getType() const { return type_; }
    std::string getCreatedAt() const { return createdAt_; }
    std::string getUpdatedAt() const { return updatedAt_; }
    std::optional<std::string> getDescription() const { return description_; }
    std::optional<json> getCoordinates() const { return coordinates_; }
    std::optional<double> getTotalArea() const { return totalArea_; }
    std::optional<double> getStorageCapacity() const { return storageCapacity_; }
    std::optional<json> getContactPerson() const { return contactPerson_; }
    std::optional<json> getOperatingHours() const { return operatingHours_; }
    std::optional<std::vector<std::string>> getCapabilities() const { return capabilities_; }
    std::optional<int> getZones() const { return zones_; }
    std::optional<int> getDockDoors() const { return dockDoors_; }
    std::optional<bool> getIsActive() const { return isActive_; }

    // Serialization
    json toJson() const;

private:
    // Required fields
    std::string id_;
    std::string code_;
    std::string name_;
    std::string status_;
    json address_;
    std::string type_;
    std::string createdAt_;
    std::string updatedAt_;
    
    // Optional fields
    std::optional<std::string> description_;
    std::optional<json> coordinates_;
    std::optional<double> totalArea_;
    std::optional<double> storageCapacity_;
    std::optional<json> contactPerson_;
    std::optional<json> operatingHours_;
    std::optional<std::vector<std::string>> capabilities_;
    std::optional<int> zones_;
    std::optional<int> dockDoors_;
    std::optional<bool> isActive_;

    // Validation
    void validateUuid(const std::string& uuid, const std::string& fieldName) const;
    void validateDateTime(const std::string& dateTime, const std::string& fieldName) const;
    void validateStatus(const std::string& status) const;
};

} // namespace dtos
} // namespace warehouse
