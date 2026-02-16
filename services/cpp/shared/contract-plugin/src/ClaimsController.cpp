#include "contract-plugin/ClaimsController.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <Poco/Net/HTTPResponse.h>

using json = nlohmann::json;

namespace contract {

ClaimsController::ClaimsController(const ContractConfig& config,
                                   std::shared_ptr<IClaimsService> claimsService)
    : http::ControllerBase("/api/v1/claims")
    , config_(config)
    , claimsService_(claimsService) {
    
    // Register routes matching inventory-service ClaimsController
    Get("/", [this](http::HttpContext& ctx) {
        return this->handleGetAllClaims(ctx);
    });
    
    Get("/fulfilments", [this](http::HttpContext& ctx) {
        return this->handleGetFulfilments(ctx);
    });
    
    Get("/references", [this](http::HttpContext& ctx) {
        return this->handleGetReferences(ctx);
    });
    
    Get("/services", [this](http::HttpContext& ctx) {
        return this->handleGetServices(ctx);
    });
    
    Get("/supports/{type:alpha}/{name:alphanum}/{version:alphanum}", [this](http::HttpContext& ctx) {
        return this->handleSupportsCheck(ctx);
    });
    
    spdlog::info("ClaimsController initialized with 5 endpoints (claimsPath={})",
                 config_.claimsPath);
}

std::string ClaimsController::handleGetAllClaims(http::HttpContext& ctx) {
    auto claims = claimsService_->getAllClaims();
    ctx.setHeader("Content-Type", "application/json");
    return claims.dump(2);
}

std::string ClaimsController::handleGetFulfilments(http::HttpContext& ctx) {
    auto fulfilments = claimsService_->getFulfilments();
    ctx.setHeader("Content-Type", "application/json");
    return fulfilments.dump(2);
}

std::string ClaimsController::handleGetReferences(http::HttpContext& ctx) {
    auto references = claimsService_->getReferences();
    ctx.setHeader("Content-Type", "application/json");
    return references.dump(2);
}

std::string ClaimsController::handleGetServices(http::HttpContext& ctx) {
    auto services = claimsService_->getServices();
    ctx.setHeader("Content-Type", "application/json");
    return services.dump(2);
}

std::string ClaimsController::handleSupportsCheck(http::HttpContext& ctx) {
    std::string type = ctx.routeParams["type"];
    std::string name = ctx.routeParams["name"];
    std::string version = ctx.routeParams["version"];
    
    auto result = claimsService_->checkSupport(type, name, version);
    ctx.setHeader("Content-Type", "application/json");
    return result.dump(2);
}

} // namespace contract
