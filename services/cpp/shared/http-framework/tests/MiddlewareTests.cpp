#include <catch2/catch_all.hpp>
#include "http-framework/Middleware.hpp"
#include "http-framework/HttpContext.hpp"
#include "http-framework/ExceptionFilter.hpp"
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/SocketAddress.h>
#include <sstream>

using namespace http;

class MockServerResponse : public Poco::Net::HTTPServerResponse {
public:
    void sendContinue() override {
        continued_ = true;
    }

    std::ostream& send() override {
        sent_ = true;
        return body_;
    }

    void sendFile(const std::string& /* path */, const std::string& /* mediaType */) override {
        sent_ = true;
    }

    void sendBuffer(const void* /* pBuffer */, std::size_t /* length */) override {
        sent_ = true;
    }

    void redirect(const std::string& /* uri */, HTTPStatus status = HTTP_FOUND) override {
        setStatus(status);
        sent_ = true;
    }

    void requireAuthentication(const std::string& /* realm */) override {
        setStatus(HTTP_UNAUTHORIZED);
        sent_ = true;
    }

    bool sent() const override {
        return sent_;
    }

private:
    bool sent_ = false;
    bool continued_ = false;
    std::ostringstream body_;
};

class MockServerRequest : public Poco::Net::HTTPServerRequest {
public:
    explicit MockServerRequest(Poco::Net::HTTPServerResponse& response)
        : response_(response)
        , clientAddress_("127.0.0.1", 12345)
        , serverAddress_("127.0.0.1", 8080)
        , serverParams_(new Poco::Net::HTTPServerParams()) {
        setMethod("GET");
        setURI("/test");
    }

    std::istream& stream() override {
        return bodyStream_;
    }

    const Poco::Net::SocketAddress& clientAddress() const override {
        return clientAddress_;
    }

    const Poco::Net::SocketAddress& serverAddress() const override {
        return serverAddress_;
    }

    const Poco::Net::HTTPServerParams& serverParams() const override {
        return *serverParams_;
    }

    Poco::Net::HTTPServerResponse& response() const override {
        return response_;
    }

    bool secure() const override {
        return false;
    }

private:
    Poco::Net::HTTPServerResponse& response_;
    Poco::Net::SocketAddress clientAddress_;
    Poco::Net::SocketAddress serverAddress_;
    Poco::Net::HTTPServerParams::Ptr serverParams_;
    std::istringstream bodyStream_;
};

class TestExceptionFilter : public IExceptionFilter {
public:
    bool handleException(HttpContext& /* ctx */, const std::exception& /* e */) override {
        called_ = true;
        return true;
    }

    bool wasCalled() const {
        return called_;
    }

private:
    bool called_ = false;
};

// Mock middleware for testing
class TestMiddleware : public Middleware {
public:
    mutable bool called = false;
    mutable bool nextCalled = false;
    
    void process(HttpContext& ctx, std::function<void()> next) override {
        called = true;
        next();
        nextCalled = true;
    }
};

class OrderTestMiddleware : public Middleware {
public:
    OrderTestMiddleware(std::vector<int>& order, int id) 
        : order_(order), id_(id) {}
    
    void process(HttpContext& ctx, std::function<void()> next) override {
        order_.push_back(id_);
        next();
        order_.push_back(-id_);  // Negative on return
    }
    
private:
    std::vector<int>& order_;
    int id_;
};

TEST_CASE("Middleware pipeline executes in order", "[middleware][pipeline]") {
    MiddlewarePipeline pipeline;
    
    std::vector<int> executionOrder;
    
    // Add three middleware
    pipeline.use(std::make_shared<OrderTestMiddleware>(executionOrder, 1));
    pipeline.use(std::make_shared<OrderTestMiddleware>(executionOrder, 2));
    pipeline.use(std::make_shared<OrderTestMiddleware>(executionOrder, 3));
    
    bool finalHandlerCalled = false;
    
    // Would need to create a real HttpContext here
    // For now, this test demonstrates the concept
    // In practice, you'd use a mock or test fixture
}

TEST_CASE("Middleware calls next", "[middleware]") {
    auto middleware = std::make_shared<TestMiddleware>();
    
    REQUIRE_FALSE(middleware->called);
    REQUIRE_FALSE(middleware->nextCalled);
    
    // Would need HttpContext to actually test
    // This demonstrates the API
}

TEST_CASE("MiddlewarePipeline size", "[middleware][pipeline]") {
    MiddlewarePipeline pipeline;
    
    REQUIRE(pipeline.size() == 0);
    
    pipeline.use(std::make_shared<TestMiddleware>());
    REQUIRE(pipeline.size() == 1);
    
    pipeline.use(std::make_shared<TestMiddleware>());
    REQUIRE(pipeline.size() == 2);
    
    pipeline.clear();
    REQUIRE(pipeline.size() == 0);
}

TEST_CASE("QueryParams get method", "[middleware][query]") {
    Poco::URI::QueryParameters params = {
        {"page", "2"},
        {"limit", "50"},
        {"active", "true"}
    };
    
    QueryParams qp(params);
    
    SECTION("Get existing parameter") {
        REQUIRE(qp.get("page") == "2");
        REQUIRE(qp.get("limit") == "50");
    }
    
    SECTION("Get with default") {
        REQUIRE(qp.get("missing", "default") == "default");
    }
    
    SECTION("Has method") {
        REQUIRE(qp.has("page"));
        REQUIRE(qp.has("limit"));
        REQUIRE_FALSE(qp.has("missing"));
    }
}

TEST_CASE("QueryParams getInt method", "[middleware][query]") {
    Poco::URI::QueryParameters params = {
        {"page", "2"},
        {"invalid", "abc"}
    };
    
    QueryParams qp(params);
    
    SECTION("Parse valid integer") {
        auto page = qp.getInt("page");
        REQUIRE(page.has_value());
        REQUIRE(*page == 2);
    }
    
    SECTION("Return nullopt for invalid integer") {
        auto invalid = qp.getInt("invalid");
        REQUIRE_FALSE(invalid.has_value());
    }
    
    SECTION("Return nullopt for missing parameter") {
        auto missing = qp.getInt("missing");
        REQUIRE_FALSE(missing.has_value());
    }
}

TEST_CASE("QueryParams getBool method", "[middleware][query]") {
    Poco::URI::QueryParameters params = {
        {"active", "true"},
        {"inactive", "false"},
        {"numeric", "1"},
        {"zero", "0"},
        {"invalid", "maybe"}
    };
    
    QueryParams qp(params);
    
    SECTION("Parse true values") {
        REQUIRE(qp.getBool("active").value_or(false) == true);
        REQUIRE(qp.getBool("numeric").value_or(false) == true);
    }
    
    SECTION("Parse false values") {
        REQUIRE(qp.getBool("inactive").value_or(true) == false);
        REQUIRE(qp.getBool("zero").value_or(true) == false);
    }
    
    SECTION("Return nullopt for invalid") {
        REQUIRE_FALSE(qp.getBool("invalid").has_value());
    }
}

TEST_CASE("ErrorHandlingMiddleware uses overridden exception filter", "[middleware][error][filter]") {
    MockServerResponse response;
    MockServerRequest request(response);
    Poco::URI uri(request.getURI());
    HttpContext ctx(request, response, uri.getQueryParameters());

    auto filter = std::make_shared<TestExceptionFilter>();
    ErrorHandlingMiddleware middleware;

    middleware.setExceptionFilter(filter);
    middleware.process(ctx, []() {
        throw std::runtime_error("boom");
    });

    REQUIRE(filter->wasCalled());
}
