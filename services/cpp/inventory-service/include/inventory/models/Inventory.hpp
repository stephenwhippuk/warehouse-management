#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace inventory {
namespace models {

using json = nlohmann::json;

enum class InventoryStatus {
    AVAILABLE,
    RESERVED,
    ALLOCATED,
    QUARANTINE,
    DAMAGED,
    EXPIRED,
    RECALLED
};

enum class QualityStatus {
    PASSED,
    FAILED,
    PENDING,
    NOT_TESTED
};

// Forward declarations for helper functions
std::string inventoryStatusToString(InventoryStatus status);
InventoryStatus inventoryStatusFromString(const std::string& str);
std::string qualityStatusToString(QualityStatus status);
QualityStatus qualityStatusFromString(const std::string& str);

class Inventory {
public:
    // Constructors
    Inventory() = default;
    Inventory(const std::string& id, 
              const std::string& productId,
              const std::string& warehouseId,
              const std::string& locationId,
              int quantity);

    // Getters
    std::string getId() const { return id_; }
    std::string getProductId() const { return productId_; }
    std::string getWarehouseId() const { return warehouseId_; }
    std::string getLocationId() const { return locationId_; }
    int getQuantity() const { return quantity_; }
    int getAvailableQuantity() const { return availableQuantity_; }
    int getReservedQuantity() const { return reservedQuantity_; }
    int getAllocatedQuantity() const { return allocatedQuantity_; }
    std::optional<std::string> getSerialNumber() const { return serialNumber_; }
    std::optional<std::string> getBatchNumber() const { return batchNumber_; }
    std::optional<std::string> getExpirationDate() const { return expirationDate_; }
    std::optional<std::string> getManufactureDate() const { return manufactureDate_; }
    std::optional<std::string> getReceivedDate() const { return receivedDate_; }
    std::optional<std::string> getLastCountedDate() const { return lastCountedDate_; }
    std::optional<std::string> getLastCountedBy() const { return lastCountedBy_; }
    std::optional<double> getCostPerUnit() const { return costPerUnit_; }
    InventoryStatus getStatus() const { return status_; }
    QualityStatus getQualityStatus() const { return qualityStatus_; }
    std::optional<std::string> getNotes() const { return notes_; }
    std::optional<json> getMetadata() const { return metadata_; }
    std::optional<std::string> getCreatedAt() const { return createdAt_; }
    std::optional<std::string> getUpdatedAt() const { return updatedAt_; }
    std::optional<std::string> getCreatedBy() const { return createdBy_; }
    std::optional<std::string> getUpdatedBy() const { return updatedBy_; }

    // Setters
    void setId(const std::string& id) { id_ = id; }
    void setProductId(const std::string& productId) { productId_ = productId; }
    void setWarehouseId(const std::string& warehouseId) { warehouseId_ = warehouseId; }
    void setLocationId(const std::string& locationId) { locationId_ = locationId; }
    void setQuantity(int quantity) { quantity_ = quantity; }
    void setAvailableQuantity(int availableQuantity) { availableQuantity_ = availableQuantity; }
    void setReservedQuantity(int reservedQuantity) { reservedQuantity_ = reservedQuantity; }
    void setAllocatedQuantity(int allocatedQuantity) { allocatedQuantity_ = allocatedQuantity; }
    void setSerialNumber(const std::optional<std::string>& serialNumber) { serialNumber_ = serialNumber; }
    void setBatchNumber(const std::optional<std::string>& batchNumber) { batchNumber_ = batchNumber; }
    void setExpirationDate(const std::optional<std::string>& expirationDate) { expirationDate_ = expirationDate; }
    void setManufactureDate(const std::optional<std::string>& manufactureDate) { manufactureDate_ = manufactureDate; }
    void setReceivedDate(const std::optional<std::string>& receivedDate) { receivedDate_ = receivedDate; }
    void setLastCountedDate(const std::optional<std::string>& lastCountedDate) { lastCountedDate_ = lastCountedDate; }
    void setLastCountedBy(const std::optional<std::string>& lastCountedBy) { lastCountedBy_ = lastCountedBy; }
    void setCostPerUnit(const std::optional<double>& costPerUnit) { costPerUnit_ = costPerUnit; }
    void setStatus(InventoryStatus status) { status_ = status; }
    void setQualityStatus(QualityStatus qualityStatus) { qualityStatus_ = qualityStatus; }
    void setNotes(const std::optional<std::string>& notes) { notes_ = notes; }
    void setMetadata(const std::optional<json>& metadata) { metadata_ = metadata; }
    void setCreatedAt(const std::optional<std::string>& createdAt) { createdAt_ = createdAt; }
    void setUpdatedAt(const std::optional<std::string>& updatedAt) { updatedAt_ = updatedAt; }
    void setCreatedBy(const std::optional<std::string>& createdBy) { createdBy_ = createdBy; }
    void setUpdatedBy(const std::optional<std::string>& updatedBy) { updatedBy_ = updatedBy; }

    // Business methods
    void reserve(int quantity);
    void release(int quantity);
    void allocate(int quantity);
    void deallocate(int quantity);
    void adjust(int quantityChange, const std::string& reason);
    bool isExpired() const;
    bool isLowStock(int threshold) const;

    // Serialization
    json toJson() const;
    static Inventory fromJson(const json& j);

private:
    std::string id_;
    std::string productId_;
    std::string warehouseId_;
    std::string locationId_;
    int quantity_ = 0;
    int availableQuantity_ = 0;
    int reservedQuantity_ = 0;
    int allocatedQuantity_ = 0;
    std::optional<std::string> serialNumber_;
    std::optional<std::string> batchNumber_;
    std::optional<std::string> expirationDate_;
    std::optional<std::string> manufactureDate_;
    std::optional<std::string> receivedDate_;
    std::optional<std::string> lastCountedDate_;
    std::optional<std::string> lastCountedBy_;
    std::optional<double> costPerUnit_;
    InventoryStatus status_ = InventoryStatus::AVAILABLE;
    QualityStatus qualityStatus_ = QualityStatus::NOT_TESTED;
    std::optional<std::string> notes_;
    std::optional<json> metadata_;
    
    // Audit fields
    std::optional<std::string> createdAt_;
    std::optional<std::string> updatedAt_;
    std::optional<std::string> createdBy_;
    std::optional<std::string> updatedBy_;
};

} // namespace models
} // namespace inventory
