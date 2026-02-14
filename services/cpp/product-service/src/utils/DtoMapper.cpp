#include "product/utils/DtoMapper.hpp"

namespace product::utils {

dtos::ProductItemDto DtoMapper::toProductItemDto(const models::Product& product) {
    // Convert status enum to string
    std::string statusStr;
    switch (product.getStatus()) {
        case models::Product::Status::ACTIVE:
            statusStr = "active";
            break;
        case models::Product::Status::INACTIVE:
            statusStr = "inactive";
            break;
        case models::Product::Status::DISCONTINUED:
            statusStr = "discontinued";
            break;
    }
    
    return dtos::ProductItemDto(
        product.getId(),
        product.getSku(),
        product.getName(),
        product.getDescription(),
        product.getCategory(),
        statusStr
    );
}

}  // namespace product::utils
