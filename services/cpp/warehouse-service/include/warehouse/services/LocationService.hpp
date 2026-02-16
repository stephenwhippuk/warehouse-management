#pragma once

#include "warehouse/services/ILocationService.hpp"
#include "warehouse/models/Location.hpp"
#include "warehouse/dtos/LocationDto.hpp"
#include <http-framework/IServiceProvider.hpp>
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
class LocationService : public ILocationService {
public:
    explicit LocationService(http::IServiceProvider& provider);
    
    // Business operations - return DTOs, not models
    std::optional<dtos::LocationDto> getById(const std::string& id) override;
    std::vector<dtos::LocationDto> getAll() override;
    std::vector<dtos::LocationDto> getByWarehouse(const std::string& warehouseId) override;
    std::vector<dtos::LocationDto> getByWarehouseAndZone(const std::string& warehouseId, const std::string& zone) override;
    std::vector<dtos::LocationDto> getAvailablePickingLocations(const std::string& warehouseId) override;
    
    dtos::LocationDto createLocation(const models::Location& location) override;
    dtos::LocationDto updateLocation(const models::Location& location) override;
    bool deleteLocation(const std::string& id) override;
    dtos::LocationDto reserveLocation(const std::string& id) override;
    dtos::LocationDto releaseLocation(const std::string& id) override;
    dtos::LocationDto markLocationFull(const std::string& id) override;
    
    // Validation
    bool isValidLocation(const models::Location& location, std::string& errorMessage) override;
    
    // Route optimization
    std::vector<dtos::LocationDto> optimizePickingRoute(const std::vector<std::string>& locationIds) override;
    
private:
    std::shared_ptr<repositories::LocationRepository> repo_;
    
    // Helper methods for DTO conversion (DRY pattern)
    dtos::LocationDto convertToDto(const models::Location& location);
    std::vector<dtos::LocationDto> convertToDtos(const std::vector<models::Location>& locations);
    
    bool validateCode(const std::string& code);
    bool validateDimensions(const models::Dimensions& dimensions);
};

} // namespace warehouse::services
