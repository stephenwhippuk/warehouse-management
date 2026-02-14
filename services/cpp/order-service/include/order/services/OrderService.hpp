#pragma once

#include "order/models/Order.hpp"
#include "order/dtos/OrderDto.hpp"
#include <optional>
#include <vector>
#include <memory>

namespace order::repositories {
    class OrderRepository; // Forward declaration
}

namespace order::services {

/**
 * @brief Service layer for order business logic
 * 
 * CRITICAL: Services return DTOs, not domain models.
 * Models remain internal to service/repository layers.
 */
class OrderService {
public:
    explicit OrderService(std::shared_ptr<repositories::OrderRepository> repository);
    
    // CRUD operations - return DTOs, not models
    std::optional<dtos::OrderDto> getById(const std::string& id);
    std::vector<dtos::OrderDto> getAll();
    dtos::OrderDto create(const models::Order& order);
    dtos::OrderDto update(const models::Order& order);
    bool deleteById(const std::string& id);
    
    // Business operations - return DTOs
    dtos::OrderDto cancelOrder(const std::string& id, const std::string& reason);
    
private:
    std::shared_ptr<repositories::OrderRepository> repository_;
};

} // namespace order::services
