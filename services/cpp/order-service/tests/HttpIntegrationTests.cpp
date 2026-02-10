#include <catch2/catch_all.hpp>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Timespan.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <sstream>
#include <string>
#include <thread>
#include <random>

using nlohmann::json;

namespace {

struct HttpConfig {
    std::string host;
    unsigned short port;
    bool enabled;
};

HttpConfig getHttpConfig() {
    HttpConfig cfg{};

    const char* enabledEnv = std::getenv("ORDER_HTTP_INTEGRATION");
    cfg.enabled = (enabledEnv != nullptr && std::string(enabledEnv) == "1");

    const char* hostEnv = std::getenv("ORDER_HTTP_HOST");
    cfg.host = hostEnv ? std::string(hostEnv) : std::string("localhost");

    const char* portEnv = std::getenv("ORDER_HTTP_PORT");
    if (portEnv) {
        cfg.port = static_cast<unsigned short>(std::stoi(portEnv));
    } else {
        cfg.port = 8082;
    }

    return cfg;
}

std::string getServiceApiKey() {
    const char* keyEnv = std::getenv("SERVICE_API_KEY");
    return keyEnv ? std::string(keyEnv) : std::string();
}

std::string generateRandomUuid() {
    static thread_local std::mt19937 rng{std::random_device{}()};
    static const char* hex = "0123456789abcdef";

    auto genBlock = [&](int count) {
        std::string out;
        out.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) {
            out.push_back(hex[rng() % 16]);
        }
        return out;
    };

    return genBlock(8) + "-" + genBlock(4) + "-" + genBlock(4) + "-" +
           genBlock(4) + "-" + genBlock(12);
}

// Simple helper to GET a JSON endpoint, with basic retry to allow the service to start.
json getJsonWithRetry(const std::string& host,
                      unsigned short port,
                      const std::string& path,
                      Poco::Net::HTTPResponse::HTTPStatus expectedStatus = Poco::Net::HTTPResponse::HTTP_OK,
                      int maxAttempts = 30,
                      int sleepMillis = 1000) {
    using namespace Poco::Net;

    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        try {
            HTTPClientSession session(host, port);
            session.setTimeout(Poco::Timespan(5, 0));

            HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
            session.sendRequest(request);

            HTTPResponse response;
            std::istream& rs = session.receiveResponse(response);

            if (response.getStatus() != expectedStatus) {
                std::ostringstream msg;
                msg << "Unexpected HTTP status " << static_cast<int>(response.getStatus())
                    << " for " << path;
                throw std::runtime_error(msg.str());
            }

            std::ostringstream oss;
            oss << rs.rdbuf();
            const auto body = oss.str();

            if (body.empty()) {
                throw std::runtime_error("Empty response body for " + path);
            }

            return json::parse(body);
        } catch (const std::exception&) {
            if (attempt == maxAttempts) {
                throw;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMillis));
        }
    }

    throw std::runtime_error("Failed to call HTTP endpoint after retries");
}

json doJsonRequest(const HttpConfig& cfg,
                   const std::string& method,
                   const std::string& path,
                   const json* requestBody,
                   Poco::Net::HTTPResponse::HTTPStatus expectedStatus) {
    using namespace Poco::Net;

    HTTPClientSession session(cfg.host, cfg.port);
    session.setTimeout(Poco::Timespan(5, 0));

    HTTPRequest request(method, path, HTTPMessage::HTTP_1_1);

    const auto apiKey = getServiceApiKey();
    if (!apiKey.empty()) {
        request.set("X-Service-Api-Key", apiKey);
    }

    if (requestBody) {
        const auto bodyStr = requestBody->dump();
        request.setContentType("application/json");
        request.setContentLength(static_cast<int>(bodyStr.size()));
        std::ostream& os = session.sendRequest(request);
        os << bodyStr;
    } else {
        session.sendRequest(request);
    }

    HTTPResponse response;
    std::istream& rs = session.receiveResponse(response);

    if (response.getStatus() != expectedStatus) {
        std::ostringstream msg;
        msg << "Unexpected HTTP status " << static_cast<int>(response.getStatus())
            << " for " << path;
        throw std::runtime_error(msg.str());
    }

    std::ostringstream oss;
    oss << rs.rdbuf();
    const auto bodyStr = oss.str();

    if (bodyStr.empty()) {
        return json();
    }

    return json::parse(bodyStr);
}

} // namespace

