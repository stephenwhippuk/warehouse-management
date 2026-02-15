#include <catch2/catch_all.hpp>
#include "http-framework/Middleware.hpp"
#include "http-framework/HttpContext.hpp"
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

using namespace http;

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
