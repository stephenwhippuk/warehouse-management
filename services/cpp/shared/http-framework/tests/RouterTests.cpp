#include <catch2/catch_all.hpp>
#include "http-framework/Router.hpp"

using namespace http;

TEST_CASE("Route matches exact path", "[router][route]") {
    Route route("GET", "/api/v1/inventory", [](HttpContext& ctx) { 
        return "test"; 
    });
    
    SECTION("Matches exact path") {
        REQUIRE(route.matches("GET", "/api/v1/inventory"));
    }
    
    SECTION("Does not match different path") {
        REQUIRE_FALSE(route.matches("GET", "/api/v1/products"));
    }
    
    SECTION("Does not match different method") {
        REQUIRE_FALSE(route.matches("POST", "/api/v1/inventory"));
    }
}

TEST_CASE("Route matches with parameters", "[router][route][params]") {
    Route route("GET", "/api/v1/inventory/{id}", [](HttpContext& ctx) {
        return "test";
    });
    
    SECTION("Matches path with parameter") {
        REQUIRE(route.matches("GET", "/api/v1/inventory/123"));
        REQUIRE(route.matches("GET", "/api/v1/inventory/abc-def"));
    }
    
    SECTION("Does not match without parameter") {
        REQUIRE_FALSE(route.matches("GET", "/api/v1/inventory/"));
        REQUIRE_FALSE(route.matches("GET", "/api/v1/inventory"));
    }
}

TEST_CASE("Route extracts parameters", "[router][route][params]") {
    Route route("GET", "/api/v1/inventory/{id}/location/{locationId}", 
        [](HttpContext& ctx) { return "test"; });
    
    auto params = route.extractParameters("/api/v1/inventory/123/location/456");
    
    REQUIRE(params.size() == 2);
    REQUIRE(params["id"] == "123");
    REQUIRE(params["locationId"] == "456");
}

TEST_CASE("Route with UUID constraint", "[router][route][constraint]") {
    Route route("GET", "/api/v1/inventory/{id:uuid}", [](HttpContext& ctx) {
        return "test";
    });
    
    SECTION("Matches valid UUID") {
        REQUIRE(route.matches("GET", "/api/v1/inventory/550e8400-e29b-41d4-a716-446655440000"));
    }
    
    SECTION("Does not match invalid UUID") {
        REQUIRE_FALSE(route.matches("GET", "/api/v1/inventory/123"));
        REQUIRE_FALSE(route.matches("GET", "/api/v1/inventory/not-a-uuid"));
    }
}

TEST_CASE("Route with int constraint", "[router][route][constraint]") {
    Route route("GET", "/api/v1/page/{page:int}", [](HttpContext& ctx) {
        return "test";
    });
    
    SECTION("Matches integer") {
        REQUIRE(route.matches("GET", "/api/v1/page/1"));
        REQUIRE(route.matches("GET", "/api/v1/page/999"));
    }
    
    SECTION("Does not match non-integer") {
        REQUIRE_FALSE(route.matches("GET", "/api/v1/page/abc"));
        REQUIRE_FALSE(route.matches("GET", "/api/v1/page/12.5"));
    }
}

TEST_CASE("Router adds and finds routes", "[router]") {
    Router router;
    
    int callCount = 0;
    auto handler = [&callCount](HttpContext& ctx) { 
        callCount++;
        return "test"; 
    };
    
    router.addRoute("GET", "/api/v1/inventory", handler);
    router.addRoute("POST", "/api/v1/inventory", handler);
    
    SECTION("Finds matching route") {
        auto route = router.findRoute("GET", "/api/v1/inventory");
        REQUIRE(route != nullptr);
        REQUIRE(route->getMethod() == "GET");
        REQUIRE(route->getPattern() == "/api/v1/inventory");
    }
    
    SECTION("Returns nullptr for non-matching route") {
        auto route = router.findRoute("DELETE", "/api/v1/inventory");
        REQUIRE(route == nullptr);
    }
    
    SECTION("Distinguishes by method") {
        auto getRoute = router.findRoute("GET", "/api/v1/inventory");
        auto postRoute = router.findRoute("POST", "/api/v1/inventory");
        
        REQUIRE(getRoute != nullptr);
        REQUIRE(postRoute != nullptr);
        REQUIRE(getRoute != postRoute);
    }
}

TEST_CASE("Router with multiple parameters", "[router][params]") {
    Router router;
    
    router.addRoute("GET", "/api/v1/{resource}/{id}/sub/{subId}", 
        [](HttpContext& ctx) { return "test"; });
    
    auto route = router.findRoute("GET", "/api/v1/inventory/123/sub/456");
    REQUIRE(route != nullptr);
    
    auto params = route->extractParameters("/api/v1/inventory/123/sub/456");
    REQUIRE(params["resource"] == "inventory");
    REQUIRE(params["id"] == "123");
    REQUIRE(params["subId"] == "456");
}

TEST_CASE("RouteBuilder fluent API", "[router][builder]") {
    Router router;
    
    RouteBuilder(router)
        .get("/inventory", [](HttpContext& ctx) { return "get"; })
        .post("/inventory", [](HttpContext& ctx) { return "post"; })
        .put("/inventory/{id}", [](HttpContext& ctx) { return "put"; })
        .del("/inventory/{id}", [](HttpContext& ctx) { return "delete"; });
    
    REQUIRE(router.size() == 4);
    REQUIRE(router.findRoute("GET", "/inventory") != nullptr);
    REQUIRE(router.findRoute("POST", "/inventory") != nullptr);
    REQUIRE(router.findRoute("PUT", "/inventory/123") != nullptr);
    REQUIRE(router.findRoute("DELETE", "/inventory/123") != nullptr);
}
