#pragma once

#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/NameValueCollection.h>
#include <cstdint>

namespace inventory {
namespace utils {

enum class AuthStatus {
    NotConfigured,
    MissingToken,
    InvalidToken,
    Authorized
};

class Auth {
public:
    // Service-to-service authentication using an API key.
    //
    // Configuration:
    // - SERVICE_API_KEY environment variable (highest priority)
    // - auth.serviceApiKey in config/application.json
    //
    // Request headers (any of these):
    // - X-Service-Api-Key: <key>
    // - Authorization: ApiKey <key>
    //
    // Returns:
    // - NotConfigured: no API key configured; auth is effectively disabled
    // - MissingToken: key configured but no credentials provided
    // - InvalidToken: provided credentials do not match configured key
    // - Authorized: credentials valid
    static AuthStatus authorizeServiceRequest(const Poco::Net::HTTPServerRequest& request);

    // Testable variant that works directly with header collection
    static AuthStatus authorizeServiceHeaders(const Poco::Net::NameValueCollection& headers);

    // Simple in-memory metrics (process-local, non-persistent)
    static std::uint64_t authorizedCount();
    static std::uint64_t missingTokenCount();
    static std::uint64_t invalidTokenCount();
};

} // namespace utils
} // namespace inventory
