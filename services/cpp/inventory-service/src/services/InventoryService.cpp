#include "inventory/services/InventoryService.hpp"
#include "inventory/utils/Logger.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace inventory {
namespace services {

InventoryService::InventoryService(std::shared_ptr<repositories::InventoryRepository> repository,
                                   std::shared_ptr<utils::MessageBus> messageBus)
    : repository_(repository), messageBus_(std::move(messageBus)) {}

std::optional<models::Inventory> InventoryService::getById(const std::string& id) {
    return repository_->findById(id);
}

std::vector<models::Inventory> InventoryService::getAll() {
    return repository_->findAll();
}

std::vector<models::Inventory> InventoryService::getByProductId(const std::string& productId) {
    return repository_->findByProductId(productId);
}

std::vector<models::Inventory> InventoryService::getByWarehouseId(const std::string& warehouseId) {
    return repository_->findByWarehouseId(warehouseId);
}

std::vector<models::Inventory> InventoryService::getByLocationId(const std::string& locationId) {
    return repository_->findByLocationId(locationId);
}

std::vector<models::Inventory> InventoryService::getLowStock(int threshold) {
    if (threshold < 0) {
        throw std::invalid_argument("Threshold must be non-negative");
    }
    return repository_->findLowStock(threshold);
}

std::vector<models::Inventory> InventoryService::getExpired() {
    return repository_->findExpired();
}

models::Inventory InventoryService::create(const models::Inventory& inventory) {
    if (!isValidInventory(inventory)) {
        throw std::invalid_argument("Invalid inventory data");
    }
    auto created = repository_->create(inventory);

    if (messageBus_) {
        try {
            messageBus_->publish("created", created.toJson());
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.created event: {}", ex.what());
        }
    }

    return created;
}

models::Inventory InventoryService::update(const models::Inventory& inventory) {
    if (!isValidInventory(inventory)) {
        throw std::invalid_argument("Invalid inventory data");
    }
    
    auto existing = repository_->findById(inventory.getId());
    if (!existing) {
        throw std::runtime_error("Inventory not found: " + inventory.getId());
    }

    auto updated = repository_->update(inventory);

    if (messageBus_) {
        try {
            messageBus_->publish("updated", updated.toJson());
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.updated event: {}", ex.what());
        }
    }

    return updated;
}

bool InventoryService::remove(const std::string& id) {
    auto existing = repository_->findById(id);
    if (!existing) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    // Check if there are any reserved or allocated quantities
    if (existing->getReservedQuantity() > 0 || existing->getAllocatedQuantity() > 0) {
        throw std::runtime_error("Cannot delete inventory with reserved or allocated quantities");
    }

    bool deleted = repository_->deleteById(id);

    if (deleted && messageBus_) {
        try {
            nlohmann::json payload = {
                {"id", id},
                {"event", "deleted"}
            };
            messageBus_->publish("deleted", payload);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.deleted event: {}", ex.what());
        }
    }

    return deleted;
}

void InventoryService::reserve(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->reserve(quantity);
    auto updated = repository_->update(*inventory);

    if (messageBus_) {
        try {
            nlohmann::json payload = updated.toJson();
            payload["action"] = "reserve";
            payload["quantity"] = quantity;
            messageBus_->publish("reserved", payload);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.reserved event: {}", ex.what());
        }
    }
}

void InventoryService::release(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->release(quantity);
    auto updated = repository_->update(*inventory);

    if (messageBus_) {
        try {
            nlohmann::json payload = updated.toJson();
            payload["action"] = "release";
            payload["quantity"] = quantity;
            messageBus_->publish("released", payload);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.released event: {}", ex.what());
        }
    }
}

void InventoryService::allocate(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->allocate(quantity);
    auto updated = repository_->update(*inventory);

    if (messageBus_) {
        try {
            nlohmann::json payload = updated.toJson();
            payload["action"] = "allocate";
            payload["quantity"] = quantity;
            messageBus_->publish("allocated", payload);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.allocated event: {}", ex.what());
        }
    }
}

void InventoryService::deallocate(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->deallocate(quantity);
    auto updated = repository_->update(*inventory);

    if (messageBus_) {
        try {
            nlohmann::json payload = updated.toJson();
            payload["action"] = "deallocate";
            payload["quantity"] = quantity;
            messageBus_->publish("deallocated", payload);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.deallocated event: {}", ex.what());
        }
    }
}

void InventoryService::adjust(const std::string& id, int quantityChange, const std::string& reason) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    if (reason.empty()) {
        throw std::invalid_argument("Adjustment reason is required");
    }
    
    inventory->adjust(quantityChange, reason);
    auto updated = repository_->update(*inventory);

    if (messageBus_) {
        try {
            nlohmann::json payload = updated.toJson();
            payload["action"] = "adjust";
            payload["quantityChange"] = quantityChange;
            payload["reason"] = reason;
            messageBus_->publish("adjusted", payload);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.adjusted event: {}", ex.what());
        }
    }
}

bool InventoryService::isValidInventory(const models::Inventory& inventory) const {
    // Validate required fields
    if (inventory.getId().empty()) return false;
    if (inventory.getProductId().empty()) return false;
    if (inventory.getWarehouseId().empty()) return false;
    if (inventory.getLocationId().empty()) return false;
    if (inventory.getQuantity() < 0) return false;
    
    // Validate quantity relationships
    try {
        validateQuantities(
            inventory.getQuantity(),
            inventory.getAvailableQuantity(),
            inventory.getReservedQuantity(),
            inventory.getAllocatedQuantity()
        );
    } catch (const std::exception&) {
        return false;
    }
    
    return true;
}

int InventoryService::getTotalQuantityForProduct(const std::string& productId) {
    return repository_->getTotalQuantityByProduct(productId);
}

int InventoryService::getAvailableQuantityForProduct(const std::string& productId) {
    return repository_->getAvailableQuantityByProduct(productId);
}

void InventoryService::validateQuantities(int quantity, int available, int reserved, int allocated) const {
    if (quantity < 0) {
        throw std::invalid_argument("Quantity cannot be negative");
    }
    if (available < 0 || reserved < 0 || allocated < 0) {
        throw std::invalid_argument("Quantities cannot be negative");
    }
    if (quantity != available + reserved + allocated) {
        throw std::invalid_argument("Quantity must equal available + reserved + allocated");
    }
}

} // namespace services
} // namespace inventory
