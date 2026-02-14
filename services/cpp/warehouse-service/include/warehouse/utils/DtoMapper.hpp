#ifndef WAREHOUSE_UTILS_DTO_MAPPER_HPP
#define WAREHOUSE_UTILS_DTO_MAPPER_HPP

#include "warehouse/dtos/WarehouseDto.hpp"
#include "warehouse/dtos/LocationDto.hpp"
#include "warehouse/models/Warehouse.hpp"
#include "warehouse/models/Location.hpp"
#include <memory>

namespace warehouse {
namespace utils {

/**
 * @brief Utility class for converting domain models to DTOs
 * 
 * Provides static methods to convert internal models to external
 * Data Transfer Objects that conform to contract specifications.
 */
class DtoMapper {
public:
    /**
     * @brief Convert Warehouse model to WarehouseDto
     * 
     * @param warehouse The warehouse domain model
     * @return dtos::WarehouseDto The DTO representation
     */
    static dtos::WarehouseDto toWarehouseDto(const models::Warehouse& warehouse);
    
    /**
     * @brief Convert Location model to LocationDto
     * 
     * @param location The location domain model
     * @param warehouseCode The warehouse code (identity field)
     * @param warehouseName Optional warehouse name
     * @return dtos::LocationDto The DTO representation
     * 
     * @note warehouseCode is required as an identity field for the Warehouse reference
     * @note Real implementation should fetch from warehouse-service API
     */
    static dtos::LocationDto toLocationDto(
        const models::Location& location,
        const std::string& warehouseCode,
        const std::optional<std::string>& warehouseName = std::nullopt);

private:
    // Prevent instantiation
    DtoMapper() = default;
};

} // namespace utils
} // namespace warehouse

#endif // WAREHOUSE_UTILS_DTO_MAPPER_HPP
