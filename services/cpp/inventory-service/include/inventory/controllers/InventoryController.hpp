#pragma once

#include "inventory/services/InventoryService.hpp"
#include <Poco/Net/HTTPRequestHandler.h>
#include <memory>

namespace inventory {
namespace controllers {

class InventoryController : public Poco::Net::HTTPRequestHandler {
public:
    explicit InventoryController(std::shared_ptr<services::InventoryService> service);
    
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;
    
private:
    std::shared_ptr<services::InventoryService> service_;
    
    void handleGetAll(Poco::Net::HTTPServerResponse& response);
    void handleGetById(const std::string& id, Poco::Net::HTTPServerResponse& response);
    void handleGetByProduct(const std::string& productId, Poco::Net::HTTPServerResponse& response);
    void handleGetByWarehouse(const std::string& warehouseId, Poco::Net::HTTPServerResponse& response);
    void handleGetByLocation(const std::string& locationId, Poco::Net::HTTPServerResponse& response);
    void handleGetLowStock(int threshold, Poco::Net::HTTPServerResponse& response);
    void handleGetExpired(Poco::Net::HTTPServerResponse& response);
    void handleCreate(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
    void handleUpdate(const std::string& id, Poco::Net::HTTPServerRequest& request, 
                     Poco::Net::HTTPServerResponse& response);
    void handleDelete(const std::string& id, Poco::Net::HTTPServerResponse& response);
    void handleReserve(const std::string& id, Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response);
    void handleRelease(const std::string& id, Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response);
    void handleAllocate(const std::string& id, Poco::Net::HTTPServerRequest& request,
                       Poco::Net::HTTPServerResponse& response);
    void handleDeallocate(const std::string& id, Poco::Net::HTTPServerRequest& request,
                         Poco::Net::HTTPServerResponse& response);
    void handleAdjust(const std::string& id, Poco::Net::HTTPServerRequest& request,
                     Poco::Net::HTTPServerResponse& response);
    
    void sendJsonResponse(Poco::Net::HTTPServerResponse& response, 
                         const std::string& json, 
                         Poco::Net::HTTPResponse::HTTPStatus status = Poco::Net::HTTPResponse::HTTP_OK);
    void sendErrorResponse(Poco::Net::HTTPServerResponse& response,
                          const std::string& message,
                          Poco::Net::HTTPResponse::HTTPStatus status);
};

} // namespace controllers
} // namespace inventory
