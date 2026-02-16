#pragma once

#include <http-framework/ControllerBase.hpp>
#include <http-framework/HttpContext.hpp>
#include <http-framework/IServiceProvider.hpp>
#include <memory>
#include <string>

namespace order::controllers {

/**
 * @brief HTTP controller for order endpoints using http-framework
 * 
 * Handles:
 * - GET /api/v1/orders - List orders
 * - GET /api/v1/orders/:id - Get order by ID
 * - POST /api/v1/orders - Create new order
 * - PUT /api/v1/orders/:id - Update order
 * - POST /api/v1/orders/:id/cancel - Cancel order
 */
class OrderController : public http::ControllerBase {
public:
    explicit OrderController(http::IServiceProvider& provider);

private:
    std::string getAll(http::HttpContext& ctx);
    std::string getById(http::HttpContext& ctx);
    std::string create(http::HttpContext& ctx);
    std::string update(http::HttpContext& ctx);
    std::string cancelOrder(http::HttpContext& ctx);
    
    http::IServiceProvider& provider_;
};

} // namespace order::controllers
