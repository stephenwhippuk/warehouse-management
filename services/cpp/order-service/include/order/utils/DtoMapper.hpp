#ifndef ORDER_UTILS_DTO_MAPPER_HPP
#define ORDER_UTILS_DTO_MAPPER_HPP

#include "order/dtos/OrderDto.hpp"
#include "order/models/Order.hpp"
#include <memory>

namespace order {
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
     * @brief Convert Order model to OrderDto
     * 
     * @param order The order domain model
     * @param warehouseCode The warehouse code (identity field)
     * @param warehouseName Optional warehouse name
     * @return dtos::OrderDto The DTO representation
     * 
     * @note warehouseCode is required as an identity field for the Warehouse reference
     * @note Real implementation should fetch from warehouse-service API
     */
    static dtos::OrderDto toOrderDto(
        const models::Order& order,
        const std::string& warehouseCode,
        const std::optional<std::string>& warehouseName = std::nullopt);

private:
    // Prevent instantiation
    DtoMapper() = default;
};

} // namespace utils
} // namespace order

#endif // ORDER_UTILS_DTO_MAPPER_HPP
