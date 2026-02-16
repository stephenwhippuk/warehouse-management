#include "product/controllers/HealthController.hpp"
#include "product/utils/Logger.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

namespace product::controllers {

HealthController::HealthController()
    : http::ControllerBase("/health") {
    
    // GET /health
    Get("/", [this](http::HttpContext& ctx) {
        return this->handleHealth(ctx);
    });
}

std::string HealthController::handleHealth(http::HttpContext& ctx) {
    if (auto logger = utils::Logger::getLogger()) {
        logger->debug("Health check request");
    }
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    
    json body = {
        {"status", "healthy"},
        {"service", "product-service"},
        {"timestamp", ss.str()}
    };
    
    return body.dump();
}

}  // namespace product::controllers
