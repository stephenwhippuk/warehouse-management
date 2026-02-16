#include "product/services/ProductService.hpp"
#include "product/repositories/ProductRepository.hpp"
#include "product/utils/DtoMapper.hpp"
#include "product/utils/Logger.hpp"
#include <warehouse/messaging/EventPublisher.hpp>
#include <warehouse/messaging/Event.hpp>
#include <stdexcept>
#include <uuid/uuid.h>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace product::services {

ProductService::ProductService(http::IServiceProvider& provider)
    : repository_(provider.getService<repositories::ProductRepository>()),
      eventPublisher_(provider.getService<warehouse::messaging::EventPublisher>()) {
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
    auto dto = utils::DtoMapper::toProductItemDto(created);
    
    // Publish ProductCreated event
    publishProductCreated(dto);
    
    return dto;
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
    auto dto = utils::DtoMapper::toProductItemDto(updated);
    
    // Publish ProductUpdated event
    publishProductUpdated(dto);
    
    return dto;
}

bool ProductService::deleteById(const std::string& id) {
    // Get product details before deletion for event
    auto product = repository_->findById(id);
    if (!product) {
        return false;
    }
    
    std::string sku = product->getSku();
    bool deleted = repository_->deleteById(id);
    
    if (deleted) {
        // Publish ProductDeleted event
        publishProductDeleted(id, sku);
    }
    
    return deleted;
}

std::string ProductService::generateUuid() const {
    uuid_t uuid;
    char uuid_str[37];
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

std::string ProductService::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

void ProductService::publishProductCreated(const dtos::ProductItemDto& product) {
    if (!eventPublisher_) {
        utils::Logger::debug("Event publisher not available, skipping ProductCreated event");
        return;
    }
    
    try {
        // Create event with product data
        warehouse::messaging::Event event("product.created", product.toJson(), "product-service");
        
        // Publish event (library handles all the details)
        eventPublisher_->publish(event);
        
        utils::Logger::info("Published product.created event for product {} (event id: {})", 
                          product.getId(), event.getId());
    } catch (const std::exception& e) {
        utils::Logger::error("Failed to publish ProductCreated event: {}", e.what());
    }
}

void ProductService::publishProductUpdated(const dtos::ProductItemDto& product) {
    if (!eventPublisher_) {
        utils::Logger::debug("Event publisher not available, skipping ProductUpdated event");
        return;
    }
    
    try {
        // Create event with product data
        warehouse::messaging::Event event("product.updated", product.toJson(), "product-service");
        
        // Publish event
        eventPublisher_->publish(event);
        
        utils::Logger::info("Published product.updated event for product {} (event id: {})", 
                          product.getId(), event.getId());
    } catch (const std::exception& e) {
        utils::Logger::error("Failed to publish ProductUpdated event: {}", e.what());
    }
}

void ProductService::publishProductDeleted(const std::string& id, const std::string& sku) {
    if (!eventPublisher_) {
        utils::Logger::debug("Event publisher not available, skipping ProductDeleted event");
        return;
    }
    
    try {
        // Create event data
        json data = {
            {"id", id},
            {"sku", sku}
        };
        
        warehouse::messaging::Event event("product.deleted", data, "product-service");
        
        // Publish event
        eventPublisher_->publish(event);
        
        utils::Logger::info("Published product.deleted event for product {} (event id: {})", 
                          id, event.getId());
    } catch (const std::exception& e) {
        utils::Logger::error("Failed to publish ProductDeleted event: {}", e.what());
    }
}

}  // namespace product::services
