#pragma once

#include "warehouse/models/Location.hpp"
#include "warehouse/dtos/LocationDto.hpp"
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
 * 
 * CRITICAL: Services return DTOs, not domain models.
 * Models remain internal to service/repository layers.
 */
class LocationService {
public:
    explicit LocationService(std::shared_ptr<repositories::LocationRepository> repo);
    
    // Business operations - return DTOs, not models
    std::optional<dtos::LocationDto> getById(const std::string& id);
    std::vector<dtos::LocationDto> getAll();
    std::vector<dtos::LocationDto> getByWarehouse(const std::string& warehouseId);
    std::vector<dtos::LocationDto> getByWarehouseAndZone(const std::string& warehouseId, const std::string& zone);
    std::vector<dtos::LocationDto> getAvailablePickingLocations(const std::string& warehouseId);
    
    dtos::LocationDto createLocation(const models::Location& location);
    dtos::LocationDto updateLocation(const models::Location& location);
    bool deleteLocation(const std::string& id);
    dtos::LocationDto reserveLocation(const std::string& id);
    dtos::LocationDto releaseLocation(const std::string& id);
    dtos::LocationDto markLocationFull(const std::string& id);
    
    // Validation
    bool isValidLocation(const models::Location& location, std::string& errorMessage);
    
    // Route optimization
    std::vector<dtos::LocationDto> optimizePickingRoute(const std::vector<std::string>& locationIds);
    
private:
    std::shared_ptr<repositories::LocationRepository> repo_;
    
    // Helper methods for DTO conversion (DRY pattern)
    dtos::LocationDto convertToDto(const models::Location& location);
    std::vector<dtos::LocationDto> convertToDtos(const std::vector<models::Location>& locations);
    
    bool validateCode(const std::string& code);
    bool validateDimensions(const models::Dimensions& dimensions);
};

} // namespace warehouse::services
