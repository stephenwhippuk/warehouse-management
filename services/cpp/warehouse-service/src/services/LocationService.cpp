#include "warehouse/services/LocationService.hpp"
#include "warehouse/repositories/LocationRepository.hpp"
#include "warehouse/utils/Logger.hpp"
#include <regex>
#include <algorithm>

namespace warehouse::services {

LocationService::LocationService(std::shared_ptr<repositories::LocationRepository> repo)
    : repo_(repo) {
}

std::optional<models::Location> LocationService::getById(const std::string& id) {
    return repo_->findById(id);
}

std::vector<models::Location> LocationService::getAll() {
    return repo_->findAll();
}

std::vector<models::Location> LocationService::getByWarehouse(const std::string& warehouseId) {
    return repo_->findByWarehouse(warehouseId);
}

std::vector<models::Location> LocationService::getByWarehouseAndZone(const std::string& warehouseId, const std::string& zone) {
    return repo_->findByWarehouseAndZone(warehouseId, zone);
}

std::vector<models::Location> LocationService::getAvailablePickingLocations(const std::string& warehouseId) {
    return repo_->findAvailablePickingLocations(warehouseId);
}

std::string LocationService::createLocation(const models::Location& location) {
    std::string errorMessage;
    if (!isValidLocation(location, errorMessage)) {
        utils::Logger::warn("Invalid location: {}", errorMessage);
        throw std::invalid_argument(errorMessage);
    }
    
    if (repo_->codeExists(location.getWarehouseId(), location.getCode())) {
        throw std::invalid_argument("Location code already exists in this warehouse");
    }
    
    return repo_->create(location);
}

bool LocationService::updateLocation(const models::Location& location) {
    std::string errorMessage;
    if (!isValidLocation(location, errorMessage)) {
        utils::Logger::warn("Invalid location update: {}", errorMessage);
        return false;
    }
    
    return repo_->update(location);
}

bool LocationService::deleteLocation(const std::string& id) {
    return repo_->deleteById(id);
}

bool LocationService::reserveLocation(const std::string& id) {
    // TODO: Implement status change to reserved
    utils::Logger::info("LocationService::reserveLocation({})", id);
    return false;
}

bool LocationService::releaseLocation(const std::string& id) {
    // TODO: Implement status change back to active
    utils::Logger::info("LocationService::releaseLocation({})", id);
    return false;
}

bool LocationService::markLocationFull(const std::string& id) {
    // TODO: Implement status change to full
    utils::Logger::info("LocationService::markLocationFull({})", id);
    return false;
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

std::vector<models::Location> LocationService::optimizePickingRoute(const std::vector<std::string>& locationIds) {
    // TODO: Implement route optimization algorithm
    // For now, just return locations in the order they were provided
    utils::Logger::info("LocationService::optimizePickingRoute() called with {} locations", locationIds.size());
    
    std::vector<models::Location> route;
    for (const auto& id : locationIds) {
        auto location = repo_->findById(id);
        if (location) {
            route.push_back(*location);
        }
    }
    
    return route;
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
