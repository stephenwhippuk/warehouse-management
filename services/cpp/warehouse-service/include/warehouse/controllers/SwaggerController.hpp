#ifndef WAREHOUSE_CONTROLLERS_SWAGGERCONTROLLER_HPP
#define WAREHOUSE_CONTROLLERS_SWAGGERCONTROLLER_HPP

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace warehouse {
namespace controllers {

/**
 * @brief Controller for serving OpenAPI/Swagger documentation
 */
class SwaggerController : public Poco::Net::HTTPRequestHandler {
public:
    SwaggerController() = default;

    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;

private:
    json generateSpecification();
    void addSchemas(json& spec);
    void addEndpoints(json& spec);
    void sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                         const std::string& jsonContent,
                         int statusCode = 200);
};

} // namespace controllers
} // namespace warehouse

#endif // WAREHOUSE_CONTROLLERS_SWAGGERCONTROLLER_HPP
