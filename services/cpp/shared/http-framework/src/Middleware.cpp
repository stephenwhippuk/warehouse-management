#include "http-framework/Middleware.hpp"
#include "http-framework/ExceptionFilter.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace http {

// ============================================================================
// MiddlewarePipeline Implementation
// ============================================================================

void MiddlewarePipeline::use(std::shared_ptr<Middleware> middleware) {
    middleware_.push_back(middleware);
}

void MiddlewarePipeline::execute(HttpContext& ctx, std::function<void()> finalHandler) {
    executeNext(ctx, 0, finalHandler);
}

size_t MiddlewarePipeline::size() const {
    return middleware_.size();
}

void MiddlewarePipeline::clear() {
    middleware_.clear();
}

void MiddlewarePipeline::executeNext(HttpContext& ctx, size_t index, std::function<void()> finalHandler) {
    if (index >= middleware_.size()) {
        // All middleware executed, call final handler
        finalHandler();
        return;
    }
    
    // Execute current middleware
    middleware_[index]->process(ctx, [this, &ctx, index, finalHandler]() {
        // Next middleware
        executeNext(ctx, index + 1, finalHandler);
    });
}

// ============================================================================
// LoggingMiddleware Implementation
// ============================================================================

void LoggingMiddleware::process(HttpContext& ctx, std::function<void()> next) {
    // Note: In production, use a proper logging library like spdlog
    // For now, just print to stdout
    auto start = std::chrono::steady_clock::now();
    
    std::cout << "[HTTP] " << ctx.getMethod() << " " << ctx.getPath() << std::endl;
    
    next();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "[HTTP] " << ctx.response.getStatus() 
              << " " << ctx.getPath() 
              << " (" << duration.count() << "ms)" << std::endl;
}

// ============================================================================
// CorsMiddleware Implementation
// ============================================================================

CorsMiddleware::CorsMiddleware(const std::string& allowOrigin,
                               const std::string& allowMethods,
                               const std::string& allowHeaders)
    : allowOrigin_(allowOrigin)
    , allowMethods_(allowMethods)
    , allowHeaders_(allowHeaders) {
}

void CorsMiddleware::process(HttpContext& ctx, std::function<void()> next) {
    // Set CORS headers
    ctx.setHeader("Access-Control-Allow-Origin", allowOrigin_);
    ctx.setHeader("Access-Control-Allow-Methods", allowMethods_);
    ctx.setHeader("Access-Control-Allow-Headers", allowHeaders_);
    ctx.setHeader("Access-Control-Max-Age", "3600");
    
    // Handle OPTIONS preflight request
    if (ctx.getMethod() == "OPTIONS") {
        ctx.sendNoContent();
        return;
    }
    
    next();
}

// ============================================================================
// ErrorHandlingMiddleware Implementation
// ============================================================================

ErrorHandlingMiddleware::ErrorHandlingMiddleware(std::shared_ptr<IExceptionFilter> filter)
    : filter_(filter ? filter : std::make_shared<DefaultExceptionFilter>()) {
}

void ErrorHandlingMiddleware::process(HttpContext& ctx, std::function<void()> next) {
    try {
        next();
    } catch (const std::exception& e) {
        // Use the filter to handle the exception
        filter_->handleException(ctx, e);
    } catch (...) {
        // Handle unknown exceptions
        ctx.sendError("Unknown internal server error",
                     Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

// ============================================================================
// AuthenticationMiddleware Implementation
// ============================================================================

AuthenticationMiddleware::AuthenticationMiddleware(const std::string& apiKey,
                                                  const std::vector<std::string>& excludePaths)
    : apiKey_(apiKey)
    , excludePaths_(excludePaths) {
}

void AuthenticationMiddleware::process(HttpContext& ctx, std::function<void()> next) {
    // Check if path is excluded from authentication
    if (isExcluded(ctx.getPath())) {
        next();
        return;
    }
    
    // Extract API key from request
    std::string providedKey = extractApiKey(ctx);
    
    if (providedKey.empty()) {
        ctx.sendError("Missing authentication token", 
                     Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
        return;
    }
    
    if (providedKey != apiKey_) {
        ctx.sendError("Invalid authentication token",
                     Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
        return;
    }
    
    // Authentication successful
    next();
}

bool AuthenticationMiddleware::isExcluded(const std::string& path) const {
    for (const auto& excluded : excludePaths_) {
        if (path == excluded || path.find(excluded) == 0) {
            return true;
        }
    }
    return false;
}

std::string AuthenticationMiddleware::extractApiKey(HttpContext& ctx) const {
    // Try X-Service-Api-Key header
    if (ctx.hasHeader("X-Service-Api-Key")) {
        return ctx.getHeader("X-Service-Api-Key");
    }
    
    // Try Authorization: ApiKey <key>
    if (ctx.hasHeader("Authorization")) {
        std::string auth = ctx.getHeader("Authorization");
        if (auth.find("ApiKey ") == 0) {
            return auth.substr(7);  // Skip "ApiKey "
        }
    }
    
    return "";
}

} // namespace http
