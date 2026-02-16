#pragma once

#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>
#include <map>
#include <string>
#include <any>
#include <optional>
#include <sstream>
#include <memory>
#include <nlohmann/json.hpp>
#include "IServiceScope.hpp"
#include "IServiceProvider.hpp"

using json = nlohmann::json;

namespace http {

/**
 * @brief Helper class for accessing query parameters
 */
class QueryParams {
public:
    explicit QueryParams(const Poco::URI::QueryParameters& params);
    
    /**
     * @brief Get query parameter value
     * @param key Parameter name
     * @param defaultValue Default value if not found
     * @return Parameter value or default
     */
    std::string get(const std::string& key, const std::string& defaultValue = "") const;
    
    /**
     * @brief Check if parameter exists
     */
    bool has(const std::string& key) const;
    
    /**
     * @brief Get all parameters as map
     */
    const std::map<std::string, std::string>& all() const;
    
    /**
     * @brief Get parameter as integer
     */
    std::optional<int> getInt(const std::string& key) const;
    
    /**
     * @brief Get parameter as boolean
     */
    std::optional<bool> getBool(const std::string& key) const;

private:
    std::map<std::string, std::string> params_;
};

/**
 * @brief HTTP request/response context with route and query parameters
 * 
 * Encapsulates all request/response data and provides helpers for common operations.
 * Passed through middleware pipeline and to endpoint handlers.
 */
struct HttpContext {
    // Original Poco request/response
    Poco::Net::HTTPServerRequest& request;
    Poco::Net::HTTPServerResponse& response;
    
    // Extracted parameters
    std::map<std::string, std::string> routeParams;  // e.g., {id} from /inventory/{id}
    QueryParams queryParams;                          // e.g., ?page=1&limit=10
    
    // Request-scoped data (shared between middleware and handlers)
    std::map<std::string, std::any> items;
    
    // Parsed URI
    Poco::URI uri;
    
    // Service scope for this request (created by ServiceScopeMiddleware)
    std::shared_ptr<IServiceScope> serviceScope;
    
    /**
     * @brief Constructor
     */
    HttpContext(Poco::Net::HTTPServerRequest& req, 
                Poco::Net::HTTPServerResponse& res,
                const Poco::URI::QueryParameters& queryParams);
    
    /**
     * @brief Read request body as string
     */
    std::string getBodyAsString();
    
    /**
     * @brief Parse request body as JSON
     * @throws json::parse_error if body is not valid JSON
     */
    json getBodyAsJson();
    
    /**
     * @brief Send JSON response
     */
    void sendJson(const std::string& jsonStr, 
                  Poco::Net::HTTPResponse::HTTPStatus status = Poco::Net::HTTPResponse::HTTP_OK);
    
    /**
     * @brief Send JSON response from nlohmann::json object
     */
    void sendJson(const json& jsonObj,
                  Poco::Net::HTTPResponse::HTTPStatus status = Poco::Net::HTTPResponse::HTTP_OK);
    
    /**
     * @brief Send error response
     */
    void sendError(const std::string& message,
                   Poco::Net::HTTPResponse::HTTPStatus status = Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR,
                   const std::string& requestId = "");
    
    /**
     * @brief Send not found response
     */
    void sendNotFound(const std::string& message = "Resource not found");
    
    /**
     * @brief Send created response with location header
     */
    void sendCreated(const std::string& location, const std::string& jsonStr);
    
    /**
     * @brief Send no content response
     */
    void sendNoContent();
    
    /**
     * @brief Get request path (without query string)
     */
    std::string getPath() const;
    
    /**
     * @brief Get HTTP method
     */
    std::string getMethod() const;
    
    /**
     * @brief Set response status
     */
    void setStatus(Poco::Net::HTTPResponse::HTTPStatus status);
    
    /**
     * @brief Add response header
     */
    void setHeader(const std::string& name, const std::string& value);
    
    /**
     * @brief Get request header
     */
    std::string getHeader(const std::string& name, const std::string& defaultValue = "") const;
    
    /**
     * @brief Check if request has header
     */
    bool hasHeader(const std::string& name) const;
    
    /**
     * @brief Set the service scope for this request
     * This is called by ServiceScopeMiddleware
     */
    void setServiceScope(std::shared_ptr<IServiceScope> scope);
    
    /**
     * @brief Get the service scope for this request
     * Returns the scope created by ServiceScopeMiddleware
     * @return Shared pointer to IServiceScope (may be nullptr if middleware not used)
     */
    std::shared_ptr<IServiceScope> getServiceScope() const;
    
    /**
     * @brief Get a service from the request scope (convenience method)
     * @tparam T Service interface type
     * @return Shared pointer to the service
     * @throws std::runtime_error if service not found or scope not set
     */
    template<typename T>
    std::shared_ptr<T> getService() {
        if (!serviceScope) {
            throw std::runtime_error("Service scope not set. Ensure ServiceScopeMiddleware is added to the pipeline.");
        }
        return serviceScope->getServiceProvider().getService<T>();
    }

private:
    bool bodyRead_ = false;
    std::string bodyCache_;
};

} // namespace http
