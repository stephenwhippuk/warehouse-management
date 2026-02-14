#pragma once

#include "warehouse/models/Warehouse.hpp"
#include "warehouse/dtos/WarehouseDto.hpp"
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
 * 
 * CRITICAL: Services return DTOs, not domain models.
 * Models remain internal to service/repository layers.
 */
class WarehouseService {
public:
    explicit WarehouseService(std::shared_ptr<repositories::WarehouseRepository> repo);
    
    // Business operations - return DTOs, not models
    std::optional<dtos::WarehouseDto> getById(const std::string& id);
    std::optional<dtos::WarehouseDto> getByCode(const std::string& code);
    std::vector<dtos::WarehouseDto> getAll();
    std::vector<dtos::WarehouseDto> getActiveWarehouses();
    
    dtos::WarehouseDto createWarehouse(const models::Warehouse& warehouse);
    dtos::WarehouseDto updateWarehouse(const models::Warehouse& warehouse);
    bool deleteWarehouse(const std::string& id);
    dtos::WarehouseDto activateWarehouse(const std::string& id);
    dtos::WarehouseDto deactivateWarehouse(const std::string& id);
    
    // Validation
    bool isValidWarehouse(const models::Warehouse& warehouse, std::string& errorMessage);
    
private:
    std::shared_ptr<repositories::WarehouseRepository> repo_;
    
    // Helper methods for DTO conversion (DRY pattern)
    dtos::WarehouseDto convertToDto(const models::Warehouse& warehouse);
    std::vector<dtos::WarehouseDto> convertToDtos(const std::vector<models::Warehouse>& warehouses);
    
    bool validateCode(const std::string& code);
    bool validateAddress(const models::Address& address);
};

} // namespace warehouse::services
