#pragma once

#include "http-framework/HttpContext.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace http {

/**
 * @brief Base class for HTTP middleware
 * 
 * Middleware can inspect/modify the request, execute logic, and pass control
 * to the next middleware in the pipeline by calling next().
 * 
 * Example:
 * class LoggingMiddleware : public Middleware {
 * public:
 *     void process(HttpContext& ctx, std::function<void()> next) override {
 *         Logger::info("Request: {} {}", ctx.getMethod(), ctx.getPath());
 *         next();  // Call next middleware
 *         Logger::info("Response: {}", ctx.response.getStatus());
 *     }
 * };
 */
class Middleware {
public:
    virtual ~Middleware() = default;
    
    /**
     * @brief Process the request
     * @param ctx HTTP context
     * @param next Function to call next middleware in pipeline
     */
    virtual void process(HttpContext& ctx, std::function<void()> next) = 0;
};

/**
 * @brief Middleware pipeline - chains middleware together
 */
class MiddlewarePipeline {
public:
    /**
     * @brief Add middleware to the end of the pipeline
     */
    void use(std::shared_ptr<Middleware> middleware);
    
    /**
     * @brief Execute the middleware pipeline
     * @param ctx HTTP context
     * @param finalHandler Handler to call after all middleware
     */
    void execute(HttpContext& ctx, std::function<void()> finalHandler);
    
    /**
     * @brief Get number of middleware in pipeline
     */
    size_t size() const;
    
    /**
     * @brief Clear all middleware
     */
    void clear();

private:
    std::vector<std::shared_ptr<Middleware>> middleware_;
    
    void executeNext(HttpContext& ctx, size_t index, std::function<void()> finalHandler);
};

// ============================================================================
// Built-in Middleware
// ============================================================================

/**
 * @brief Logging middleware - logs request/response with timing
 */
class LoggingMiddleware : public Middleware {
public:
    void process(HttpContext& ctx, std::function<void()> next) override;
};

/**
 * @brief CORS middleware - adds Cross-Origin Resource Sharing headers
 */
class CorsMiddleware : public Middleware {
public:
    explicit CorsMiddleware(const std::string& allowOrigin = "*",
                           const std::string& allowMethods = "GET, POST, PUT, DELETE, OPTIONS",
                           const std::string& allowHeaders = "Content-Type, Authorization, X-Service-Api-Key");
    
    void process(HttpContext& ctx, std::function<void()> next) override;

private:
    std::string allowOrigin_;
    std::string allowMethods_;
    std::string allowHeaders_;
};

/**
 * @brief Error handling middleware - catches exceptions and returns error responses
 * 
 * Uses an exception filter to convert exceptions to HTTP responses.
 * By default, uses DefaultExceptionFilter which handles standard HTTP exceptions.
 */
class ErrorHandlingMiddleware : public Middleware {
public:
    /**
     * @brief Constructor with custom exception filter
     * @param filter Exception filter to use (defaults to DefaultExceptionFilter)
     */
    explicit ErrorHandlingMiddleware(std::shared_ptr<class IExceptionFilter> filter = nullptr);

    /**
     * @brief Replace the exception filter (must be non-null)
     */
    void setExceptionFilter(std::shared_ptr<class IExceptionFilter> filter);
    
    void process(HttpContext& ctx, std::function<void()> next) override;

private:
    std::shared_ptr<class IExceptionFilter> filter_;
};

/**
 * @brief Authentication middleware - validates service-to-service API keys
 */
class AuthenticationMiddleware : public Middleware {
public:
    /**
     * @brief Constructor
     * @param apiKey Expected API key for authentication
     * @param excludePaths Paths that don't require authentication
     */
    explicit AuthenticationMiddleware(const std::string& apiKey,
                                     const std::vector<std::string>& excludePaths = {"/health", "/swagger.json"});
    
    void process(HttpContext& ctx, std::function<void()> next) override;

private:
    std::string apiKey_;
    std::vector<std::string> excludePaths_;
    
    bool isExcluded(const std::string& path) const;
    std::string extractApiKey(HttpContext& ctx) const;
};

} // namespace http
