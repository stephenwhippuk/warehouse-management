#pragma once

#include "warehouse/models/Location.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace warehouse::repositories {
    class LocationRepository; // Forward declaration
}

namespace warehouse::services {

/**
 * @brief Business logic service for location operations
 */
class LocationService {
public:
    explicit LocationService(std::shared_ptr<repositories::LocationRepository> repo);
    
    // Business operations
    std::optional<models::Location> getById(const std::string& id);
    std::vector<models::Location> getAll();
    std::vector<models::Location> getByWarehouse(const std::string& warehouseId);
    std::vector<models::Location> getByWarehouseAndZone(const std::string& warehouseId, const std::string& zone);
    std::vector<models::Location> getAvailablePickingLocations(const std::string& warehouseId);
    
    std::string createLocation(const models::Location& location);
    bool updateLocation(const models::Location& location);
    bool deleteLocation(const std::string& id);
    bool reserveLocation(const std::string& id);
    bool releaseLocation(const std::string& id);
    bool markLocationFull(const std::string& id);
    
    // Validation
    bool isValidLocation(const models::Location& location, std::string& errorMessage);
    
    // Route optimization
    std::vector<models::Location> optimizePickingRoute(const std::vector<std::string>& locationIds);
    
private:
    std::shared_ptr<repositories::LocationRepository> repo_;
    
    bool validateCode(const std::string& code);
    bool validateDimensions(const models::Dimensions& dimensions);
};

} // namespace warehouse::services
