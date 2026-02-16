#pragma once

#include "warehouse/services/IWarehouseService.hpp"
#include "warehouse/models/Warehouse.hpp"
#include "warehouse/dtos/WarehouseDto.hpp"
#include <http-framework/IServiceProvider.hpp>
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
class WarehouseService : public IWarehouseService {
public:
    explicit WarehouseService(http::IServiceProvider& provider);
    
    // Business operations - return DTOs, not models
    std::optional<dtos::WarehouseDto> getById(const std::string& id) override;
    std::optional<dtos::WarehouseDto> getByCode(const std::string& code) override;
    std::vector<dtos::WarehouseDto> getAll() override;
    std::vector<dtos::WarehouseDto> getActiveWarehouses() override;
    
    dtos::WarehouseDto createWarehouse(const models::Warehouse& warehouse) override;
    dtos::WarehouseDto updateWarehouse(const models::Warehouse& warehouse) override;
    bool deleteWarehouse(const std::string& id) override;
    dtos::WarehouseDto activateWarehouse(const std::string& id) override;
    dtos::WarehouseDto deactivateWarehouse(const std::string& id) override;
    
    // Validation
    bool isValidWarehouse(const models::Warehouse& warehouse, std::string& errorMessage) override;
    
private:
    std::shared_ptr<repositories::WarehouseRepository> repo_;
    
    // Helper methods for DTO conversion (DRY pattern)
    dtos::WarehouseDto convertToDto(const models::Warehouse& warehouse);
    std::vector<dtos::WarehouseDto> convertToDtos(const std::vector<models::Warehouse>& warehouses);
    
    bool validateCode(const std::string& code);
    bool validateAddress(const models::Address& address);
};

} // namespace warehouse::services
