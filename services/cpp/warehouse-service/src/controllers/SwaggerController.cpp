#include "warehouse/controllers/SwaggerController.hpp"
#include "warehouse/utils/SwaggerGenerator.hpp"
#include "warehouse/utils/Logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace warehouse::controllers {

SwaggerController::SwaggerController()
    : http::ControllerBase("/api/swagger.json") {
    
    // GET /api/swagger.json (OpenAPI specification)
    Get("/", [this](http::HttpContext& ctx) {
        return this->handleSwagger(ctx);
    });
}

std::string SwaggerController::handleSwagger(http::HttpContext& /* ctx */) {
    // Generate OpenAPI specification
    json spec = utils::SwaggerGenerator::generateSpec(
        "Warehouse Service API",
        "1.0.0",
        "API for managing warehouse facilities and storage locations"
    );
    
    // Add tags for grouping endpoints
    spec["tags"] = json::array({
        {{"name", "Warehouses"}, {"description", "Warehouse facility management"}},
        {{"name", "Locations"}, {"description", "Storage location management"}},
        {{"name", "Health"}, {"description", "Service health checks"}}
    });
    
    return spec.dump(2);  // Pretty-printed JSON
}

}  // namespace warehouse::controllers
