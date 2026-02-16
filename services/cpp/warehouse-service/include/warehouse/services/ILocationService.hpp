#pragma once

#include "warehouse/dtos/LocationDto.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace warehouse::models {
    class Location;
}

namespace warehouse::services {

/**
 * @brief Interface for location service business logic
 * 
 * All operations return DTOs (Data Transfer Objects), not domain models.
 * Models remain internal to the service/repository layers.
 */
class ILocationService {
public:
    virtual ~ILocationService() = default;
    
    // Query operations
    virtual std::optional<dtos::LocationDto> getById(const std::string& id) = 0;
    virtual std::vector<dtos::LocationDto> getAll() = 0;
    virtual std::vector<dtos::LocationDto> getByWarehouse(const std::string& warehouseId) = 0;
    virtual std::vector<dtos::LocationDto> getByWarehouseAndZone(const std::string& warehouseId, const std::string& zone) = 0;
    virtual std::vector<dtos::LocationDto> getAvailablePickingLocations(const std::string& warehouseId) = 0;
    
    // Mutation operations
    virtual dtos::LocationDto createLocation(const models::Location& location) = 0;
    virtual dtos::LocationDto updateLocation(const models::Location& location) = 0;
    virtual bool deleteLocation(const std::string& id) = 0;
    virtual dtos::LocationDto reserveLocation(const std::string& id) = 0;
    virtual dtos::LocationDto releaseLocation(const std::string& id) = 0;
    virtual dtos::LocationDto markLocationFull(const std::string& id) = 0;
    
    // Validation
    virtual bool isValidLocation(const models::Location& location, std::string& errorMessage) = 0;
    
    // Route optimization
    virtual std::vector<dtos::LocationDto> optimizePickingRoute(const std::vector<std::string>& locationIds) = 0;
};

}  // namespace warehouse::services
