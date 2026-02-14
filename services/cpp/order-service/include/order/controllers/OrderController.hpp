#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <memory>
#include <string>

namespace order::services {
    class OrderService; // Forward declaration
}

namespace order::controllers {

/**
 * @brief HTTP controller for order endpoints
 * 
 * Handles:
 * - GET /api/v1/orders - List orders
 * - GET /api/v1/orders/:id - Get order by ID
 * - POST /api/v1/orders - Create new order
 * - PUT /api/v1/orders/:id - Update order
 * - POST /api/v1/orders/:id/cancel - Cancel order
 */
class OrderController : public Poco::Net::HTTPRequestHandler {
public:
    explicit OrderController(std::shared_ptr<services::OrderService> service);
    
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;

private:
    void handleGetAll(Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    
    void handleGetById(const std::string& id,
                      Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response);
    
    void handleCreate(Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    
    void handleUpdate(const std::string& id,
                     Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    
    void handleCancel(const std::string& id,
                     Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    
    void sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                         int status,
                         const std::string& body);
    
    void sendErrorResponse(Poco::Net::HTTPServerResponse& response,
                          int status,
                          const std::string& message);
    
    std::string extractIdFromPath(const std::string& path);

    std::shared_ptr<services::OrderService> service_;
};

} // namespace order::controllers
