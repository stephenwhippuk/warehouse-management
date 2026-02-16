#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "http-framework/Router.hpp"
#include "http-framework/ControllerBase.hpp"

using namespace http;

TEST_CASE("Router detects duplicate routes", "[router][duplicate][validation]") {
    Router router;
    
    auto handler1 = [](HttpContext& ctx) { return "handler1"; };
    auto handler2 = [](HttpContext& ctx) { return "handler2"; };
    
    SECTION("Throws on exact duplicate route") {
        router.addRoute("GET", "/api/v1/inventory", handler1);
        
        REQUIRE_THROWS_WITH(
            router.addRoute("GET", "/api/v1/inventory", handler2),
            Catch::Matchers::ContainsSubstring("Duplicate route")
        );
    }
    
    SECTION("Allows same pattern with different methods") {
        REQUIRE_NOTHROW(router.addRoute("GET", "/api/v1/inventory", handler1));
        REQUIRE_NOTHROW(router.addRoute("POST", "/api/v1/inventory", handler1));
        REQUIRE_NOTHROW(router.addRoute("PUT", "/api/v1/inventory", handler1));
        REQUIRE_NOTHROW(router.addRoute("DELETE", "/api/v1/inventory", handler1));
        
        REQUIRE(router.size() == 4);
    }
    
    SECTION("Allows same method with different patterns") {
        REQUIRE_NOTHROW(router.addRoute("GET", "/api/v1/inventory", handler1));
        REQUIRE_NOTHROW(router.addRoute("GET", "/api/v1/inventory/{id}", handler1));
        REQUIRE_NOTHROW(router.addRoute("GET", "/api/v1/products", handler1));
        
        REQUIRE(router.size() == 3);
    }
    
    SECTION("Duplicate check is case-sensitive for paths") {
        router.addRoute("GET", "/api/v1/inventory", handler1);
        
        // Different path case should be treated as different route
        REQUIRE_NOTHROW(router.addRoute("GET", "/api/v1/Inventory", handler2));
    }
    
    SECTION("Duplicate check is case-sensitive for methods") {
        router.addRoute("GET", "/api/v1/inventory", handler1);
        
        // Different method case should be treated as different route
        REQUIRE_NOTHROW(router.addRoute("get", "/api/v1/inventory", handler2));
    }
    
    SECTION("Error message includes method and pattern") {
        router.addRoute("POST", "/api/v1/inventory/reserve", handler1);
        
        try {
            router.addRoute("POST", "/api/v1/inventory/reserve", handler2);
            FAIL("Should have thrown");
        } catch (const std::runtime_error& e) {
            std::string msg = e.what();
            REQUIRE(msg.find("POST") != std::string::npos);
            REQUIRE(msg.find("/api/v1/inventory/reserve") != std::string::npos);
        }
    }
}

TEST_CASE("Router hasRoute() checks existence", "[router][duplicate][validation]") {
    Router router;
    auto handler = [](HttpContext& ctx) { return "test"; };
    
    router.addRoute("GET", "/api/v1/inventory", handler);
    router.addRoute("POST", "/api/v1/inventory", handler);
    router.addRoute("GET", "/api/v1/products", handler);
    
    SECTION("Returns true for existing routes") {
        REQUIRE(router.hasRoute("GET", "/api/v1/inventory"));
        REQUIRE(router.hasRoute("POST", "/api/v1/inventory"));
        REQUIRE(router.hasRoute("GET", "/api/v1/products"));
    }
    
    SECTION("Returns false for non-existing routes") {
        REQUIRE_FALSE(router.hasRoute("DELETE", "/api/v1/inventory"));
        REQUIRE_FALSE(router.hasRoute("GET", "/api/v1/orders"));
        REQUIRE_FALSE(router.hasRoute("POST", "/api/v1/products"));
    }
    
    SECTION("Method and pattern must both match") {
        REQUIRE_FALSE(router.hasRoute("PUT", "/api/v1/inventory"));
        REQUIRE_FALSE(router.hasRoute("GET", "/api/v1/products/123"));
    }
}

TEST_CASE("Router throws on null route", "[router][duplicate][validation]") {
    Router router;
    
    REQUIRE_THROWS_WITH(
        router.addRoute(nullptr),
        Catch::Matchers::ContainsSubstring("null")
    );
}

TEST_CASE("Router duplicate detection with parametrized routes", "[router][duplicate][validation]") {
    Router router;
    auto handler1 = [](HttpContext& ctx) { return "h1"; };
    auto handler2 = [](HttpContext& ctx) { return "h2"; };
    
    SECTION("Parametrized routes with same pattern are duplicates") {
        router.addRoute("GET", "/api/v1/inventory/{id}", handler1);
        
        REQUIRE_THROWS(
            router.addRoute("GET", "/api/v1/inventory/{id}", handler2)
        );
    }
    
    SECTION("Different parameter names = different patterns") {
        REQUIRE_NOTHROW(router.addRoute("GET", "/api/v1/inventory/{id}", handler1));
        REQUIRE_NOTHROW(router.addRoute("GET", "/api/v1/inventory/{itemId}", handler2));
        REQUIRE(router.size() == 2);
    }
    
    SECTION("Pattern with constraint is exact match") {
        router.addRoute("GET", "/api/v1/inventory/{id:uuid}", handler1);
        
        REQUIRE_THROWS(
            router.addRoute("GET", "/api/v1/inventory/{id:uuid}", handler2)
        );
    }
    
    SECTION("Different constraints = different patterns") {
        REQUIRE_NOTHROW(router.addRoute("GET", "/api/v1/page/{page:int}", handler1));
        REQUIRE_NOTHROW(router.addRoute("GET", "/api/v1/page/{page:alpha}", handler2));
        REQUIRE(router.size() == 2);
    }
}

