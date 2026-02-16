#pragma once

#include "http-framework/ControllerBase.hpp"
#include "http-framework/HttpContext.hpp"
#include <string>

namespace warehouse::controllers {

/**
 * @brief HTTP controller for location endpoints
 * 
 * Handles:
 * - GET /api/v1/locations - List all locations
 * - GET /api/v1/locations/{id} - Get location by ID
 * - POST /api/v1/locations - Create new location
 * - PUT /api/v1/locations/{id} - Update location
 * - DELETE /api/v1/locations/{id} - Delete location
 */
class LocationController : public http::ControllerBase {
public:
    LocationController();

private:
    std::string handleGetAll(http::HttpContext& ctx);
    std::string handleGetById(http::HttpContext& ctx);
    std::string handleCreate(http::HttpContext& ctx);
    std::string handleUpdate(http::HttpContext& ctx);
    std::string handleDelete(http::HttpContext& ctx);
};

} // namespace warehouse::controllers
