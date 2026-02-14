#pragma once

#include "warehouse/dtos/LocationDto.hpp"
#include <vector>
#include <nlohmann/json.hpp>

namespace warehouse {
namespace dtos {

using json = nlohmann::json;

/**
 * @brief Paginated list of locations
 * 
 * Conforms to LocationListDto contract v1.0
 */
class LocationListDto {
public:
    /**
     * @brief Construct location list DTO with pagination
     * @param items Vector of location DTOs
     * @param total Total number of locations (NonNegativeInteger)
     * @param page Current page number (PositiveInteger)
     * @param pageSize Number of items per page (PositiveInteger)
     */
    LocationListDto(const std::vector<LocationDto>& items,
                    int total,
                    int page,
                    int pageSize);

    // Immutable getters
    const std::vector<LocationDto>& getItems() const { return items_; }
    int getTotal() const { return total_; }
    int getPage() const { return page_; }
    int getPageSize() const { return pageSize_; }
    int getTotalPages() const { return totalPages_; }

    // Serialization
    json toJson() const;

private:
    std::vector<LocationDto> items_;
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
