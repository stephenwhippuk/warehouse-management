#include <catch2/catch_all.hpp>

#include "inventory/Server.hpp"

using inventory::RouteTarget;

TEST_CASE("HTTP routing maps paths to expected targets", "[routing][server]") {
    SECTION("Health endpoint routes to Health target") {
        auto target = inventory::resolveRoute("/health");
        REQUIRE(target == RouteTarget::Health);
    }

    SECTION("Swagger endpoint routes to Swagger target") {
        auto target = inventory::resolveRoute("/api/swagger.json");
        REQUIRE(target == RouteTarget::Swagger);
    }

    SECTION("Inventory and other paths route to Inventory target by default") {
        REQUIRE(inventory::resolveRoute("/api/v1/inventory") == RouteTarget::Inventory);
        REQUIRE(inventory::resolveRoute("/") == RouteTarget::Inventory);
        REQUIRE(inventory::resolveRoute("/unknown") == RouteTarget::Inventory);
    }
}
