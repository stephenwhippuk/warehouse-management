#include "warehouse/services/WarehouseService.hpp"
#include "warehouse/repositories/WarehouseRepository.hpp"
#include "warehouse/utils/Logger.hpp"
#include <regex>

namespace warehouse::services {

WarehouseService::WarehouseService(std::shared_ptr<repositories::WarehouseRepository> repo)
    : repo_(repo) {
}

std::optional<models::Warehouse> WarehouseService::getById(const std::string& id) {
    return repo_->findById(id);
}

std::optional<models::Warehouse> WarehouseService::getByCode(const std::string& code) {
    return repo_->findByCode(code);
}

std::vector<models::Warehouse> WarehouseService::getAll() {
    return repo_->findAll();
}

std::vector<models::Warehouse> WarehouseService::getActiveWarehouses() {
    return repo_->findByStatus(models::Status::Active);
}

std::string WarehouseService::createWarehouse(const models::Warehouse& warehouse) {
    std::string errorMessage;
    if (!isValidWarehouse(warehouse, errorMessage)) {
        utils::Logger::warn("Invalid warehouse: {}", errorMessage);
        throw std::invalid_argument(errorMessage);
    }
    
    if (repo_->codeExists(warehouse.getCode())) {
        throw std::invalid_argument("Warehouse code already exists");
    }
    
    return repo_->create(warehouse);
}

bool WarehouseService::updateWarehouse(const models::Warehouse& warehouse) {
    std::string errorMessage;
    if (!isValidWarehouse(warehouse, errorMessage)) {
        utils::Logger::warn("Invalid warehouse update: {}", errorMessage);
        return false;
    }
    
    return repo_->update(warehouse);
}

bool WarehouseService::deleteWarehouse(const std::string& id) {
    return repo_->deleteById(id);
}

bool WarehouseService::activateWarehouse(const std::string& id) {
    // TODO: Implement status change to active
    utils::Logger::info("WarehouseService::activateWarehouse({})", id);
    return false;
}

bool WarehouseService::deactivateWarehouse(const std::string& id) {
    // TODO: Implement status change to inactive
    utils::Logger::info("WarehouseService::deactivateWarehouse({})", id);
    return false;
}

bool WarehouseService::isValidWarehouse(const models::Warehouse& warehouse, std::string& errorMessage) {
    if (warehouse.getCode().empty()) {
        errorMessage = "Warehouse code is required";
        return false;
    }
    
    if (!validateCode(warehouse.getCode())) {
        errorMessage = "Invalid warehouse code format";
        return false;
    }
    
    if (warehouse.getName().empty()) {
        errorMessage = "Warehouse name is required";
        return false;
    }
    
    if (!validateAddress(warehouse.getAddress())) {
        errorMessage = "Invalid warehouse address";
        return false;
    }
    
    return true;
}

bool WarehouseService::validateCode(const std::string& code) {
    // Code should match pattern: ^[A-Z0-9-]+$
    std::regex pattern("^[A-Z0-9-]+$");
    return std::regex_match(code, pattern);
}

bool WarehouseService::validateAddress(const models::Address& address) {
    return !address.street.empty() &&
           !address.city.empty() &&
           !address.postalCode.empty() &&
           address.country.length() == 2; // ISO 3166-1 alpha-2
}

} // namespace warehouse::services
