#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <memory>
#include <string>

namespace warehouse::services {
    class WarehouseService; // Forward declaration
}

namespace warehouse::controllers {

/**
 * @brief HTTP controller for warehouse endpoints
 * 
 * Handles:
 * - GET /api/v1/warehouses - List all warehouses
 * - GET /api/v1/warehouses/:id - Get warehouse by ID
 * - POST /api/v1/warehouses - Create new warehouse
 * - PUT /api/v1/warehouses/:id - Update warehouse
 * - DELETE /api/v1/warehouses/:id - Delete warehouse
 */
class WarehouseController : public Poco::Net::HTTPRequestHandler {
public:
    explicit WarehouseController(std::shared_ptr<services::WarehouseService> service);
    
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
    
    void handleDelete(const std::string& id,
                     Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    
    void sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                         int status,
                         const std::string& body);
    
    void sendErrorResponse(Poco::Net::HTTPServerResponse& response,
                          int status,
                          const std::string& message);
    
    std::string extractIdFromPath(const std::string& path);

    std::shared_ptr<services::WarehouseService> service_;
};

} // namespace warehouse::controllers
