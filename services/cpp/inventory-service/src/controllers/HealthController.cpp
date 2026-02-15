#include "inventory/controllers/HealthController.hpp"
#include "inventory/utils/Auth.hpp"
#include "inventory/utils/Logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace inventory {
namespace controllers {

HealthController::HealthController() : ControllerBase("/health") {
    Get("/", [this](http::HttpContext& ctx) {
        return handleHealthCheck(ctx);
    });
}

std::string HealthController::handleHealthCheck(http::HttpContext& ctx) {
    (void)ctx; // Unused for now

    utils::Logger::debug("Health check requested");

    json payload;
    payload["status"] = "ok";
    payload["service"] = "inventory-service";

    json authMetrics;
    authMetrics["authorized"] = utils::Auth::authorizedCount();
    authMetrics["missingToken"] = utils::Auth::missingTokenCount();
    authMetrics["invalidToken"] = utils::Auth::invalidTokenCount();

    payload["auth"] = authMetrics;

    return payload.dump();
}

} // namespace controllers
} // namespace inventory
