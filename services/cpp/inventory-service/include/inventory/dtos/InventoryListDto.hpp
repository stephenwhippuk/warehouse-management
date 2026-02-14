#pragma once

#include "inventory/dtos/InventoryItemDto.hpp"
#include <vector>
#include <nlohmann/json.hpp>

namespace inventory {
namespace dtos {

using json = nlohmann::json;

/**
 * @brief Paginated list of inventory items DTO
 * 
 * Conforms to InventoryListDto contract v1.0
 */
class InventoryListDto {
public:
    /**
     * @brief Construct inventory list DTO with all required fields
     * @param items Array of inventory items
     * @param totalCount Total number of items matching the query (NonNegativeInteger)
     * @param page Current page number (PositiveInteger)
     * @param pageSize Number of items per page (PositiveInteger)
     * @param totalPages Total number of pages (PositiveInteger)
     */
    InventoryListDto(const std::vector<InventoryItemDto>& items,
                     int totalCount,
                     int page,
                     int pageSize,
                     int totalPages);

    // Getters (immutable)
    const std::vector<InventoryItemDto>& getItems() const { return items_; }
    int getTotalCount() const { return totalCount_; }
    int getPage() const { return page_; }
    int getPageSize() const { return pageSize_; }
    int getTotalPages() const { return totalPages_; }

    // Serialization
    json toJson() const;

private:
    std::vector<InventoryItemDto> items_;
    int totalCount_;
    int page_;
    int pageSize_;
    int totalPages_;

    // Validation
    void validateNonNegativeInteger(int value, const std::string& fieldName) const;
    void validatePositiveInteger(int value, const std::string& fieldName) const;
};

} // namespace dtos
} // namespace inventory
