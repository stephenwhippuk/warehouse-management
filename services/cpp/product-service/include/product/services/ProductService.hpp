#pragma once

#include "product/dtos/ProductItemDto.hpp"
#include "product/dtos/ProductListDto.hpp"
#include "product/repositories/ProductRepository.hpp"
#include <memory>
#include <optional>

namespace product::services {

/**
 * @brief Product business logic layer
 * 
 * Handles product operations and converts models to DTOs.
 * CRITICAL: Always returns DTOs, never models.
 */
class ProductService {
public:
    explicit ProductService(std::shared_ptr<repositories::ProductRepository> repository);

    // Product queries - return DTOs
    std::optional<dtos::ProductItemDto> getById(const std::string& id);
    std::optional<dtos::ProductItemDto> getBySku(const std::string& sku);
    dtos::ProductListDto getAll(int page = 1, int pageSize = 50);
    dtos::ProductListDto getActive(int page = 1, int pageSize = 50);

    // Product mutations - return DTOs
    dtos::ProductItemDto create(const std::string& sku,
                                const std::string& name,
                                const std::optional<std::string>& description,
                                const std::optional<std::string>& category);
    
    dtos::ProductItemDto update(const std::string& id,
                                const std::string& name,
                                const std::optional<std::string>& description,
                                const std::optional<std::string>& category,
                                const std::string& status);
    
    bool deleteById(const std::string& id);

private:
    std::shared_ptr<repositories::ProductRepository> repository_;
};

}  // namespace product::services
