#include "product/dtos/ProductListDto.hpp"
#include <stdexcept>

namespace product::dtos {

ProductListDto::ProductListDto(const std::vector<ProductItemDto>& items,
                               int totalCount,
                               int page,
                               int pageSize,
                               int totalPages)
    : items_(items)
    , totalCount_(totalCount)
    , page_(page)
    , pageSize_(pageSize)
    , totalPages_(totalPages) {
    
    if (totalCount < 0) {
        throw std::invalid_argument("totalCount must be non-negative");
    }
    if (page < 1) {
        throw std::invalid_argument("page must be at least 1");
    }
    if (pageSize < 1) {
        throw std::invalid_argument("pageSize must be at least 1");
    }
    if (totalPages < 0) {
        throw std::invalid_argument("totalPages must be non-negative");
    }
}

json ProductListDto::toJson() const {
    json itemsArray = json::array();
    for (const auto& item : items_) {
        itemsArray.push_back(item.toJson());
    }
    
    return json{
        {"items", itemsArray},
        {"totalCount", totalCount_},
        {"page", page_},
        {"pageSize", pageSize_},
        {"totalPages", totalPages_}
    };
}

}  // namespace product::dtos
