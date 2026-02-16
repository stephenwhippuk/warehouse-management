#pragma once

#include <http-framework/ControllerBase.hpp>
#include <http-framework/IServiceProvider.hpp>
#include <memory>

namespace inventory {
namespace controllers {

class InventoryController : public http::ControllerBase {
public:
    InventoryController();
    
private:
    // Helper methods
    http::IServiceProvider& getScopedProvider(http::HttpContext& ctx);
    void requireServiceAuth(http::HttpContext& ctx);
    
    // Handler methods
    std::string handleGetAll(http::HttpContext& ctx);
    std::string handleGetById(http::HttpContext& ctx);
    std::string handleGetByProduct(http::HttpContext& ctx);
    std::string handleGetByWarehouse(http::HttpContext& ctx);
    std::string handleGetByLocation(http::HttpContext& ctx);
    std::string handleGetLowStock(http::HttpContext& ctx);
    std::string handleGetExpired(http::HttpContext& ctx);
    std::string handleCreate(http::HttpContext& ctx);
    std::string handleUpdate(http::HttpContext& ctx);
    std::string handleDelete(http::HttpContext& ctx);
    std::string handleReserve(http::HttpContext& ctx);
    std::string handleRelease(http::HttpContext& ctx);
    std::string handleAllocate(http::HttpContext& ctx);
    std::string handleDeallocate(http::HttpContext& ctx);
    std::string handleAdjust(http::HttpContext& ctx);
};

} // namespace controllers
} // namespace inventory
