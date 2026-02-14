#include "warehouse/services/LocationService.hpp"
#include "warehouse/repositories/LocationRepository.hpp"
#include "warehouse/utils/Logger.hpp"
#include "warehouse/utils/DtoMapper.hpp"
#include <regex>
#include <algorithm>

namespace warehouse::services {

LocationService::LocationService(std::shared_ptr<repositories::LocationRepository> repo)
    : repo_(repo) {
}

std::optional<dtos::LocationDto> LocationService::getById(const std::string& id) {
    auto location = repo_->findById(id);
    if (!location) {
        return std::nullopt;
    }
    
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + location->getWarehouseId().substr(0, 8);
    
    return utils::DtoMapper::toLocationDto(*location, warehouseCode);
}

std::vector<dtos::LocationDto> LocationService::getAll() {
    return convertToDtos(repo_->findAll());
}

std::vector<dtos::LocationDto> LocationService::getByWarehouse(const std::string& warehouseId) {
    return convertToDtos(repo_->findByWarehouse(warehouseId));
}

std::vector<dtos::LocationDto> LocationService::getByWarehouseAndZone(const std::string& warehouseId, const std::string& zone) {
    return convertToDtos(repo_->findByWarehouseAndZone(warehouseId, zone));
}

std::vector<dtos::LocationDto> LocationService::getAvailablePickingLocations(const std::string& warehouseId) {
    return convertToDtos(repo_->findAvailablePickingLocations(warehouseId));
}

dtos::LocationDto LocationService::createLocation(const models::Location& location) {
    std::string errorMessage;
    if (!isValidLocation(location, errorMessage)) {
        utils::Logger::warn("Invalid location: {}", errorMessage);
        throw std::invalid_argument(errorMessage);
    }
    
    if (repo_->codeExists(location.getWarehouseId(), location.getCode())) {
        throw std::invalid_argument("Location code already exists in this warehouse");
    }
    
    auto id = repo_->create(location);
    auto created = repo_->findById(id);
    if (!created) {
        throw std::runtime_error("Failed to retrieve created location");
    }
    
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + created->getWarehouseId().substr(0, 8);
    
    return utils::DtoMapper::toLocationDto(*created, warehouseCode);
}

dtos::LocationDto LocationService::updateLocation(const models::Location& location) {
    std::string errorMessage;
    if (!isValidLocation(location, errorMessage)) {
        utils::Logger::warn("Invalid location update: {}", errorMessage);
        throw std::invalid_argument(errorMessage);
    }
    
    bool success = repo_->update(location);
    if (!success) {
        throw std::runtime_error("Failed to update location");
    }
    
    auto updated = repo_->findById(location.getId());
    if (!updated) {
        throw std::runtime_error("Failed to retrieve updated location");
    }
    
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + updated->getWarehouseId().substr(0, 8);
    
    return utils::DtoMapper::toLocationDto(*updated, warehouseCode);
}

bool LocationService::deleteLocation(const std::string& id) {
    return repo_->deleteById(id);
}

dtos::LocationDto LocationService::reserveLocation(const std::string& id) {
    utils::Logger::info("LocationService::reserveLocation({})", id);
    
    auto location = repo_->findById(id);
    if (!location) {
        throw std::runtime_error("Location not found: " + id);
    }
    
    // TODO: Implement status change to reserved
    location->setStatus(models::LocationStatus::Reserved);
    
    bool success = repo_->update(*location);
    if (!success) {
        throw std::runtime_error("Failed to reserve location");
    }
    
    auto updated = repo_->findById(id);
    if (!updated) {
        throw std::runtime_error("Failed to retrieve reserved location");
    }
    
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + updated->getWarehouseId().substr(0, 8);
    
    return utils::DtoMapper::toLocationDto(*updated, warehouseCode);
}

dtos::LocationDto LocationService::releaseLocation(const std::string& id) {
    utils::Logger::info("LocationService::releaseLocation({})", id);
    
    auto location = repo_->findById(id);
    if (!location) {
        throw std::runtime_error("Location not found: " + id);
    }
    
    // TODO: Implement status change back to active
    location->setStatus(models::LocationStatus::Available);
    
    bool success = repo_->update(*location);
    if (!success) {
        throw std::runtime_error("Failed to release location");
    }
    
    auto updated = repo_->findById(id);
    if (!updated) {
        throw std::runtime_error("Failed to retrieve released location");
    }
    
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + updated->getWarehouseId().substr(0, 8);
    
    return utils::DtoMapper::toLocationDto(*updated, warehouseCode);
}

dtos::LocationDto LocationService::markLocationFull(const std::string& id) {
    utils::Logger::info("LocationService::markLocationFull({})", id);
    
    auto location = repo_->findById(id);
    if (!location) {
        throw std::runtime_error("Location not found: " + id);
    }
    
    // TODO: Implement status change to full
    location->setStatus(models::LocationStatus::Full);
    
    bool success = repo_->update(*location);
    if (!success) {
        throw std::runtime_error("Failed to mark location as full");
    }
    
    auto updated = repo_->findById(id);
    if (!updated) {
        throw std::runtime_error("Failed to retrieve full location");
    }
    
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + updated->getWarehouseId().substr(0, 8);
    
    return utils::DtoMapper::toLocationDto(*updated, warehouseCode);
}

bool LocationService::isValidLocation(const models::Location& location, std::string& errorMessage) {
    if (location.getWarehouseId().empty()) {
        errorMessage = "Warehouse ID is required";
        return false;
    }
    
    if (location.getCode().empty()) {
        errorMessage = "Location code is required";
        return false;
    }
    
    if (!validateCode(location.getCode())) {
        errorMessage = "Invalid location code format";
        return false;
    }
    
    if (location.getDimensions() && !validateDimensions(*location.getDimensions())) {
        errorMessage = "Invalid location dimensions";
        return false;
    }
    
    return true;
}

std::vector<dtos::LocationDto> LocationService::optimizePickingRoute(const std::vector<std::string>& locationIds) {
    // TODO: Implement route optimization algorithm
    // For now, just return locations in the order they were provided
    utils::Logger::info("LocationService::optimizePickingRoute() called with {} locations", locationIds.size());
    
    std::vector<dtos::LocationDto> route;
    route.reserve(locationIds.size());
    
    for (const auto& id : locationIds) {
        auto location = repo_->findById(id);
        if (location) {
            route.push_back(convertToDto(*location));
        }
    }
    
    return route;
}

// Helper methods for DTO conversion (DRY pattern)
dtos::LocationDto LocationService::convertToDto(const models::Location& location) {
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + location.getWarehouseId().substr(0, 8);
    return utils::DtoMapper::toLocationDto(location, warehouseCode);
}

std::vector<dtos::LocationDto> LocationService::convertToDtos(const std::vector<models::Location>& locations) {
    std::vector<dtos::LocationDto> dtos;
    dtos.reserve(locations.size());
    
    for (const auto& location : locations) {
        dtos.push_back(convertToDto(location));
    }
    
    return dtos;
}

bool LocationService::validateCode(const std::string& code) {
    // Code should match pattern: ^[A-Z0-9-]+$
    std::regex pattern("^[A-Z0-9-]+$");
    return std::regex_match(code, pattern);
}

bool LocationService::validateDimensions(const models::Dimensions& dimensions) {
    return dimensions.length > 0 &&
           dimensions.width > 0 &&
           dimensions.height > 0 &&
           !dimensions.unit.empty();
}

} // namespace warehouse::services
