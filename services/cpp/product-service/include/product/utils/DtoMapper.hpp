#pragma once

#include "product/models/Product.hpp"
#include "product/dtos/ProductItemDto.hpp"

namespace product::utils {

/**
 * @brief Maps between models and DTOs
 * 
 * Responsible for converting Product models to ProductItemDto DTOs.
 * This ensures proper validation and ensures models remain internal.
 */
class DtoMapper {
public:
    static dtos::ProductItemDto toProductItemDto(const models::Product& product);
};

}  // namespace product::utils
