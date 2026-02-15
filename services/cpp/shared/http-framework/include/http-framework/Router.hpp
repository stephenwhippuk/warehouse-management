#pragma once

#include "http-framework/HttpContext.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <optional>
#include <regex>
#include <memory>

namespace http {

/**
 * @brief Route parameter constraint types
 */
enum class RouteConstraint {
    None,
    Uuid,      // Must match UUID format
    Int,       // Must be integer
    Alpha,     // Must be alphabetic
    AlphaNum   // Must be alphanumeric
};

/**
 * @brief Route parameter definition
 */
struct RouteParameter {
    std::string name;
    RouteConstraint constraint = RouteConstraint::None;
    bool required = true;
};

/**
 * @brief Endpoint handler function type
 * 
 * Handlers receive an HttpContext and return a JSON string response.
 * The response status can be set via ctx.setStatus() before returning.
 */
using EndpointHandler = std::function<std::string(HttpContext&)>;

/**
 * @brief HTTP route definition
 * 
 * Represents a single route pattern (e.g., "/inventory/{id}") with method and handler.
 */
class Route {
public:
    /**
     * @brief Constructor
     * @param method HTTP method (GET, POST, PUT, DELETE, etc.)
     * @param pattern Route pattern (e.g., "/inventory/{id}/reserve")
     * @param handler Endpoint handler function
     */
    Route(const std::string& method, 
          const std::string& pattern,
          EndpointHandler handler);
    
    /**
     * @brief Check if this route matches the request
     * @param method HTTP method
     * @param path Request path
     * @return true if route matches
     */
    bool matches(const std::string& method, const std::string& path) const;
    
    /**
     * @brief Extract route parameters from path
     * @param path Request path
     * @return Map of parameter names to values
     */
    std::map<std::string, std::string> extractParameters(const std::string& path) const;
    
    /**
     * @brief Get the endpoint handler
     */
    const EndpointHandler& getHandler() const;
    
    /**
     * @brief Get route method
     */
    const std::string& getMethod() const;
    
    /**
     * @brief Get route pattern
     */
    const std::string& getPattern() const;
    
    /**
     * @brief Get route parameters
     */
    const std::vector<RouteParameter>& getParameters() const;

private:
    std::string method_;
    std::string pattern_;
    EndpointHandler handler_;
    std::vector<RouteParameter> parameters_;
    std::regex regex_;
    std::vector<std::string> segments_;
    
    void parsePattern();
    std::string patternToRegex() const;
    RouteConstraint parseConstraint(const std::string& constraintStr) const;
    std::string getConstraintRegex(RouteConstraint constraint) const;
};

/**
 * @brief Route table - manages all registered routes
 */
class Router {
public:
    /**
     * @brief Add a route to the router
     */
    void addRoute(const std::string& method, 
                  const std::string& pattern, 
                  EndpointHandler handler);
    
    /**
     * @brief Add a route from Route object
     */
    void addRoute(std::shared_ptr<Route> route);
    
    /**
     * @brief Find matching route for request
     * @param method HTTP method
     * @param path Request path
     * @return Matching route or nullptr if no match
     */
    std::shared_ptr<Route> findRoute(const std::string& method, const std::string& path) const;
    
    /**
     * @brief Get all routes
     */
    const std::vector<std::shared_ptr<Route>>& getRoutes() const;
    
    /**
     * @brief Clear all routes
     */
    void clear();
    
    /**
     * @brief Get number of routes
     */
    size_t size() const;

private:
    std::vector<std::shared_ptr<Route>> routes_;
};

/**
 * @brief Route builder helper for fluent API
 * 
 * Example:
 * RouteBuilder(router)
 *     .get("/inventory", handler1)
 *     .get("/inventory/{id}", handler2)
 *     .post("/inventory", handler3);
 */
class RouteBuilder {
public:
    explicit RouteBuilder(Router& router);
    
    RouteBuilder& get(const std::string& pattern, EndpointHandler handler);
    RouteBuilder& post(const std::string& pattern, EndpointHandler handler);
    RouteBuilder& put(const std::string& pattern, EndpointHandler handler);
    RouteBuilder& del(const std::string& pattern, EndpointHandler handler);
    RouteBuilder& patch(const std::string& pattern, EndpointHandler handler);
    RouteBuilder& options(const std::string& pattern, EndpointHandler handler);

private:
    Router& router_;
};

} // namespace http
