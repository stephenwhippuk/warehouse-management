#include "order/controllers/OrderController.hpp"
#include "order/services/IOrderService.hpp"
#include "order/models/Order.hpp"
#include "order/utils/Logger.hpp"
#include <nlohmann/json.hpp>
#include <Poco/Net/HTTPResponse.h>

using json = nlohmann::json;

namespace order::controllers {

OrderController::OrderController(http::IServiceProvider& provider)
    : http::ControllerBase("/api/v1/orders")
    , provider_(provider) {
    
    // Register routes
    Get("/", [this](http::HttpContext& ctx) {
        return this->getAll(ctx);
    });
    
    Get("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->getById(ctx);
    });
    
    Post("/", [this](http::HttpContext& ctx) {
        return this->create(ctx);
    });
    
    Put("/{id:uuid}", [this](http::HttpContext& ctx) {
        return this->update(ctx);
    });
    
    Post("/{id:uuid}/cancel", [this](http::HttpContext& ctx) {
        return this->cancelOrder(ctx);
    });
}

std::string OrderController::getAll(http::HttpContext& ctx) {
    auto service = ctx.getService<services::IOrderService>();
    
    // Get query parameters
    std::string status = ctx.queryParams.get("status", "");
    std::string customerId = ctx.queryParams.get("customerId", "");
    std::string warehouseId = ctx.queryParams.get("warehouseId", "");
    
    // TODO: Implement filtering by query params
    auto orders = service->getAll();
    
    json j = json::array();
    for (const auto& order : orders) {
        j.push_back(order.toJson());
    }
    
    return j.dump();
}

std::string OrderController::getById(http::HttpContext& ctx) {
    auto service = ctx.getService<services::IOrderService>();
    std::string id = ctx.routeParams["id"];
    
    auto order = service->getById(id);
    if (!order) {
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        return R"({"error": "Order not found"})";
    }
    
    return order->toJson().dump();
}

std::string OrderController::create(http::HttpContext& ctx) {
    auto service = ctx.getService<services::IOrderService>();
    
    json body = ctx.getBodyAsJson();
    
    // Parse order from JSON
    models::Order order = models::Order::fromJson(body);
    
    // Create order
    auto created = service->create(order);
    
    ctx.setStatus(Poco::Net::HTTPResponse::HTTP_CREATED);
    return created.toJson().dump();
}

std::string OrderController::update(http::HttpContext& ctx) {
    auto service = ctx.getService<services::IOrderService>();
    std::string id = ctx.routeParams["id"];
    
    json body = ctx.getBodyAsJson();
    
    // Parse order from JSON
    models::Order order = models::Order::fromJson(body);
    
    // Ensure ID matches
    if (order.getId() != id) {
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        return R"({"error": "Order ID in body does not match URL"})";
    }
    
    // Update order
    auto updated = service->update(order);
    
    return updated.toJson().dump();
}

std::string OrderController::cancelOrder(http::HttpContext& ctx) {
    auto service = ctx.getService<services::IOrderService>();
    std::string id = ctx.routeParams["id"];
    
    json body = ctx.getBodyAsJson();
    std::string reason = body.value("reason", "Customer requested");
    
    // Cancel order
    auto cancelled = service->cancelOrder(id, reason);
    
    return cancelled.toJson().dump();
}

} // namespace order::controllers
