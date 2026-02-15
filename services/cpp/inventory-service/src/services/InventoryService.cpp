#include "inventory/services/InventoryService.hpp"
#include "inventory/utils/Logger.hpp"
#include "inventory/utils/DtoMapper.hpp"
#include <warehouse/messaging/Event.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace inventory {
namespace services {

InventoryService::InventoryService(std::shared_ptr<repositories::InventoryRepository> repository,
                                   std::shared_ptr<warehouse::messaging::EventPublisher> eventPublisher)
    : repository_(repository), eventPublisher_(std::move(eventPublisher)) {}

std::optional<dtos::InventoryItemDto> InventoryService::getById(const std::string& id) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        return std::nullopt;
    }
    return convertToDto(*inventory);
}

std::vector<dtos::InventoryItemDto> InventoryService::getAll() {
    return convertToDtos(repository_->findAll());
}

std::vector<dtos::InventoryItemDto> InventoryService::getByProductId(const std::string& productId) {
    return convertToDtos(repository_->findByProductId(productId));
}

std::vector<dtos::InventoryItemDto> InventoryService::getByWarehouseId(const std::string& warehouseId) {
    return convertToDtos(repository_->findByWarehouseId(warehouseId));
}

std::vector<dtos::InventoryItemDto> InventoryService::getByLocationId(const std::string& locationId) {
    return convertToDtos(repository_->findByLocationId(locationId));
}

std::vector<dtos::InventoryItemDto> InventoryService::getLowStock(int threshold) {
    if (threshold < 0) {
        throw std::invalid_argument("Threshold must be non-negative");
    }
    return convertToDtos(repository_->findLowStock(threshold));
}

std::vector<dtos::InventoryItemDto> InventoryService::getExpired() {
    return convertToDtos(repository_->findExpired());
}

dtos::InventoryItemDto InventoryService::create(const models::Inventory& inventory) {
    if (!isValidInventory(inventory)) {
        throw std::invalid_argument("Invalid inventory data");
    }
    auto created = repository_->create(inventory);

    if (eventPublisher_) {
        try {
            warehouse::messaging::Event event("inventory.created", created.toJson(), "inventory-service");
            eventPublisher_->publish(event);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.created event: {}", ex.what());
        }
    }

    return convertToDto(created);
}

dtos::InventoryItemDto InventoryService::update(const models::Inventory& inventory) {
    if (!isValidInventory(inventory)) {
        throw std::invalid_argument("Invalid inventory data");
    }
    
    auto existing = repository_->findById(inventory.getId());
    if (!existing) {
        throw std::runtime_error("Inventory not found: " + inventory.getId());
    }

    auto updated = repository_->update(inventory);

    if (eventPublisher_) {
        try {
            warehouse::messaging::Event event("inventory.updated", updated.toJson(), "inventory-service");
            eventPublisher_->publish(event);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.updated event: {}", ex.what());
        }
    }

    return convertToDto(updated);
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

    if (deleted && eventPublisher_) {
        try {
            nlohmann::json payload = {
                {"id", id},
                {"event", "deleted"}
            };
            warehouse::messaging::Event event("inventory.deleted", payload, "inventory-service");
            eventPublisher_->publish(event);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.deleted event: {}", ex.what());
        }
    }

    return deleted;
}

dtos::InventoryOperationResultDto InventoryService::reserve(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->reserve(quantity);
    auto updated = repository_->update(*inventory);

    if (eventPublisher_) {
        try {
            nlohmann::json payload = updated.toJson();
            payload["action"] = "reserve";
            payload["quantity"] = quantity;
            warehouse::messaging::Event event("inventory.reserved", payload, "inventory-service");
            eventPublisher_->publish(event);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.reserved event: {}", ex.what());
        }
    }
    
    return utils::DtoMapper::toInventoryOperationResultDto(
        updated, "reserve", quantity, true, std::nullopt
    );
}

