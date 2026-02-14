#include "order/dtos/OrderListDto.hpp"
#include <stdexcept>

namespace order {
namespace dtos {

OrderListDto::OrderListDto(
    const std::vector<OrderDto>& items,
    int totalCount,
    int page,
    int pageSize,
    int totalPages)
    : items_(items)
    , totalCount_(totalCount)
    , page_(page)
    , pageSize_(pageSize)
    , totalPages_(totalPages) {
    
    // Validate all fields
    validateNonNegativeInteger(totalCount_, "totalCount");
    validatePositiveInteger(page_, "page");
    validatePositiveInteger(pageSize_, "pageSize");
    validatePositiveInteger(totalPages_, "totalPages");
}

void OrderListDto::validateNonNegativeInteger(int value, const std::string& fieldName) const {
    if (value < 0) {
        throw std::invalid_argument(fieldName + " must be non-negative");
    }
}

void OrderListDto::validatePositiveInteger(int value, const std::string& fieldName) const {
    if (value < 1) {
        throw std::invalid_argument(fieldName + " must be positive (greater than 0)");
    }
}

json OrderListDto::toJson() const {
    json itemsJson = json::array();
    for (const auto& item : items_) {
        itemsJson.push_back(item.toJson());
    }
    
    return {
        {"items", itemsJson},
        {"totalCount", totalCount_},
        {"page", page_},
        {"pageSize", pageSize_},
        {"totalPages", totalPages_}
    };
}

} // namespace dtos
} // namespace order
