#include "order/controllers/ClaimsController.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

namespace order::controllers {

ClaimsController::ClaimsController(http::IServiceProvider& provider)
    : http::ControllerBase("/api/v1/claims") {
    
    Get("/", [this](http::HttpContext& ctx) {
        return this->getClaims(ctx);
    });
}

std::string ClaimsController::getClaims(http::HttpContext& ctx) {
    (void)ctx;  // Unused parameter
    
    // Read claims.json file
    std::ifstream file("claims.json");
    if (!file.is_open()) {
        json error = {{"error", "claims.json not found"}};
        return error.dump();
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    return content;
}

} // namespace order::controllers
