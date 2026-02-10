#pragma once

#include "order/models/Order.hpp"
#include <optional>
#include <vector>
#include <memory>

namespace order::repositories {

/**
 * @brief Repository for order data access
 */
class OrderRepository {
public:
    OrderRepository() = default;
    
    // CRUD operations
    std::optional<models::Order> findById(const std::string& id);
    std::vector<models::Order> findAll();
    models::Order create(const models::Order& order);
    models::Order update(const models::Order& order);
    bool deleteById(const std::string& id);
    
    // Query operations
    std::vector<models::Order> findByStatus(const std::string& status);
    std::vector<models::Order> findByCustomerId(const std::string& customerId);
    std::vector<models::Order> findByWarehouseId(const std::string& warehouseId);
};

} // namespace order::repositories
