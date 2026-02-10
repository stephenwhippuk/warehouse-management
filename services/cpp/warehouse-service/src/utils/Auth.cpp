#include "warehouse/utils/Auth.hpp"
#include "warehouse/utils/Config.hpp"
#include "warehouse/utils/Logger.hpp"

namespace warehouse {
namespace utils {

namespace {

std::atomic<std::uint64_t> g_authorizedCount{0};
std::atomic<std::uint64_t> g_missingTokenCount{0};
std::atomic<std::uint64_t> g_invalidTokenCount{0};

std::string getConfiguredApiKey() {
    // Environment variable takes precedence
    std::string fromEnv = Config::getEnv("SERVICE_API_KEY", "");
    if (!fromEnv.empty()) {
        return fromEnv;
    }

    // Fallback to config file value
    return Config::getString("auth.serviceApiKey", "");
}

} // namespace

AuthStatus Auth::authorizeServiceHeaders(const Poco::Net::NameValueCollection& headers) {
    const std::string apiKey = getConfiguredApiKey();
    if (apiKey.empty()) {
        // Auth not configured; treat as disabled
        Logger::debug("Service API key not configured; skipping auth");
        return AuthStatus::NotConfigured;
    }

    std::string token;

    // Preferred header: X-Service-Api-Key
    if (headers.has("X-Service-Api-Key")) {
        token = headers.get("X-Service-Api-Key");
    } else if (headers.has("Authorization")) {
        // Fallback: Authorization: ApiKey <token>
        const std::string authHeader = headers.get("Authorization");
        const std::string prefix = "ApiKey ";
        if (authHeader.rfind(prefix, 0) == 0 && authHeader.size() > prefix.size()) {
            token = authHeader.substr(prefix.size());
        }
    }

    if (token.empty()) {
        ++g_missingTokenCount;
        Logger::warn("Missing service authentication token");
        return AuthStatus::MissingToken;
    }

    if (token != apiKey) {
        ++g_invalidTokenCount;
        Logger::warn("Invalid service authentication token");
        return AuthStatus::InvalidToken;
    }
    ++g_authorizedCount;
    Logger::debug("Service authentication successful");
    return AuthStatus::Authorized;
}

AuthStatus Auth::authorizeServiceRequest(const Poco::Net::HTTPServerRequest& request) {
    const auto& headers = static_cast<const Poco::Net::NameValueCollection&>(request);
    return authorizeServiceHeaders(headers);
}

std::uint64_t Auth::authorizedCount() {
    return g_authorizedCount.load(std::memory_order_relaxed);
}

std::uint64_t Auth::missingTokenCount() {
    return g_missingTokenCount.load(std::memory_order_relaxed);
}

std::uint64_t Auth::invalidTokenCount() {
    return g_invalidTokenCount.load(std::memory_order_relaxed);
}

} // namespace utils
} // namespace warehouse
