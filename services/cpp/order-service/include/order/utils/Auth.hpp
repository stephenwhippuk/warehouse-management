#pragma once

#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <string>
#include <optional>

namespace order {
namespace utils {

class Auth {
public:
    /**
     * @brief Authorize service-to-service request
     * 
     * Checks for valid API key in X-Service-Api-Key header or Authorization: ApiKey header.
     * Sends 401/403 responses if authentication fails.
     * 
     * @param request HTTP request
     * @param response HTTP response
     * @return true if authorized, false if unauthorized (response already sent)
     */
    static bool authorizeServiceRequest(
        Poco::Net::HTTPServerRequest& request,
        Poco::Net::HTTPServerResponse& response
    );

    /**
     * @brief Extract API key from request headers
     * 
     * Looks for X-Service-Api-Key header or Authorization: ApiKey <token> header
     * 
     * @param request HTTP request
     * @return API key if found, std::nullopt otherwise
     */
    static std::optional<std::string> extractApiKey(
        Poco::Net::HTTPServerRequest& request
    );

    /**
     * @brief Validate API key against configured key
     * 
     * @param apiKey API key to validate
     * @return true if valid, false otherwise
     */
    static bool validateApiKey(const std::string& apiKey);

    /**
     * @brief Get configured service API key
     * 
     * Reads from SERVICE_API_KEY env var or config file
     * 
     * @return configured API key (empty if not configured)
     */
    static std::string getConfiguredApiKey();

private:
    static void sendUnauthorized(Poco::Net::HTTPServerResponse& response, const std::string& message);
    static void sendForbidden(Poco::Net::HTTPServerResponse& response, const std::string& message);
};

} // namespace utils
} // namespace order
