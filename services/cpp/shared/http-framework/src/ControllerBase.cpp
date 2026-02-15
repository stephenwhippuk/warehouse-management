#include "http-framework/ControllerBase.hpp"
#include <algorithm>

namespace http {

// ============================================================================
// ControllerBase Implementation
// ============================================================================

ControllerBase::ControllerBase(const std::string& baseRoute) 
    : baseRoute_(baseRoute) {
    // Ensure baseRoute doesn't end with /
    if (!baseRoute_.empty() && baseRoute_.back() == '/') {
        baseRoute_.pop_back();
    }
}

const std::string& ControllerBase::getBaseRoute() const {
    return baseRoute_;
}

const std::vector<std::shared_ptr<Route>>& ControllerBase::getRoutes() const {
    return routes_;
}

void ControllerBase::registerRoutes(Router& router) {
    for (const auto& route : routes_) {
        router.addRoute(route);
    }
}

void ControllerBase::Get(const std::string& pattern, EndpointHandler handler) {
    addRoute("GET", pattern, handler);
}

void ControllerBase::Post(const std::string& pattern, EndpointHandler handler) {
    addRoute("POST", pattern, handler);
}

void ControllerBase::Put(const std::string& pattern, EndpointHandler handler) {
    addRoute("PUT", pattern, handler);
}

void ControllerBase::Delete(const std::string& pattern, EndpointHandler handler) {
    addRoute("DELETE", pattern, handler);
}

void ControllerBase::Patch(const std::string& pattern, EndpointHandler handler) {
    addRoute("PATCH", pattern, handler);
}

void ControllerBase::Options(const std::string& pattern, EndpointHandler handler) {
    addRoute("OPTIONS", pattern, handler);
}

std::string ControllerBase::buildPath(const std::string& pattern) const {
    if (pattern.empty() || pattern == "/") {
        return baseRoute_;
    }
    
    std::string fullPath = baseRoute_;
    if (pattern.front() != '/') {
        fullPath += "/";
    }
    fullPath += pattern;
    
    return fullPath;
}

std::optional<json> ControllerBase::parseJsonBody(HttpContext& ctx) {
    try {
        return ctx.getBodyAsJson();
    } catch (const json::parse_error& e) {
        ctx.sendError("Invalid JSON: " + std::string(e.what()),
                     Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        return std::nullopt;
    }
}

bool ControllerBase::validateRequiredFields(HttpContext& ctx, 
                                            const json& body,
                                            const std::vector<std::string>& fields) {
    std::vector<std::string> missing;
    
    for (const auto& field : fields) {
        if (!body.contains(field)) {
            missing.push_back(field);
        }
    }
    
    if (!missing.empty()) {
        std::string missingFields;
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i > 0) missingFields += ", ";
            missingFields += missing[i];
        }
        
        ctx.sendError("Missing required fields: " + missingFields,
                     Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        return false;
    }
    
    return true;
}

std::optional<std::string> ControllerBase::getRouteParam(HttpContext& ctx, const std::string& name) {
    auto it = ctx.routeParams.find(name);
    if (it == ctx.routeParams.end()) {
        ctx.sendError("Missing route parameter: " + name,
                     Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        return std::nullopt;
    }
    return it->second;
}

void ControllerBase::addRoute(const std::string& method, 
                             const std::string& pattern, 
                             EndpointHandler handler) {
    std::string fullPath = buildPath(pattern);
    auto route = std::make_shared<Route>(method, fullPath, handler);
    routes_.push_back(route);
}

} // namespace http
