#pragma once

#include "warehouse/models/Warehouse.hpp"
#include <http-framework/IServiceProvider.hpp>
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace warehouse::utils {
    class Database;
}

namespace warehouse::repositories {

/**
 * @brief Repository for warehouse data access
 */
class WarehouseRepository {
public:
    explicit WarehouseRepository(http::IServiceProvider& provider);
    
    // CRUD operations
    std::optional<models::Warehouse> findById(const std::string& id);
    std::optional<models::Warehouse> findByCode(const std::string& code);
    std::vector<models::Warehouse> findAll();
    std::vector<models::Warehouse> findByStatus(models::Status status);
    
    std::string create(const models::Warehouse& warehouse);
    bool update(const models::Warehouse& warehouse);
    bool deleteById(const std::string& id);
    
    bool exists(const std::string& id);
    bool codeExists(const std::string& code);

private:
    std::shared_ptr<utils::Database> db_;
    
    models::Warehouse mapRowToWarehouse(/* database row type */ const void* row);
};

} // namespace warehouse::repositories