dtos::InventoryOperationResultDto InventoryService::release(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->release(quantity);
    auto updated = repository_->update(*inventory);

    if (eventPublisher_) {
        try {
            nlohmann::json payload = updated.toJson();
            payload["action"] = "release";
            payload["quantity"] = quantity;
            warehouse::messaging::Event event("inventory.released", payload, "inventory-service");
            eventPublisher_->publish(event);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.released event: {}", ex.what());
        }
    }
    
    return utils::DtoMapper::toInventoryOperationResultDto(
        updated, "release", quantity, true, std::nullopt
    );
}

dtos::InventoryOperationResultDto InventoryService::allocate(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->allocate(quantity);
    auto updated = repository_->update(*inventory);

    if (eventPublisher_) {
        try {
            nlohmann::json payload = updated.toJson();
            payload["action"] = "allocate";
            payload["quantity"] = quantity;
            warehouse::messaging::Event event("inventory.allocated", payload, "inventory-service");
            eventPublisher_->publish(event);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.allocated event: {}", ex.what());
        }
    }
    
    return utils::DtoMapper::toInventoryOperationResultDto(
        updated, "allocate", quantity, true, std::nullopt
    );
}

dtos::InventoryOperationResultDto InventoryService::deallocate(const std::string& id, int quantity) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    inventory->deallocate(quantity);
    auto updated = repository_->update(*inventory);

    if (eventPublisher_) {
        try {
            nlohmann::json payload = updated.toJson();
            payload["action"] = "deallocate";
            payload["quantity"] = quantity;
            warehouse::messaging::Event event("inventory.deallocated", payload, "inventory-service");
            eventPublisher_->publish(event);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.deallocated event: {}", ex.what());
        }
    }
    
    return utils::DtoMapper::toInventoryOperationResultDto(
        updated, "deallocate", quantity, true, std::nullopt
    );
}

dtos::InventoryOperationResultDto InventoryService::adjust(const std::string& id, int quantityChange, const std::string& reason) {
    auto inventory = repository_->findById(id);
    if (!inventory) {
        throw std::runtime_error("Inventory not found: " + id);
    }
    
    if (reason.empty()) {
        throw std::invalid_argument("Adjustment reason is required");
    }
    
    inventory->adjust(quantityChange, reason);
    auto updated = repository_->update(*inventory);

    if (eventPublisher_) {
        try {
            nlohmann::json payload = updated.toJson();
            payload["action"] = "adjust";
            payload["quantityChange"] = quantityChange;
            payload["reason"] = reason;
            warehouse::messaging::Event event("inventory.adjusted", payload, "inventory-service");
            eventPublisher_->publish(event);
        } catch (const std::exception& ex) {
            utils::Logger::warn("Failed to publish inventory.adjusted event: {}", ex.what());
        }
    }
    
    return utils::DtoMapper::toInventoryOperationResultDto(
        updated, "adjust", quantityChange, true, reason
    );
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

dtos::InventoryItemDto InventoryService::convertToDto(const models::Inventory& inventory) const {
    // TODO: Fetch identity fields from Product, Warehouse, Location services
    // For now, using placeholder values
    return utils::DtoMapper::toInventoryItemDto(
        inventory,
        "SKU-" + inventory.getProductId().substr(0, 8),  // Placeholder SKU
        "WH-" + inventory.getWarehouseId().substr(0, 8), // Placeholder warehouse code
        "LOC-" + inventory.getLocationId().substr(0, 8)  // Placeholder location code
    );
}

std::vector<dtos::InventoryItemDto> InventoryService::convertToDtos(
    const std::vector<models::Inventory>& inventories) const {
    
    // TODO: Batch fetch identity fields from services to improve performance
    std::vector<dtos::InventoryItemDto> dtos;
    dtos.reserve(inventories.size());
    
    for (const auto& inventory : inventories) {
        dtos.push_back(convertToDto(inventory));
    }
    
    return dtos;
}

} // namespace services
} // namespace inventory
