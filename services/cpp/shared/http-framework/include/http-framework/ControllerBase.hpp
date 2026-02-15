#pragma once

#include "http-framework/HttpContext.hpp"
#include "http-framework/Router.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace http {

/**
 * @brief Base class for HTTP controllers
 * 
 * Controllers group related endpoints under a common base route.
 * Each controller registers its endpoints in the constructor.
 * 
 * Example:
 * class InventoryController : public ControllerBase {
 * public:
 *     InventoryController(std::shared_ptr<InventoryService> service) 
 *         : ControllerBase("/api/v1/inventory"), service_(service) {
 *         
 *         Get("/", [this](HttpContext& ctx) { return getAll(ctx); });
 *         Get("/{id}", [this](HttpContext& ctx) { return getById(ctx); });
 *         Post("/", [this](HttpContext& ctx) { return create(ctx); });
 *         Put("/{id}", [this](HttpContext& ctx) { return update(ctx); });
 *         Delete("/{id}", [this](HttpContext& ctx) { return deleteById(ctx); });
 *         Post("/{id}/reserve", [this](HttpContext& ctx) { return reserve(ctx); });
 *     }
 *     
 * private:
 *     std::shared_ptr<InventoryService> service_;
 *     
 *     std::string getAll(HttpContext& ctx) {
 *         // Implementation
 *     }
 * };
 */
class ControllerBase {
public:
    /**
     * @brief Constructor
     * @param baseRoute Base route for all endpoints (e.g., "/api/v1/inventory")
     */
    explicit ControllerBase(const std::string& baseRoute);
    
    virtual ~ControllerBase() = default;
    
    /**
     * @brief Get the base route
     */
    const std::string& getBaseRoute() const;
    
    /**
     * @brief Get all routes defined by this controller
     */
    const std::vector<std::shared_ptr<Route>>& getRoutes() const;
    
    /**
     * @brief Register all controller routes with router
     */
    void registerRoutes(Router& router);

protected:
    /**
     * @brief Register GET endpoint
     * @param pattern Route pattern relative to baseRoute (e.g., "/{id}")
     * @param handler Endpoint handler function
     */
    void Get(const std::string& pattern, EndpointHandler handler);
    
    /**
     * @brief Register POST endpoint
     */
    void Post(const std::string& pattern, EndpointHandler handler);
    
    /**
     * @brief Register PUT endpoint
     */
    void Put(const std::string& pattern, EndpointHandler handler);
    
    /**
     * @brief Register DELETE endpoint
     */
    void Delete(const std::string& pattern, EndpointHandler handler);
    
    /**
     * @brief Register PATCH endpoint
     */
    void Patch(const std::string& pattern, EndpointHandler handler);
    
    /**
     * @brief Register OPTIONS endpoint
     */
    void Options(const std::string& pattern, EndpointHandler handler);
    
    /**
     * @brief Build full route path from relative pattern
     */
    std::string buildPath(const std::string& pattern) const;
    
    /**
     * @brief Helper: Parse JSON body or return error
     * @return json object or std::nullopt if parsing failed (error already sent)
     */
    std::optional<json> parseJsonBody(HttpContext& ctx);
    
    /**
     * @brief Helper: Validate required fields in JSON
     * @return true if all fields present, false otherwise (error already sent)
     */
    bool validateRequiredFields(HttpContext& ctx, const json& body, 
                               const std::vector<std::string>& fields);
    
    /**
     * @brief Helper: Get route parameter or return error
     * @return Parameter value or std::nullopt if not found (error already sent)
     */
    std::optional<std::string> getRouteParam(HttpContext& ctx, const std::string& name);

private:
    std::string baseRoute_;
    std::vector<std::shared_ptr<Route>> routes_;
    
    void addRoute(const std::string& method, const std::string& pattern, EndpointHandler handler);
};

} // namespace http
