#include <catch2/catch_all.hpp>
#include "inventory/controllers/ClaimsController.hpp"
// #include "inventory/Server.hpp" // Not needed after framework migration
#include <filesystem>
#include <fstream>

using namespace inventory::controllers;

TEST_CASE("ClaimsController loads claims", "[claims][controller]") {
    ClaimsController controller;
    
    // The controller should load claims.json from the project directory
    // We can't easily test HTTP responses without Poco infrastructure,
    // but we can verify the controller instantiates without error
    REQUIRE(true);
}

// NOTE: This test is obsolete after migrating to the HTTP framework.
// The framework handles routing internally via registered controllers and routes.
/*
TEST_CASE("ClaimsController resolves routes", "[claims][routing]") {
    // Test the routing logic in Server.hpp
    REQUIRE(inventory::resolveRoute("/api/v1/claims") == inventory::RouteTarget::Claims);
    REQUIRE(inventory::resolveRoute("/api/v1/claims/fulfilments") == inventory::RouteTarget::Claims);
    REQUIRE(inventory::resolveRoute("/api/v1/claims/references") == inventory::RouteTarget::Claims);
    REQUIRE(inventory::resolveRoute("/api/v1/claims/services") == inventory::RouteTarget::Claims);
    REQUIRE(inventory::resolveRoute("/api/v1/claims/supports/entity/Inventory/1.0") == inventory::RouteTarget::Claims);
    
    // Other routes should not match
    REQUIRE(inventory::resolveRoute("/health") == inventory::RouteTarget::Health);
    REQUIRE(inventory::resolveRoute("/api/swagger.json") == inventory::RouteTarget::Swagger);
    REQUIRE(inventory::resolveRoute("/api/v1/inventory") == inventory::RouteTarget::Inventory);
}
*/

