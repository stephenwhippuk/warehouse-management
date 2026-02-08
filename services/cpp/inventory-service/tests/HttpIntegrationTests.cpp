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

    const char* enabledEnv = std::getenv("INVENTORY_HTTP_INTEGRATION");
    cfg.enabled = (enabledEnv != nullptr && std::string(enabledEnv) == "1");

    const char* hostEnv = std::getenv("INVENTORY_HTTP_HOST");
    cfg.host = hostEnv ? std::string(hostEnv) : std::string("localhost");

    const char* portEnv = std::getenv("INVENTORY_HTTP_PORT");
    if (portEnv) {
        cfg.port = static_cast<unsigned short>(std::stoi(portEnv));
    } else {
        cfg.port = 8080;
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

    // Very small chance of collision, more than sufficient for tests
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
        WARN("INVENTORY_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    REQUIRE_FALSE(cfg.host.empty());
    REQUIRE(cfg.port > 0);

    auto payload = getJsonWithRetry(cfg.host, cfg.port, "/health");

    REQUIRE(payload.contains("status"));
    REQUIRE(payload.contains("service"));
    REQUIRE(payload["status"].get<std::string>() == "ok");
    REQUIRE(payload["service"].get<std::string>() == "inventory-service");
    REQUIRE(payload.contains("auth"));
}

TEST_CASE("Swagger endpoint serves OpenAPI spec", "[http][integration][swagger]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("INVENTORY_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    auto spec = getJsonWithRetry(cfg.host, cfg.port, "/api/swagger.json");

    REQUIRE(spec.contains("openapi"));
    REQUIRE(spec.contains("paths"));
    auto& paths = spec["paths"];
    REQUIRE(paths.is_object());

    // Core inventory collection endpoint should be documented
    REQUIRE(paths.contains("/api/v1/inventory"));
}

TEST_CASE("Inventory list endpoint responds with JSON array", "[http][integration][inventory]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("INVENTORY_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    auto body = doJsonRequest(cfg,
                              Poco::Net::HTTPRequest::HTTP_GET,
                              "/api/v1/inventory",
                              nullptr,
                              Poco::Net::HTTPResponse::HTTP_OK);
    REQUIRE(body.is_array());
}

TEST_CASE("Inventory CRUD and stock operations work over HTTP", "[http][integration][inventory][crud]") {
    auto cfg = getHttpConfig();
    if (!cfg.enabled) {
        WARN("INVENTORY_HTTP_INTEGRATION not set; skipping HTTP integration tests");
        return;
    }

    // Ensure service is up before hitting authenticated endpoints
    (void)getJsonWithRetry(cfg.host, cfg.port, "/health");

    const std::string id = generateRandomUuid();
    // Use dedicated IDs that do not overlap with repository test fixtures
    const std::string productId  = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
    const std::string warehouseId= "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";
    const std::string locationId = "cccccccc-cccc-cccc-cccc-cccccccccccc";

    // Create inventory record
    json createBody = {
        {"id", id},
        {"productId", productId},
        {"warehouseId", warehouseId},
        {"locationId", locationId},
        {"quantity", 10},
        {"availableQuantity", 10},
        {"reservedQuantity", 0},
        {"allocatedQuantity", 0},
        {"status", "available"},
        {"qualityStatus", "not_tested"}
    };

    auto created = doJsonRequest(cfg,
                                 Poco::Net::HTTPRequest::HTTP_POST,
                                 "/api/v1/inventory",
                                 &createBody,
                                 Poco::Net::HTTPResponse::HTTP_CREATED);

    REQUIRE(created["id"].get<std::string>() == id);

    // GET by ID
    auto fetched = doJsonRequest(cfg,
                                 Poco::Net::HTTPRequest::HTTP_GET,
                                 "/api/v1/inventory/" + id,
                                 nullptr,
                                 Poco::Net::HTTPResponse::HTTP_OK);

    REQUIRE(fetched["id"].get<std::string>() == id);
    REQUIRE(fetched["quantity"].get<int>() == 10);

    // List endpoints should respond with arrays
    auto byProduct = doJsonRequest(cfg,
                                   Poco::Net::HTTPRequest::HTTP_GET,
                                   "/api/v1/inventory/product/" + productId,
                                   nullptr,
                                   Poco::Net::HTTPResponse::HTTP_OK);
    REQUIRE(byProduct.is_array());

    auto byWarehouse = doJsonRequest(cfg,
                                     Poco::Net::HTTPRequest::HTTP_GET,
                                     "/api/v1/inventory/warehouse/" + warehouseId,
                                     nullptr,
                                     Poco::Net::HTTPResponse::HTTP_OK);
    REQUIRE(byWarehouse.is_array());

    auto byLocation = doJsonRequest(cfg,
                                    Poco::Net::HTTPRequest::HTTP_GET,
                                    "/api/v1/inventory/location/" + locationId,
                                    nullptr,
                                    Poco::Net::HTTPResponse::HTTP_OK);
    REQUIRE(byLocation.is_array());

    auto lowStock = doJsonRequest(cfg,
                                  Poco::Net::HTTPRequest::HTTP_GET,
                                  "/api/v1/inventory/low-stock?threshold=1000",
                                  nullptr,
                                  Poco::Net::HTTPResponse::HTTP_OK);
    REQUIRE(lowStock.is_array());

    auto expired = doJsonRequest(cfg,
                                 Poco::Net::HTTPRequest::HTTP_GET,
                                 "/api/v1/inventory/expired",
                                 nullptr,
                                 Poco::Net::HTTPResponse::HTTP_OK);
    REQUIRE(expired.is_array());

    // Reserve 3 units
    json reserveBody = {
        {"quantity", 3}
    };
    auto afterReserve = doJsonRequest(cfg,
                                      Poco::Net::HTTPRequest::HTTP_POST,
                                      "/api/v1/inventory/" + id + "/reserve",
                                      &reserveBody,
                                      Poco::Net::HTTPResponse::HTTP_OK);

    REQUIRE(afterReserve["reservedQuantity"].get<int>() == 3);
    REQUIRE(afterReserve["availableQuantity"].get<int>() == 7);

    // Release 1 unit
    json releaseBody = {
        {"quantity", 1}
    };
    auto afterRelease = doJsonRequest(cfg,
                                      Poco::Net::HTTPRequest::HTTP_POST,
                                      "/api/v1/inventory/" + id + "/release",
                                      &releaseBody,
                                      Poco::Net::HTTPResponse::HTTP_OK);

    REQUIRE(afterRelease["reservedQuantity"].get<int>() == 2);
    REQUIRE(afterRelease["availableQuantity"].get<int>() == 8);

    // Allocate 1 unit
    json allocateBody = {
        {"quantity", 1}
    };
    auto afterAllocate = doJsonRequest(cfg,
                                       Poco::Net::HTTPRequest::HTTP_POST,
                                       "/api/v1/inventory/" + id + "/allocate",
                                       &allocateBody,
                                       Poco::Net::HTTPResponse::HTTP_OK);

    REQUIRE(afterAllocate["reservedQuantity"].get<int>() == 1);
    REQUIRE(afterAllocate["allocatedQuantity"].get<int>() == 1);
    REQUIRE(afterAllocate["availableQuantity"].get<int>() == 8);

    // Deallocate 1 unit
    json deallocateBody = {
        {"quantity", 1}
    };
    auto afterDeallocate = doJsonRequest(cfg,
                                         Poco::Net::HTTPRequest::HTTP_POST,
                                         "/api/v1/inventory/" + id + "/deallocate",
                                         &deallocateBody,
                                         Poco::Net::HTTPResponse::HTTP_OK);

    REQUIRE(afterDeallocate["reservedQuantity"].get<int>() == 1);
    REQUIRE(afterDeallocate["allocatedQuantity"].get<int>() == 0);
    REQUIRE(afterDeallocate["availableQuantity"].get<int>() == 9);

    // Adjust +1 unit
    json adjustBody = {
        {"quantityChange", 1},
        {"reason", "http-integration-test"}
    };
    auto afterAdjust = doJsonRequest(cfg,
                                     Poco::Net::HTTPRequest::HTTP_POST,
                                     "/api/v1/inventory/" + id + "/adjust",
                                     &adjustBody,
                                     Poco::Net::HTTPResponse::HTTP_OK);

    REQUIRE(afterAdjust["quantity"].get<int>() == 11);
    REQUIRE(afterAdjust["reservedQuantity"].get<int>() == 1);
    REQUIRE(afterAdjust["allocatedQuantity"].get<int>() == 0);
    REQUIRE(afterAdjust["availableQuantity"].get<int>() == 10);

    // Release remaining reserved quantity so the record can be deleted
    json finalReleaseBody = {
        {"quantity", 1}
    };
    auto afterFinalRelease = doJsonRequest(cfg,
                                           Poco::Net::HTTPRequest::HTTP_POST,
                                           "/api/v1/inventory/" + id + "/release",
                                           &finalReleaseBody,
                                           Poco::Net::HTTPResponse::HTTP_OK);

    REQUIRE(afterFinalRelease["reservedQuantity"].get<int>() == 0);
    REQUIRE(afterFinalRelease["availableQuantity"].get<int>() == 11);

    // Delete inventory record
    (void)doJsonRequest(cfg,
                        Poco::Net::HTTPRequest::HTTP_DELETE,
                        "/api/v1/inventory/" + id,
                        nullptr,
                        Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
}
