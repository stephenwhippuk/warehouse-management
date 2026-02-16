#pragma once

#include "warehouse/models/Location.hpp"
#include <http-framework/IServiceProvider.hpp>
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace warehouse::utils {
    class Database;
}

namespace warehouse::repositories {

/**
 * @brief Repository for location data access
 */
class LocationRepository {
public:
    explicit LocationRepository(http::IServiceProvider& provider);
    
    // CRUD operations
    std::optional<models::Location> findById(const std::string& id);
    std::optional<models::Location> findByCode(const std::string& warehouseId, const std::string& code);
    std::vector<models::Location> findAll();
    std::vector<models::Location> findByWarehouse(const std::string& warehouseId);
    std::vector<models::Location> findByWarehouseAndZone(const std::string& warehouseId, const std::string& zone);
    std::vector<models::Location> findByWarehouseAndType(const std::string& warehouseId, models::LocationType type);
    std::vector<models::Location> findByStatus(models::LocationStatus status);
    std::vector<models::Location> findAvailablePickingLocations(const std::string& warehouseId);
    
    std::string create(const models::Location& location);
    bool update(const models::Location& location);
    bool deleteById(const std::string& id);
    
    bool exists(const std::string& id);
    bool codeExists(const std::string& warehouseId, const std::string& code);

private:
    std::shared_ptr<utils::Database> db_;
    
    models::Location mapRowToLocation(/* database row type */ const void* row);
};

} // namespace warehouse::repositories
