#include "product/utils/Auth.hpp"
#include "product/utils/Logger.hpp"

namespace product::utils {

std::string Auth::serviceApiKey_;

void Auth::setServiceApiKey(const std::string& key) {
    serviceApiKey_ = key;
}

bool Auth::authorizeServiceRequest(Poco::Net::HTTPServerRequest& request,
                                   Poco::Net::HTTPServerResponse& response) {
    if (serviceApiKey_.empty()) {
        // NOTE: For product-service, an empty serviceApiKey_ is treated as
        // "auth disabled" and all requests are allowed. Other services
        // (e.g., warehouse-service) may instead surface a distinct
        // "not configured" status to callers. This deviation is intentional
        // to keep product-service accessible when no SERVICE_API_KEY or
        // auth.serviceApiKey is configured.
        Logger::warn("Auth::authorizeServiceRequest: SERVICE_API_KEY not configured; "
                     "treating request as authorized (auth disabled for product-service).");
        return true;
    }
    
    std::string apiKey = extractApiKey(request);
    
    if (apiKey.empty()) {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
        response.setContentType("application/json");
        response.send() << "{\"error\": \"Unauthorized\", \"message\": \"Missing API key\"}";
        return false;
    }
    
    if (apiKey != serviceApiKey_) {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_FORBIDDEN);
        response.setContentType("application/json");
        response.send() << "{\"error\": \"Forbidden\", \"message\": \"Invalid API key\"}";
        return false;
    }
    
    return true;
}

std::string Auth::extractApiKey(Poco::Net::HTTPServerRequest& request) {
    // Check X-Service-Api-Key header
    try {
        if (request.has("X-Service-Api-Key")) {
            return request.get("X-Service-Api-Key");
        }
    } catch (...) {}
    
    // Check Authorization: ApiKey header
    try {
        std::string auth = request.get("Authorization", "");
        if (auth.find("ApiKey ") == 0) {
            return auth.substr(7);  // "ApiKey " is 7 chars
        }
    } catch (...) {}
    
    return "";
}

}  // namespace product::utils
