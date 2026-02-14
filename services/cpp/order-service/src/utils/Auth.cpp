#include "order/utils/Auth.hpp"
#include "order/utils/Config.hpp"
#include "order/utils/Logger.hpp"
#include <Poco/Net/HTTPResponse.h>
#include <cstdlib>
#include <algorithm>

namespace order {
namespace utils {

bool Auth::authorizeServiceRequest(
    Poco::Net::HTTPServerRequest& request,
    Poco::Net::HTTPServerResponse& response
) {
    auto apiKey = extractApiKey(request);
    
    if (!apiKey) {
        Logger::warn("Missing API key in request to {}", request.getURI());
        sendUnauthorized(response, "Missing API key");
        return false;
    }
    
    if (!validateApiKey(*apiKey)) {
        Logger::warn("Invalid API key in request to {}", request.getURI());
        sendForbidden(response, "Invalid API key");
        return false;
    }
    
    return true;
}

std::optional<std::string> Auth::extractApiKey(Poco::Net::HTTPServerRequest& request) {
    // Check X-Service-Api-Key header
    if (request.has("X-Service-Api-Key")) {
        return request.get("X-Service-Api-Key");
    }
    
    // Check Authorization header
    if (request.has("Authorization")) {
        std::string authHeader = request.get("Authorization");
        
        // Check for "ApiKey <token>" format
        if (authHeader.substr(0, 7) == "ApiKey ") {
            return authHeader.substr(7);
        }
    }
    
    return std::nullopt;
}

bool Auth::validateApiKey(const std::string& apiKey) {
    std::string configuredKey = getConfiguredApiKey();
    
    if (configuredKey.empty()) {
        Logger::warn("No API key configured - authentication disabled");
        return true; // Allow requests if no key is configured
    }
    
    return apiKey == configuredKey;
}

std::string Auth::getConfiguredApiKey() {
    // Check environment variable first
    const char* envKey = std::getenv("SERVICE_API_KEY");
    if (envKey != nullptr && std::string(envKey).length() > 0) {
        return std::string(envKey);
    }
    
    // Fall back to config file
    return Config::getString("auth.serviceApiKey", "");
}

void Auth::sendUnauthorized(Poco::Net::HTTPServerResponse& response, const std::string& message) {
    response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
    response.setContentType("application/json");
    
    std::ostream& out = response.send();
    out << R"({"error":")" << message << R"(","status":401})";
}

void Auth::sendForbidden(Poco::Net::HTTPServerResponse& response, const std::string& message) {
    response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
    response.setContentType("application/json");
    
    std::ostream& out = response.send();
    out << R"({"error":")" << message << R"(","status":403})";
}

} // namespace utils
} // namespace order
