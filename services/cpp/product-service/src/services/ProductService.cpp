#include "product/services/ProductService.hpp"
#include "product/utils/DtoMapper.hpp"
#include <stdexcept>
#include <uuid/uuid.h>
#include <cstring>

namespace product::services {

ProductService::ProductService(std::shared_ptr<repositories::ProductRepository> repository)
    : repository_(repository) {
    if (!repository_) {
        throw std::invalid_argument("Repository cannot be null");
    }
}

std::optional<dtos::ProductItemDto> ProductService::getById(const std::string& id) {
    auto product = repository_->findById(id);
    if (!product) {
        return std::nullopt;
    }
    
    return utils::DtoMapper::toProductItemDto(*product);
}

std::optional<dtos::ProductItemDto> ProductService::getBySku(const std::string& sku) {
    auto product = repository_->findBySku(sku);
    if (!product) {
        return std::nullopt;
    }
    
    return utils::DtoMapper::toProductItemDto(*product);
}

dtos::ProductListDto ProductService::getAll(int page, int pageSize) {
    auto products = repository_->findAll();
    
    std::vector<dtos::ProductItemDto> dtos;
    dtos.reserve(products.size());
    
    for (const auto& product : products) {
        dtos.push_back(utils::DtoMapper::toProductItemDto(product));
    }
    
    int totalCount = dtos.size();
    int totalPages = (totalCount + pageSize - 1) / pageSize;
    
    return dtos::ProductListDto(dtos, totalCount, page, pageSize, totalPages);
}

dtos::ProductListDto ProductService::getActive(int page, int pageSize) {
    auto products = repository_->findActive();
    
    std::vector<dtos::ProductItemDto> dtos;
    dtos.reserve(products.size());
    
    for (const auto& product : products) {
        dtos.push_back(utils::DtoMapper::toProductItemDto(product));
    }
    
    int totalCount = dtos.size();
    int totalPages = (totalCount + pageSize - 1) / pageSize;
    
    return dtos::ProductListDto(dtos, totalCount, page, pageSize, totalPages);
}

dtos::ProductItemDto ProductService::create(const std::string& sku,
                                            const std::string& name,
                                            const std::optional<std::string>& description,
                                            const std::optional<std::string>& category) {
    // Generate UUID
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    
    models::Product product(
        uuid_str,
        sku,
        name,
        description,
        category,
        models::Product::Status::ACTIVE
    );
    
    auto created = repository_->create(product);
    return utils::DtoMapper::toProductItemDto(created);
}

dtos::ProductItemDto ProductService::update(const std::string& id,
                                            const std::string& name,
                                            const std::optional<std::string>& description,
                                            const std::optional<std::string>& category,
                                            const std::string& status) {
    auto existing = repository_->findById(id);
    if (!existing) {
        throw std::runtime_error("Product not found: " + id);
    }
    
    // Convert status string to enum
    models::Product::Status statusEnum;
    if (status == "active") {
        statusEnum = models::Product::Status::ACTIVE;
    } else if (status == "inactive") {
        statusEnum = models::Product::Status::INACTIVE;
    } else if (status == "discontinued") {
        statusEnum = models::Product::Status::DISCONTINUED;
    } else {
        throw std::invalid_argument("Invalid status: " + status);
    }
    
    existing->setName(name);
    existing->setDescription(description);
    existing->setCategory(category);
    existing->setStatus(statusEnum);
    
    auto updated = repository_->update(*existing);
    return utils::DtoMapper::toProductItemDto(updated);
}

bool ProductService::deleteById(const std::string& id) {
    return repository_->deleteById(id);
}

}  // namespace product::services
