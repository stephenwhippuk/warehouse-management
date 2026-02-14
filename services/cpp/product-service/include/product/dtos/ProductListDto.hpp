#pragma once

#include "ProductItemDto.hpp"
#include <vector>

namespace product::dtos {

/**
 * @brief Paginated product list response
 * 
 * Immutable DTO for exposing paginated product lists.
 * CRITICAL: getItems() returns const reference to avoid expensive copies
 */
class ProductListDto {
public:
    ProductListDto(const std::vector<ProductItemDto>& items,
                   int totalCount,
                   int page,
                   int pageSize,
                   int totalPages);

    // Immutable getters
    // CRITICAL: Return collection by const reference (zero-cost access)
    const std::vector<ProductItemDto>& getItems() const { return items_; }
    
    // Scalar values returned by value
    int getTotalCount() const { return totalCount_; }
    int getPage() const { return page_; }
    int getPageSize() const { return pageSize_; }
    int getTotalPages() const { return totalPages_; }
    
    json toJson() const;

private:
    std::vector<ProductItemDto> items_;
    int totalCount_;
    int page_;
    int pageSize_;
    int totalPages_;
};

}  // namespace product::dtos
