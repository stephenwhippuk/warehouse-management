#include "inventory/services/InventoryService.hpp"
#include <stdexcept>

namespace inventory {
namespace services {

InventoryService::InventoryService(std::shared_ptr<repositories::InventoryRepository> repository)
    : repository_(repository) {}

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
    return repository_->create(inventory);
}

models::Inventory InventoryService::update(const models::Inventory& inventory) {
    if (!isValidInventory(inventory)) {
        throw std::invalid_argument("Invalid inventory data");
    }
    
    auto existing = repository_->findById(inventory.getId());
    if (!existing) {
        throw std::runtime_error("Inventory not found: " + inventory.getId());
    }
    
    return repository_->update(inventory);
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
    
    return repository_->deleteById(id);
}

void InventoryService::reserve(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->reserve(quantity);
    repository_->update(*inventory);
}

void InventoryService::release(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->release(quantity);
    repository_->update(*inventory);
}

void InventoryService::allocate(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->allocate(quantity);
    repository_->update(*inventory);
}

void InventoryService::deallocate(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->deallocate(quantity);
    repository_->update(*inventory);
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
    repository_->update(*inventory);
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