TEST_CASE("Health endpoint is reachable over HTTP", "[http][integration][health]") {
    auto cfg = getHttpConfig();
    
    if (!cfg.enabled) {
        WARN("ORDER_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    REQUIRE_FALSE(cfg.host.empty());
    REQUIRE(cfg.port > 0);

    auto payload = getJsonWithRetry(cfg.host, cfg.port, "/health");

    REQUIRE(payload.contains("status"));
    REQUIRE(payload.contains("service"));
    REQUIRE(payload["status"].get<std::string>() == "healthy");
    REQUIRE(payload["service"].get<std::string>() == "order-service");
    REQUIRE(payload.contains("timestamp"));
    REQUIRE(payload.contains("version"));
}

TEST_CASE("Claims endpoint returns service contracts", "[http][integration][claims]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("ORDER_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    auto claims = doJsonRequest(cfg, "GET", "/api/v1/claims", nullptr, 
                                Poco::Net::HTTPResponse::HTTP_OK);

    REQUIRE(claims.contains("service"));
    REQUIRE(claims.contains("version"));
    REQUIRE(claims.contains("fulfilments"));
    REQUIRE(claims.contains("references"));
    
    REQUIRE(claims["service"].get<std::string>() == "order-service");
    REQUIRE(claims["fulfilments"].is_array());
    REQUIRE(claims["references"].is_array());
}

TEST_CASE("List orders returns empty array initially", "[http][integration][orders]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("ORDER_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    auto response = doJsonRequest(cfg, "GET", "/api/v1/orders", nullptr,
                                  Poco::Net::HTTPResponse::HTTP_OK);

    REQUIRE(response.contains("orders"));
    REQUIRE(response["orders"].is_array());
    REQUIRE(response.contains("total"));
    REQUIRE(response.contains("page"));
    REQUIRE(response.contains("pageSize"));
}

TEST_CASE("Authentication is required for order endpoints", "[http][integration][auth]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("ORDER_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    // Temporarily unset API key to test auth
    const auto apiKey = getServiceApiKey();
    if (apiKey.empty()) {
        WARN("No SERVICE_API_KEY set; skipping auth tests");
        return;
    }

    using namespace Poco::Net;

    // Try without API key - should fail
    HTTPClientSession session(cfg.host, cfg.port);
    session.setTimeout(Poco::Timespan(5, 0));

    HTTPRequest request(HTTPRequest::HTTP_GET, "/api/v1/orders", HTTPMessage::HTTP_1_1);
    // Deliberately not setting X-Service-Api-Key

    session.sendRequest(request);

    HTTPResponse response;
    std::istream& rs = session.receiveResponse(response);

    // Should get 401 Unauthorized or 403 Forbidden
    REQUIRE((response.getStatus() == HTTPResponse::HTTP_UNAUTHORIZED ||
             response.getStatus() == HTTPResponse::HTTP_FORBIDDEN));
}

TEST_CASE("Get non-existent order returns 501", "[http][integration][orders][notfound]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("ORDER_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    const auto randomId = generateRandomUuid();
    const auto path = "/api/v1/orders/" + randomId;

    // Since service is not fully implemented, expecting 501 Not Implemented
    // When implemented, this should return 404
    auto response = doJsonRequest(cfg, "GET", path, nullptr,
                                  Poco::Net::HTTPResponse::HTTP_NOT_IMPLEMENTED);

    REQUIRE(response.contains("error"));
}

TEST_CASE("Create order returns 501", "[http][integration][orders][create]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("ORDER_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    json createRequest = {
        {"orderNumber", "ORD-TEST-001"},
        {"customerId", "CUST-123"},
        {"warehouseId", generateRandomUuid()},
        {"priority", "normal"},
        {"lineItems", json::array()},
        {"shippingAddress", {
            {"name", "John Doe"},
            {"line1", "123 Main St"},
            {"city", "Springfield"},
            {"state", "IL"},
            {"postalCode", "62701"},
            {"country", "US"}
        }}
    };

    // Since service is not fully implemented, expecting 501 Not Implemented
    auto response = doJsonRequest(cfg, "POST", "/api/v1/orders", &createRequest,
                                  Poco::Net::HTTPResponse::HTTP_NOT_IMPLEMENTED);

    REQUIRE(response.contains("error"));
}
