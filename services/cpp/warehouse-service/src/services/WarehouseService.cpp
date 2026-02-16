#include "warehouse/services/WarehouseService.hpp"
#include "warehouse/repositories/WarehouseRepository.hpp"
#include "warehouse/utils/Logger.hpp"
#include "warehouse/utils/DtoMapper.hpp"
#include <regex>

namespace warehouse::services {

WarehouseService::WarehouseService(http::IServiceProvider& provider)
    : repo_(provider.getService<repositories::WarehouseRepository>()) {
}

std::optional<dtos::WarehouseDto> WarehouseService::getById(const std::string& id) {
    auto warehouse = repo_->findById(id);
    if (!warehouse) {
        return std::nullopt;
    }
    return utils::DtoMapper::toWarehouseDto(*warehouse);
}

std::optional<dtos::WarehouseDto> WarehouseService::getByCode(const std::string& code) {
    auto warehouse = repo_->findByCode(code);
    if (!warehouse) {
        return std::nullopt;
    }
    return utils::DtoMapper::toWarehouseDto(*warehouse);
}

std::vector<dtos::WarehouseDto> WarehouseService::getAll() {
    return convertToDtos(repo_->findAll());
}

std::vector<dtos::WarehouseDto> WarehouseService::getActiveWarehouses() {
    return convertToDtos(repo_->findByStatus(models::Status::Active));
}

dtos::WarehouseDto WarehouseService::createWarehouse(const models::Warehouse& warehouse) {
    std::string errorMessage;
    if (!isValidWarehouse(warehouse, errorMessage)) {
        utils::Logger::warn("Invalid warehouse: {}", errorMessage);
        throw std::invalid_argument(errorMessage);
    }
    
    if (repo_->codeExists(warehouse.getCode())) {
        throw std::invalid_argument("Warehouse code already exists");
    }
    
    auto id = repo_->create(warehouse);
    auto created = repo_->findById(id);
    if (!created) {
        throw std::runtime_error("Failed to retrieve created warehouse");
    }
    return utils::DtoMapper::toWarehouseDto(*created);
}

dtos::WarehouseDto WarehouseService::updateWarehouse(const models::Warehouse& warehouse) {
    std::string errorMessage;
    if (!isValidWarehouse(warehouse, errorMessage)) {
        utils::Logger::warn("Invalid warehouse update: {}", errorMessage);
        throw std::invalid_argument(errorMessage);
    }
    
    bool success = repo_->update(warehouse);
    if (!success) {
        throw std::runtime_error("Failed to update warehouse");
    }
    
    auto updated = repo_->findById(warehouse.getId());
    if (!updated) {
        throw std::runtime_error("Failed to retrieve updated warehouse");
    }
    return utils::DtoMapper::toWarehouseDto(*updated);
}

bool WarehouseService::deleteWarehouse(const std::string& id) {
    return repo_->deleteById(id);
}

dtos::WarehouseDto WarehouseService::activateWarehouse(const std::string& id) {
    utils::Logger::info("WarehouseService::activateWarehouse({})", id);
    
    auto warehouse = repo_->findById(id);
    if (!warehouse) {
        throw std::runtime_error("Warehouse not found: " + id);
    }
    
    // TODO: Implement status change to active
    warehouse->setStatus(models::Status::Active);
    
    bool success = repo_->update(*warehouse);
    if (!success) {
        throw std::runtime_error("Failed to activate warehouse");
    }
    
    auto updated = repo_->findById(id);
    if (!updated) {
        throw std::runtime_error("Failed to retrieve activated warehouse");
    }
    return utils::DtoMapper::toWarehouseDto(*updated);
}

dtos::WarehouseDto WarehouseService::deactivateWarehouse(const std::string& id) {
    utils::Logger::info("WarehouseService::deactivateWarehouse({})", id);
    
    auto warehouse = repo_->findById(id);
    if (!warehouse) {
        throw std::runtime_error("Warehouse not found: " + id);
    }
    
    // TODO: Implement status change to inactive
    warehouse->setStatus(models::Status::Inactive);
    
    bool success = repo_->update(*warehouse);
    if (!success) {
        throw std::runtime_error("Failed to deactivate warehouse");
    }
    
    auto updated = repo_->findById(id);
    if (!updated) {
        throw std::runtime_error("Failed to retrieve deactivated warehouse");
    }
    return utils::DtoMapper::toWarehouseDto(*updated);
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

// Helper methods for DTO conversion (DRY pattern)
dtos::WarehouseDto WarehouseService::convertToDto(const models::Warehouse& warehouse) {
    return utils::DtoMapper::toWarehouseDto(warehouse);
}

std::vector<dtos::WarehouseDto> WarehouseService::convertToDtos(const std::vector<models::Warehouse>& warehouses) {
    std::vector<dtos::WarehouseDto> dtos;
    dtos.reserve(warehouses.size());
    
    for (const auto& warehouse : warehouses) {
        dtos.push_back(convertToDto(warehouse));
    }
    
    return dtos;
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
