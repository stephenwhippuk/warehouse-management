#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include "product/services/ProductService.hpp"
#include <memory>
#include <string>

namespace product::controllers {

/**
 * @brief Product API endpoints handler
 * 
 * Handles:
 * - GET /api/v1/products (list)
 * - GET /api/v1/products/{id} (get by id)
 * - POST /api/v1/products (create)
 * - PUT /api/v1/products/{id} (update)
 * - DELETE /api/v1/products/{id} (delete)
 */
class ProductController : public Poco::Net::HTTPRequestHandler {
public:
    explicit ProductController(std::shared_ptr<services::ProductService> service);
    
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;

private:
    std::shared_ptr<services::ProductService> service_;
    
    // Handler methods for different operations
    void handleGetAll(int page, int pageSize, Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    void handleGetById(const std::string& id, Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response);
    void handleCreate(Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    void handleUpdate(const std::string& id, Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    void handleDelete(const std::string& id, Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    
    // Helper methods
    void sendJsonResponse(Poco::Net::HTTPServerResponse& response, const std::string& json,
                         int status = 200);
    void sendErrorResponse(Poco::Net::HTTPServerResponse& response, const std::string& error,
                          const std::string& message, int status);
    std::string extractPathParameter(const std::string& uri, const std::string& paramName);
};

}  // namespace product::controllers
