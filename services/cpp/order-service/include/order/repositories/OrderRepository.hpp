#pragma once

#include "order/models/Order.hpp"
#include "order/utils/Database.hpp"
#include <http-framework/IServiceProvider.hpp>
#include <optional>
#include <vector>
#include <memory>

namespace order::repositories {

/**
 * @brief Repository for order data access
 * 
 * CRITICAL: Uses Database singleton via dependency injection
 */
class OrderRepository {
public:
    // Constructor accepts IServiceProvider for dependency injection
    explicit OrderRepository(http::IServiceProvider& provider);
    
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
    
private:
    // Database singleton (NOT raw pqxx::connection)
    std::shared_ptr<utils::Database> db_;
};

} // namespace order::repositories