TEST_CASE("Controller duplicate route detection", "[controller][duplicate][validation]") {
    class TestController1 : public ControllerBase {
    public:
        TestController1() : ControllerBase("/api/v1/inventory") {
            Get("", [](HttpContext& ctx) { return "list"; });
            Get("/{id}", [](HttpContext& ctx) { return "get"; });
            Post("", [](HttpContext& ctx) { return "create"; });
        }
    };
    
    class TestController2 : public ControllerBase {
    public:
        TestController2() : ControllerBase("/api/v1/inventory") {
            Get("/{id}", [](HttpContext& ctx) { return "get2"; });  // Duplicate!
            Post("/{id}/reserve", [](HttpContext& ctx) { return "reserve"; });
        }
    };
    
    Router router;
    auto controller1 = std::make_shared<TestController1>();
    auto controller2 = std::make_shared<TestController2>();
    
    SECTION("First controller registers successfully") {
        REQUIRE_NOTHROW(controller1->registerRoutes(router));
        REQUIRE(router.size() == 3);
    }
    
    SECTION("Second controller with duplicate route fails") {
        controller1->registerRoutes(router);
        
        REQUIRE_THROWS_WITH(
            controller2->registerRoutes(router),
            Catch::Matchers::ContainsSubstring("Duplicate")
        );
    }
}

TEST_CASE("Router detects duplicates from Route objects", "[router][duplicate][validation]") {
    Router router;
    auto handler1 = [](HttpContext& ctx) { return "h1"; };
    auto handler2 = [](HttpContext& ctx) { return "h2"; };
    
    auto route1 = std::make_shared<Route>("GET", "/api/v1/test", handler1);
    auto route2 = std::make_shared<Route>("GET", "/api/v1/test", handler2);
    auto route3 = std::make_shared<Route>("POST", "/api/v1/test", handler1);
    
    SECTION("Adding routes via Route objects") {
        REQUIRE_NOTHROW(router.addRoute(route1));
        REQUIRE_THROWS(router.addRoute(route2));
        REQUIRE_NOTHROW(router.addRoute(route3));
    }
    
    SECTION("Mixed addRoute methods check for duplicates") {
        router.addRoute("GET", "/api/v1/test", handler1);
        
        REQUIRE_THROWS(
            router.addRoute(std::make_shared<Route>("GET", "/api/v1/test", handler2))
        );
    }
}

TEST_CASE("Comprehensive duplicate detection scenario", "[router][duplicate][validation][integration]") {
    Router router;
    
    auto h1 = [](HttpContext& ctx) { return "h1"; };
    auto h2 = [](HttpContext& ctx) { return "h2"; };
    auto h3 = [](HttpContext& ctx) { return "h3"; };
    
    SECTION("Complex route registration pattern") {
        // Register core API endpoints
        router.addRoute("GET", "/api/v1/inventory", h1);
        router.addRoute("GET", "/api/v1/inventory/{id}", h1);
        router.addRoute("POST", "/api/v1/inventory", h1);
        router.addRoute("PUT", "/api/v1/inventory/{id}", h1);
        router.addRoute("DELETE", "/api/v1/inventory/{id}", h1);
        router.addRoute("POST", "/api/v1/inventory/{id}/reserve", h1);
        
        REQUIRE(router.size() == 6);
        
        // Try to add duplicate - should fail
        REQUIRE_THROWS(router.addRoute("POST", "/api/v1/inventory/{id}/reserve", h2));
        
        // Add non-duplicate - should succeed
        REQUIRE_NOTHROW(router.addRoute("DELETE", "/api/v1/inventory/{id}/reserve", h3));
        
        REQUIRE(router.size() == 7);
    }
}

TEST_CASE("Large-scale duplicate detection", "[router][duplicate][validation][performance]") {
    Router router;
    auto handler = [](HttpContext& ctx) { return "test"; };
    
    SECTION("Register many routes without duplicates") {
        for (int i = 0; i < 100; ++i) {
            std::string pattern = "/api/v1/resource" + std::to_string(i);
            REQUIRE_NOTHROW(router.addRoute("GET", pattern, handler));
        }
        REQUIRE(router.size() == 100);
    }
    
    SECTION("Duplicate detection works with many routes") {
        for (int i = 0; i < 50; ++i) {
            router.addRoute("GET", "/api/v1/resource" + std::to_string(i), handler);
        }
        
        // Try to add duplicate at various positions
        REQUIRE_THROWS(router.addRoute("GET", "/api/v1/resource0", handler));
        REQUIRE_THROWS(router.addRoute("GET", "/api/v1/resource25", handler));
        REQUIRE_THROWS(router.addRoute("GET", "/api/v1/resource49", handler));
        
        // Adding new ones still works
        REQUIRE_NOTHROW(router.addRoute("GET", "/api/v1/resource50", handler));
        REQUIRE(router.size() == 51);
    }
}
