#include "warehouse/dtos/LocationListDto.hpp"
#include <stdexcept>
#include <cmath>

namespace warehouse {
namespace dtos {

LocationListDto::LocationListDto(
    const std::vector<LocationDto>& items,
    int total,
    int page,
    int pageSize)
    : items_(items)
    , total_(total)
    , page_(page)
    , pageSize_(pageSize)
    , totalPages_(pageSize > 0 ? static_cast<int>(std::ceil(static_cast<double>(total) / pageSize)) : 0) {
    
    // Validate pagination fields
    validateNonNegativeInteger(total_, "total");
    validatePositiveInteger(page_, "page");
    validatePositiveInteger(pageSize_, "pageSize");
    
    // Validate items count doesn't exceed page size
    if (items_.size() > static_cast<size_t>(pageSize_)) {
        throw std::invalid_argument("items count cannot exceed pageSize");
    }
}

void LocationListDto::validateNonNegativeInteger(int value, const std::string& fieldName) const {
    if (value < 0) {
        throw std::invalid_argument(fieldName + " must be non-negative");
    }
}

void LocationListDto::validatePositiveInteger(int value, const std::string& fieldName) const {
    if (value < 1) {
        throw std::invalid_argument(fieldName + " must be positive (>= 1)");
    }
}

json LocationListDto::toJson() const {
    json itemsArray = json::array();
    for (const auto& item : items_) {
        itemsArray.push_back(item.toJson());
    }
    
    return {
        {"items", itemsArray},
        {"total", total_},
        {"page", page_},
        {"pageSize", pageSize_},
        {"totalPages", totalPages_}
    };
}

} // namespace dtos
} // namespace warehouse
