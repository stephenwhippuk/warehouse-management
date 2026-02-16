#pragma once

#include "warehouse/dtos/WarehouseDto.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace warehouse::models {
    class Warehouse;
}

namespace warehouse::services {

/**
 * @brief Interface for warehouse service business logic
 * 
 * All operations return DTOs (Data Transfer Objects), not domain models.
 * Models remain internal to the service/repository layers.
 */
class IWarehouseService {
public:
    virtual ~IWarehouseService() = default;
    
    // Query operations
    virtual std::optional<dtos::WarehouseDto> getById(const std::string& id) = 0;
    virtual std::optional<dtos::WarehouseDto> getByCode(const std::string& code) = 0;
    virtual std::vector<dtos::WarehouseDto> getAll() = 0;
    virtual std::vector<dtos::WarehouseDto> getActiveWarehouses() = 0;
    
    // Mutation operations
    virtual dtos::WarehouseDto createWarehouse(const models::Warehouse& warehouse) = 0;
    virtual dtos::WarehouseDto updateWarehouse(const models::Warehouse& warehouse) = 0;
    virtual bool deleteWarehouse(const std::string& id) = 0;
    virtual dtos::WarehouseDto activateWarehouse(const std::string& id) = 0;
    virtual dtos::WarehouseDto deactivateWarehouse(const std::string& id) = 0;
    
    // Validation
    virtual bool isValidWarehouse(const models::Warehouse& warehouse, std::string& errorMessage) = 0;
};

}  // namespace warehouse::services
