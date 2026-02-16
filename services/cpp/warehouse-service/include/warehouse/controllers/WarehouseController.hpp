#pragma once

#include "http-framework/ControllerBase.hpp"
#include "http-framework/HttpContext.hpp"
#include <string>

namespace warehouse::controllers {

/**
 * @brief HTTP controller for warehouse endpoints
 * 
 * Handles:
 * - GET /api/v1/warehouses - List all warehouses
 * - GET /api/v1/warehouses/{id} - Get warehouse by ID
 * - POST /api/v1/warehouses - Create new warehouse
 * - PUT /api/v1/warehouses/{id} - Update warehouse
 * - DELETE /api/v1/warehouses/{id} - Delete warehouse
 */
class WarehouseController : public http::ControllerBase {
public:
    WarehouseController();

private:
    std::string handleGetAll(http::HttpContext& ctx);
    std::string handleGetById(http::HttpContext& ctx);
    std::string handleCreate(http::HttpContext& ctx);
    std::string handleUpdate(http::HttpContext& ctx);
    std::string handleDelete(http::HttpContext& ctx);
};

}  // namespace warehouse::controllers
