#ifndef INVENTORY_CONTROLLERS_SWAGGERCONTROLLER_HPP
#define INVENTORY_CONTROLLERS_SWAGGERCONTROLLER_HPP

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace inventory {
namespace controllers {

/**
 * @brief Controller for serving OpenAPI/Swagger documentation
 * 
 * Provides endpoints to access the API specification in JSON format.
 * The specification can be used by Swagger UI, Postman, or other
 * API documentation tools.
 */
class SwaggerController : public Poco::Net::HTTPRequestHandler {
public:
    SwaggerController() = default;

    /**
     * @brief Handle HTTP request for Swagger documentation
     * @param request HTTP request object
     * @param response HTTP response object
     */
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                      Poco::Net::HTTPServerResponse& response) override;

private:
    /**
     * @brief Generate the complete OpenAPI specification
     * @return JSON object with full API specification
     */
    json generateSpecification();

    /**
     * @brief Add inventory schemas to the specification
     * @param spec The OpenAPI specification object
     */
    void addSchemas(json& spec);

    /**
     * @brief Add inventory endpoints to the specification
     * @param spec The OpenAPI specification object
     */
    void addEndpoints(json& spec);

    /**
     * @brief Send JSON response
     * @param response HTTP response object
     * @param jsonContent JSON content to send
     * @param statusCode HTTP status code
     */
    void sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                         const std::string& jsonContent,
                         int statusCode = 200);
};

} // namespace controllers
} // namespace inventory

#endif // INVENTORY_CONTROLLERS_SWAGGERCONTROLLER_HPP
