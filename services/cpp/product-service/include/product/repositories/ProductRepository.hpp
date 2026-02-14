#pragma once

#include "product/models/Product.hpp"
#include <optional>
#include <vector>
#include <memory>
#include <pqxx/pqxx>

namespace product::repositories {

/**
 * @brief Product data access layer
 * 
 * Handles all database operations for products.
 * Returns models (internal representation), not DTOs.
 */
class ProductRepository {
public:
    explicit ProductRepository(std::shared_ptr<pqxx::connection> db);

    // CRUD operations
    std::optional<models::Product> findById(const std::string& id);
    std::optional<models::Product> findBySku(const std::string& sku);
    std::vector<models::Product> findAll();
    std::vector<models::Product> findActive();
    models::Product create(const models::Product& product);
    models::Product update(const models::Product& product);
    bool deleteById(const std::string& id);

private:
    std::shared_ptr<pqxx::connection> db_;

    // Helper to convert row to Product
    models::Product rowToProduct(const pqxx::row& row);
};

}  // namespace product::repositories
