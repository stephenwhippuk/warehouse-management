#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <memory>
#include <string>

namespace warehouse::services {
    class LocationService; // Forward declaration
}

namespace warehouse::controllers {

/**
 * @brief HTTP controller for location endpoints
 * 
 * Handles:
 * - GET /api/v1/locations - List all locations (with filters)
 * - GET /api/v1/locations/:id - Get location by ID
 * - GET /api/v1/warehouses/:warehouseId/locations - Get locations by warehouse
 * - POST /api/v1/locations - Create new location
 * - PUT /api/v1/locations/:id - Update location
 * - DELETE /api/v1/locations/:id - Delete location
 */
class LocationController : public Poco::Net::HTTPRequestHandler {
public:
    explicit LocationController(std::shared_ptr<services::LocationService> service);
    
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;

private:
    void handleGetAll(Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    
    void handleGetById(const std::string& id,
                      Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response);
    
    void handleGetByWarehouse(const std::string& warehouseId,
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

    std::shared_ptr<services::LocationService> service_;
};

} // namespace warehouse::controllers
