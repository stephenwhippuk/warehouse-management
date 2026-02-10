#pragma once

#include "order/models/Order.hpp"
#include <optional>
#include <vector>
#include <memory>

namespace order::repositories {
    class OrderRepository; // Forward declaration
}

namespace order::services {

/**
 * @brief Service layer for order business logic
 */
class OrderService {
public:
    explicit OrderService(std::shared_ptr<repositories::OrderRepository> repository);
    
    // CRUD operations
    std::optional<models::Order> getById(const std::string& id);
    std::vector<models::Order> getAll();
    models::Order create(const models::Order& order);
    models::Order update(const models::Order& order);
    bool deleteById(const std::string& id);
    
    // Business operations
    models::Order cancelOrder(const std::string& id, const std::string& reason);
    
private:
    std::shared_ptr<repositories::OrderRepository> repository_;
};

} // namespace order::services
