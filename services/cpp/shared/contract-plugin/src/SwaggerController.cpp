#include "contract-plugin/SwaggerController.hpp"
#include <spdlog/spdlog.h>

using json = nlohmann::json;

namespace contract {

SwaggerController::SwaggerController(const ContractConfig& config,
                                    std::shared_ptr<ISwaggerService> swaggerService)
    : http::ControllerBase("/api")
    , config_(config)
    , swaggerService_(swaggerService) {
    
    // Register route: GET /api/swagger.json
    Get("/swagger.json", [this](http::HttpContext& ctx) {
        return this->getSwagger(ctx);
    });
    
    spdlog::info("SwaggerController initialized (contracts={})", config_.contractsPath);
}

std::string SwaggerController::getSwagger(http::HttpContext& ctx) {
    auto spec = swaggerService_->generateSpec();
    ctx.setHeader("Content-Type", "application/json");
    return spec.dump(2); // Pretty-print
}

} // namespace contract
