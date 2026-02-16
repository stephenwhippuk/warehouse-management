#pragma once

#include "order/dtos/OrderDto.hpp"
#include <optional>
#include <vector>
#include <string>

namespace order::models {
    class Order; // Forward declaration
}

namespace order::services {

/**
 * @brief Interface for order business logic service
 * 
 * CRITICAL: Services return DTOs, not domain models.
 * Models remain internal to service/repository layers.
 */
class IOrderService {
public:
    virtual ~IOrderService() = default;
    
    // CRUD operations - return DTOs, not models
    virtual std::optional<dtos::OrderDto> getById(const std::string& id) = 0;
    virtual std::vector<dtos::OrderDto> getAll() = 0;
    virtual dtos::OrderDto create(const models::Order& order) = 0;
    virtual dtos::OrderDto update(const models::Order& order) = 0;
    virtual bool deleteById(const std::string& id) = 0;
    
    // Business operations - return DTOs
    virtual dtos::OrderDto cancelOrder(const std::string& id, const std::string& reason) = 0;
};

} // namespace order::services
