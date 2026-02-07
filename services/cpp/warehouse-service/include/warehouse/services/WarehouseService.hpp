#pragma once

#include "warehouse/models/Warehouse.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace warehouse::repositories {
    class WarehouseRepository; // Forward declaration
}

namespace warehouse::services {

/**
 * @brief Business logic service for warehouse operations
 */
class WarehouseService {
public:
    explicit WarehouseService(std::shared_ptr<repositories::WarehouseRepository> repo);
    
    // Business operations
    std::optional<models::Warehouse> getById(const std::string& id);
    std::optional<models::Warehouse> getByCode(const std::string& code);
    std::vector<models::Warehouse> getAll();
    std::vector<models::Warehouse> getActiveWarehouses();
    
    std::string createWarehouse(const models::Warehouse& warehouse);
    bool updateWarehouse(const models::Warehouse& warehouse);
    bool deleteWarehouse(const std::string& id);
    bool activateWarehouse(const std::string& id);
    bool deactivateWarehouse(const std::string& id);
    
    // Validation
    bool isValidWarehouse(const models::Warehouse& warehouse, std::string& errorMessage);
    
private:
    std::shared_ptr<repositories::WarehouseRepository> repo_;
    
    bool validateCode(const std::string& code);
    bool validateAddress(const models::Address& address);
};

} // namespace warehouse::services
