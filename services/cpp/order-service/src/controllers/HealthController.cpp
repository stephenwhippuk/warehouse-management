#include "order/controllers/HealthController.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace order::controllers {

HealthController::HealthController(http::IServiceProvider& provider)
    : http::ControllerBase("/health") {
    
    Get("/", [this](http::HttpContext& ctx) {
        return this->health(ctx);
    });
}

std::string HealthController::health(http::HttpContext& ctx) {
    (void)ctx;  // Unused parameter
    
    json response = {
        {"status", "healthy"},
        {"service", "order-service"},
        {"timestamp", std::time(nullptr)}
    };
    
    return response.dump();
}

} // namespace order::controllers
