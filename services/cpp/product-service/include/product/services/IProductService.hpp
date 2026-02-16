#pragma once

#include "product/dtos/ProductItemDto.hpp"
#include "product/dtos/ProductListDto.hpp"
#include <memory>
#include <optional>
#include <string>

namespace product::services {

/**
 * @brief Interface for product business logic
 * 
 * All operations return DTOs (Data Transfer Objects), not domain models.
 * Models remain internal to the service/repository layers.
 */
class IProductService {
public:
    virtual ~IProductService() = default;
    
    // Query operations
    virtual std::optional<dtos::ProductItemDto> getById(const std::string& id) = 0;
    virtual std::optional<dtos::ProductItemDto> getBySku(const std::string& sku) = 0;
    virtual dtos::ProductListDto getAll(int page = 1, int pageSize = 50) = 0;
    virtual dtos::ProductListDto getActive(int page = 1, int pageSize = 50) = 0;
    
    // Mutation operations
    virtual dtos::ProductItemDto create(const std::string& sku,
                                       const std::string& name,
                                       const std::optional<std::string>& description,
                                       const std::optional<std::string>& category) = 0;
    
    virtual dtos::ProductItemDto update(const std::string& id,
                                       const std::string& name,
                                       const std::optional<std::string>& description,
                                       const std::optional<std::string>& category,
                                       const std::string& status) = 0;
    
    virtual bool deleteById(const std::string& id) = 0;
};

}  // namespace product::services
