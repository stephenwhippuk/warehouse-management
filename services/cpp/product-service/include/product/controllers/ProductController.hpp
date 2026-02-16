#pragma once

#include "http-framework/ControllerBase.hpp"
#include "http-framework/HttpContext.hpp"
#include <memory>
#include <string>

namespace product::controllers {

/**
 * @brief Product API endpoints handler
 * 
 * Handles:
 * - GET /api/v1/products (list)
 * - GET /api/v1/products/{id} (get by id)
 * - POST /api/v1/products (create)
 * - PUT /api/v1/products/{id} (update)
 * - DELETE /api/v1/products/{id} (delete)
 */
class ProductController : public http::ControllerBase {
public:
    ProductController();

private:
    // Handler methods for different operations
    std::string handleGetAll(http::HttpContext& ctx);
    std::string handleGetById(http::HttpContext& ctx);
    std::string handleCreate(http::HttpContext& ctx);
    std::string handleUpdate(http::HttpContext& ctx);
    std::string handleDelete(http::HttpContext& ctx);
};

}  // namespace product::controllers
