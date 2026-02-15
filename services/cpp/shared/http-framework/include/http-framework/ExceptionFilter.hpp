#pragma once

#include "http-framework/HttpException.hpp"
#include "http-framework/HttpContext.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

using json = nlohmann::json;

namespace http {

/**
 * @brief Exception filter interface
 * 
 * Filters handle the conversion of exceptions to HTTP responses.
 * Custom filters can be implemented to provide application-specific
 * error handling logic.
 */
class IExceptionFilter {
public:
    virtual ~IExceptionFilter() = default;
    
    /**
     * @brief Handle an exception and generate appropriate HTTP response
     * @param ctx HTTP context
     * @param e The exception to handle
     * @return true if the exception was handled, false to continue to next filter
     */
    virtual bool handleException(HttpContext& ctx, const std::exception& e) = 0;
};

/**
 * @brief Default exception filter
 * 
 * Handles standard HTTP exceptions and common C++ exceptions.
 * Converts them to consistent JSON error responses.
 */
class DefaultExceptionFilter : public IExceptionFilter {
public:
    /**
     * @brief Handle an exception
     * @param ctx HTTP context
     * @param e The exception to handle
     * @return true (always handles all exceptions as a catch-all)
     */
    bool handleException(HttpContext& ctx, const std::exception& e) override;

private:
    /**
     * @brief Create standard error response JSON
     */
    json createErrorResponse(const std::string& message, int status, const std::string& path) const;
};

/**
 * @brief Composite exception filter
 * 
 * Chains multiple filters together, trying each in order until
 * one handles the exception.
 */
class CompositeExceptionFilter : public IExceptionFilter {
public:
    /**
     * @brief Add a filter to the chain
     */
    void addFilter(std::shared_ptr<IExceptionFilter> filter);
    
    /**
     * @brief Handle exception by trying filters in order
     */
    bool handleException(HttpContext& ctx, const std::exception& e) override;
    
    /**
     * @brief Get number of filters
     */
    size_t size() const;

private:
    std::vector<std::shared_ptr<IExceptionFilter>> filters_;
};

} // namespace http
