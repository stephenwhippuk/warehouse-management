#include "warehouse/controllers/HealthController.hpp"
#include "warehouse/utils/Logger.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

namespace warehouse::controllers {

HealthController::HealthController()
    : http::ControllerBase("/health") {
    
    // GET /health (health check)
    Get("/", [this](http::HttpContext& ctx) {
        return this->handleHealth(ctx);
    });
}

std::string HealthController::handleHealth(http::HttpContext& ctx) {
    try {
        // Get current timestamp in ISO 8601 format
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        
        json response = {
            {"status", "healthy"},
            {"service", "warehouse-service"},
            {"timestamp", ss.str()}
        };
        
        return response.dump();
    } catch (const std::exception& e) {
        utils::Logger::error("Error in handleHealth: {}", e.what());
        ctx.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        json errorResponse = {
            {"status", "unhealthy"},
            {"service", "warehouse-service"},
            {"error", e.what()}
        };
        return errorResponse.dump();
    }
}

}  // namespace warehouse::controllers
