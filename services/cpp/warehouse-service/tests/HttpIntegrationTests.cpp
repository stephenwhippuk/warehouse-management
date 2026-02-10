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

    const char* enabledEnv = std::getenv("WAREHOUSE_HTTP_INTEGRATION");
    cfg.enabled = (enabledEnv != nullptr && std::string(enabledEnv) == "1");

    const char* hostEnv = std::getenv("WAREHOUSE_HTTP_HOST");
    cfg.host = hostEnv ? std::string(hostEnv) : std::string("localhost");

    const char* portEnv = std::getenv("WAREHOUSE_HTTP_PORT");
    if (portEnv) {
        cfg.port = static_cast<unsigned short>(std::stoi(portEnv));
    } else {
        cfg.port = 8081;
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
        WARN("WAREHOUSE_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    REQUIRE_FALSE(cfg.host.empty());
    REQUIRE(cfg.port > 0);

    auto payload = getJsonWithRetry(cfg.host, cfg.port, "/health");

    REQUIRE(payload.contains("status"));
    REQUIRE(payload.contains("service"));
    REQUIRE(payload["status"].get<std::string>() == "ok");
    REQUIRE(payload["service"].get<std::string>() == "warehouse-service");
    REQUIRE(payload.contains("auth"));
}

TEST_CASE("Swagger endpoint serves OpenAPI spec", "[http][integration][swagger]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("WAREHOUSE_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    auto spec = getJsonWithRetry(cfg.host, cfg.port, "/api/swagger.json");

    REQUIRE(spec.contains("openapi"));
    REQUIRE(spec.contains("paths"));
    auto& paths = spec["paths"];
    REQUIRE(paths.is_object());

    // Core warehouse endpoints should be documented
    REQUIRE(paths.contains("/api/v1/warehouses"));
    REQUIRE(paths.contains("/api/v1/locations"));
}

TEST_CASE("Claims endpoint responds with service claims", "[http][integration][claims]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("WAREHOUSE_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    // Ensure service is up
    (void)getJsonWithRetry(cfg.host, cfg.port, "/health");

    auto claims = doJsonRequest(cfg,
                                Poco::Net::HTTPRequest::HTTP_GET,
                                "/api/v1/claims",
                                nullptr,
                                Poco::Net::HTTPResponse::HTTP_OK);

    REQUIRE(claims.contains("service"));
    REQUIRE(claims["service"].get<std::string>() == "warehouse-service");
    REQUIRE(claims.contains("fulfilments"));
    REQUIRE(claims["fulfilments"].is_array());
}

TEST_CASE("Warehouse list endpoint responds with empty array when no data", "[http][integration][warehouse]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("WAREHOUSE_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    // Ensure service is up before hitting authenticated endpoints
    (void)getJsonWithRetry(cfg.host, cfg.port, "/health");

    auto body = doJsonRequest(cfg,
                              Poco::Net::HTTPRequest::HTTP_GET,
                              "/api/v1/warehouses",
                              nullptr,
                              Poco::Net::HTTPResponse::HTTP_OK);
    // Should accept array or object with items field
    REQUIRE((body.is_array() || body.contains("items")));
}

TEST_CASE("Location list endpoint responds with empty array when no data", "[http][integration][location]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("WAREHOUSE_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    // Ensure service is up before hitting authenticated endpoints
    (void)getJsonWithRetry(cfg.host, cfg.port, "/health");

    auto body = doJsonRequest(cfg,
                              Poco::Net::HTTPRequest::HTTP_GET,
                              "/api/v1/locations",
                              nullptr,
                              Poco::Net::HTTPResponse::HTTP_OK);
    // Should accept array or object with items field
    REQUIRE((body.is_array() || body.contains("items")));
}
