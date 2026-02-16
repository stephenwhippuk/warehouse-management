#pragma once

#include "product/services/IProductService.hpp"
#include "product/repositories/ProductRepository.hpp"
#include "product/dtos/ProductItemDto.hpp"
#include "product/dtos/ProductListDto.hpp"
#include <http-framework/IServiceProvider.hpp>
#include <warehouse/messaging/EventPublisher.hpp>
#include <memory>
#include <optional>

namespace product::services {

/**
 * @brief Product business logic layer
 * 
 * Handles product operations and converts models to DTOs.
 * CRITICAL: Always returns DTOs, never models.
 * Publishes domain events to message bus for cross-service integration.
 */
class ProductService : public IProductService {
public:
    explicit ProductService(http::IServiceProvider& provider);

    // Product queries - return DTOs
    std::optional<dtos::ProductItemDto> getById(const std::string& id) override;
    std::optional<dtos::ProductItemDto> getBySku(const std::string& sku) override;
    dtos::ProductListDto getAll(int page = 1, int pageSize = 50) override;
    dtos::ProductListDto getActive(int page = 1, int pageSize = 50) override;

    // Product mutations - return DTOs and publish events
    dtos::ProductItemDto create(const std::string& sku,
                                const std::string& name,
                                const std::optional<std::string>& description,
                                const std::optional<std::string>& category) override;
    
    dtos::ProductItemDto update(const std::string& id,
                                const std::string& name,
                                const std::optional<std::string>& description,
                                const std::optional<std::string>& category,
                                const std::string& status) override;
    
    bool deleteById(const std::string& id) override;

private:
    std::shared_ptr<repositories::ProductRepository> repository_;
    std::shared_ptr<warehouse::messaging::EventPublisher> eventPublisher_;
    
    // Event publishing
    void publishProductCreated(const dtos::ProductItemDto& product);
    void publishProductUpdated(const dtos::ProductItemDto& product);
    void publishProductDeleted(const std::string& id, const std::string& sku);
    
    // Helper methods
    std::string generateUuid() const;
    std::string getCurrentTimestamp() const;
};

}  // namespace product::services
