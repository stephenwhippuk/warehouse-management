#include "warehouse/controllers/ClaimsController.hpp"
#include "warehouse/utils/Logger.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

namespace warehouse::controllers {

ClaimsController::ClaimsController()
    : http::ControllerBase("/api/v1/claims") {
    
    // GET /api/v1/claims (service claims)
    Get("/", [this](http::HttpContext& ctx) {
        return this->handleClaims(ctx);
    });
}

std::string ClaimsController::handleClaims(http::HttpContext& /* ctx */) {
    // Load and return service claims
    try {
        std::ifstream claimsFile("claims.json");
        json claims;
        claimsFile >> claims;
        return claims.dump(2);
    } catch (const std::exception& e) {
        // Return minimal claims if file not found
        json claims = {
            {"service", "warehouse-service"},
            {"version", "1.0.0"},
            {"status", "operational"}
        };
        return claims.dump(2);
    }
}

}  // namespace warehouse::controllers
