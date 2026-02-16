#pragma once

#include "order/services/IOrderService.hpp"
#include "order/models/Order.hpp"
#include "order/dtos/OrderDto.hpp"
#include "order/repositories/OrderRepository.hpp"
#include <http-framework/IServiceProvider.hpp>
#include <optional>
#include <vector>
#include <memory>

namespace order::services {

/**
 * @brief Service layer for order business logic
 * 
 * CRITICAL: Services return DTOs, not domain models.
 * Models remain internal to service/repository layers.
 */
class OrderService : public IOrderService {
public:
    // CRITICAL: Constructor MUST accept IServiceProvider for dependency injection
    explicit OrderService(http::IServiceProvider& provider);
    
    // Override all interface methods
    std::optional<dtos::OrderDto> getById(const std::string& id) override;
    std::vector<dtos::OrderDto> getAll() override;
    dtos::OrderDto create(const models::Order& order) override;
    dtos::OrderDto update(const models::Order& order) override;
    bool deleteById(const std::string& id) override;
    
    // Business operations - return DTOs
    dtos::OrderDto cancelOrder(const std::string& id, const std::string& reason) override;
    
private:
    std::shared_ptr<repositories::OrderRepository> repository_;
};

} // namespace order::services
