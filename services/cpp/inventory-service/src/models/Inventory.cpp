#include "inventory/models/Inventory.hpp"
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace inventory {
namespace models {

// Helper functions for enum conversion
std::string inventoryStatusToString(InventoryStatus status) {
    switch (status) {
        case InventoryStatus::AVAILABLE: return "available";
        case InventoryStatus::RESERVED: return "reserved";
        case InventoryStatus::ALLOCATED: return "allocated";
        case InventoryStatus::QUARANTINE: return "quarantine";
        case InventoryStatus::DAMAGED: return "damaged";
        case InventoryStatus::EXPIRED: return "expired";
        case InventoryStatus::RECALLED: return "recalled";
        default: throw std::invalid_argument("Unknown inventory status");
    }
}

InventoryStatus inventoryStatusFromString(const std::string& str) {
    if (str == "available") return InventoryStatus::AVAILABLE;
    if (str == "reserved") return InventoryStatus::RESERVED;
    if (str == "allocated") return InventoryStatus::ALLOCATED;
    if (str == "quarantine") return InventoryStatus::QUARANTINE;
    if (str == "damaged") return InventoryStatus::DAMAGED;
    if (str == "expired") return InventoryStatus::EXPIRED;
    if (str == "recalled") return InventoryStatus::RECALLED;
    throw std::invalid_argument("Invalid inventory status string: " + str);
}

std::string qualityStatusToString(QualityStatus status) {
    switch (status) {
        case QualityStatus::PASSED: return "passed";
        case QualityStatus::FAILED: return "failed";
        case QualityStatus::PENDING: return "pending";
        case QualityStatus::NOT_TESTED: return "not_tested";
        default: throw std::invalid_argument("Unknown quality status");
    }
}

QualityStatus qualityStatusFromString(const std::string& str) {
    if (str == "passed") return QualityStatus::PASSED;
    if (str == "failed") return QualityStatus::FAILED;
    if (str == "pending") return QualityStatus::PENDING;
    if (str == "not_tested") return QualityStatus::NOT_TESTED;
    throw std::invalid_argument("Invalid quality status string: " + str);
}

// Constructor
Inventory::Inventory(const std::string& id,
                     const std::string& productId,
                     const std::string& warehouseId,
                     const std::string& locationId,
                     int quantity)
    : id_(id), productId_(productId), warehouseId_(warehouseId), 
      locationId_(locationId), quantity_(quantity), 
      availableQuantity_(quantity), reservedQuantity_(0), allocatedQuantity_(0),
      status_(InventoryStatus::AVAILABLE), qualityStatus_(QualityStatus::NOT_TESTED) {}

// Business methods
void Inventory::reserve(int quantity) {
    if (quantity < 0) {
        throw std::invalid_argument("Cannot reserve negative quantity");
    }
    if (availableQuantity_ < quantity) {
        throw std::runtime_error("Insufficient available quantity to reserve");
    }
    availableQuantity_ -= quantity;
    reservedQuantity_ += quantity;
}

void Inventory::release(int quantity) {
    if (quantity < 0) {
        throw std::invalid_argument("Cannot release negative quantity");
    }
    if (reservedQuantity_ < quantity) {
        throw std::runtime_error("Insufficient reserved quantity to release");
    }
    reservedQuantity_ -= quantity;
    availableQuantity_ += quantity;
}

void Inventory::allocate(int quantity) {
    if (quantity < 0) {
        throw std::invalid_argument("Cannot allocate negative quantity");
    }
    if (reservedQuantity_ < quantity) {
        throw std::runtime_error("Insufficient reserved quantity to allocate");
    }
    reservedQuantity_ -= quantity;
    allocatedQuantity_ += quantity;
}

void Inventory::deallocate(int quantity) {
    if (quantity < 0) {
        throw std::invalid_argument("Cannot deallocate negative quantity");
    }
    if (allocatedQuantity_ < quantity) {
        throw std::runtime_error("Insufficient allocated quantity to deallocate");
    }
    allocatedQuantity_ -= quantity;
    availableQuantity_ += quantity;
}

void Inventory::adjust(int quantityChange, const std::string& reason) {
    quantity_ += quantityChange;
    if (quantity_ < 0) {
        throw std::runtime_error("Quantity adjustment would result in negative inventory");
    }
    availableQuantity_ = quantity_ - reservedQuantity_ - allocatedQuantity_;
    if (availableQuantity_ < 0) {
        throw std::runtime_error("Invalid inventory state after adjustment");
    }
}

bool Inventory::isExpired() const {
    if (!expirationDate_) {
        return false;
    }
    // Simple ISO date comparison (YYYY-MM-DD format)
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::gmtime(&now_time_t);
    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d");
    return expirationDate_.value() < oss.str();
}

bool Inventory::isLowStock(int threshold) const {
    return availableQuantity_ < threshold;
}

// Serialization
json Inventory::toJson() const {
    json j = {
        {"id", id_},
        {"productId", productId_},
        {"warehouseId", warehouseId_},
        {"locationId", locationId_},
        {"quantity", quantity_},
        {"availableQuantity", availableQuantity_},
        {"reservedQuantity", reservedQuantity_},
        {"allocatedQuantity", allocatedQuantity_},
        {"status", inventoryStatusToString(status_)},
        {"qualityStatus", qualityStatusToString(qualityStatus_)}
    };

    if (serialNumber_) j["serialNumber"] = *serialNumber_;
    if (batchNumber_) j["batchNumber"] = *batchNumber_;
    if (expirationDate_) j["expirationDate"] = *expirationDate_;
    if (manufactureDate_) j["manufactureDate"] = *manufactureDate_;
    if (receivedDate_) j["receivedDate"] = *receivedDate_;
    if (lastCountedDate_) j["lastCountedDate"] = *lastCountedDate_;
    if (lastCountedBy_) j["lastCountedBy"] = *lastCountedBy_;
    if (costPerUnit_) j["costPerUnit"] = *costPerUnit_;
    if (notes_) j["notes"] = *notes_;
    if (metadata_) j["metadata"] = *metadata_;

    // Audit info
    if (createdAt_ || updatedAt_ || createdBy_ || updatedBy_) {
        json audit;
        if (createdAt_) audit["createdAt"] = *createdAt_;
        if (updatedAt_) audit["updatedAt"] = *updatedAt_;
        if (createdBy_) audit["createdBy"] = *createdBy_;
        if (updatedBy_) audit["updatedBy"] = *updatedBy_;
        j["audit"] = audit;
    }

    return j;
}

Inventory Inventory::fromJson(const json& j) {
    Inventory inv;
    inv.setId(j.at("id").get<std::string>());
    inv.setProductId(j.at("productId").get<std::string>());
    inv.setWarehouseId(j.at("warehouseId").get<std::string>());
    inv.setLocationId(j.at("locationId").get<std::string>());
    inv.setQuantity(j.at("quantity").get<int>());

    if (j.contains("availableQuantity")) inv.setAvailableQuantity(j["availableQuantity"].get<int>());
    if (j.contains("reservedQuantity")) inv.setReservedQuantity(j["reservedQuantity"].get<int>());
    if (j.contains("allocatedQuantity")) inv.setAllocatedQuantity(j["allocatedQuantity"].get<int>());
    if (j.contains("serialNumber")) inv.setSerialNumber(j["serialNumber"].get<std::string>());
    if (j.contains("batchNumber")) inv.setBatchNumber(j["batchNumber"].get<std::string>());
    if (j.contains("expirationDate")) inv.setExpirationDate(j["expirationDate"].get<std::string>());
    if (j.contains("manufactureDate")) inv.setManufactureDate(j["manufactureDate"].get<std::string>());
    if (j.contains("receivedDate")) inv.setReceivedDate(j["receivedDate"].get<std::string>());
    if (j.contains("lastCountedDate")) inv.setLastCountedDate(j["lastCountedDate"].get<std::string>());
    if (j.contains("lastCountedBy")) inv.setLastCountedBy(j["lastCountedBy"].get<std::string>());
    if (j.contains("costPerUnit")) inv.setCostPerUnit(j["costPerUnit"].get<double>());
    if (j.contains("notes")) inv.setNotes(j["notes"].get<std::string>());
    if (j.contains("metadata")) {
        inv.setMetadata(std::optional<json>{j["metadata"]});
    }
    
    if (j.contains("status")) {
        inv.setStatus(inventoryStatusFromString(j["status"].get<std::string>()));
    }
    if (j.contains("qualityStatus")) {
        inv.setQualityStatus(qualityStatusFromString(j["qualityStatus"].get<std::string>()));
    }

    // Audit info
    if (j.contains("audit")) {
        const auto& audit = j["audit"];
        if (audit.contains("createdAt")) inv.setCreatedAt(audit["createdAt"].get<std::string>());
        if (audit.contains("updatedAt")) inv.setUpdatedAt(audit["updatedAt"].get<std::string>());
        if (audit.contains("createdBy")) inv.setCreatedBy(audit["createdBy"].get<std::string>());
        if (audit.contains("updatedBy")) inv.setUpdatedBy(audit["updatedBy"].get<std::string>());
    }

    return inv;
}

} // namespace models
} // namespace inventory
