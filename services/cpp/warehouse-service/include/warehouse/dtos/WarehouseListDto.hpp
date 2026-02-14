#pragma once

#include "warehouse/dtos/WarehouseDto.hpp"
#include <vector>
#include <nlohmann/json.hpp>

namespace warehouse {
namespace dtos {

using json = nlohmann::json;

/**
 * @brief Paginated list of warehouses
 * 
 * Conforms to WarehouseListDto contract v1.0
 */
class WarehouseListDto {
public:
    /**
     * @brief Construct warehouse list DTO with pagination
     * @param items Vector of warehouse DTOs
     * @param total Total number of warehouses (NonNegativeInteger)
     * @param page Current page number (PositiveInteger)
     * @param pageSize Number of items per page (PositiveInteger)
     */
    WarehouseListDto(const std::vector<WarehouseDto>& items,
                     int total,
                     int page,
                     int pageSize);

    // Immutable getters
    const std::vector<WarehouseDto>& getItems() const { return items_; }
    int getTotal() const { return total_; }
    int getPage() const { return page_; }
    int getPageSize() const { return pageSize_; }
    int getTotalPages() const { return totalPages_; }

    // Serialization
    json toJson() const;

private:
    std::vector<WarehouseDto> items_;
    int total_;
    int page_;
    int pageSize_;
    int totalPages_;

    // Validation
    void validateNonNegativeInteger(int value, const std::string& fieldName) const;
    void validatePositiveInteger(int value, const std::string& fieldName) const;
};

} // namespace dtos
} // namespace warehouse
