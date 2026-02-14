#include <catch2/catch_all.hpp>
#include "product/models/Product.hpp"

using namespace product;

TEST_CASE("Product model", "[product][model]") {
    SECTION("Create valid product") {
        models::Product p("550e8400-e29b-41d4-a716-446655440000",
                         "PROD-001",
                         "Widget",
                         "A useful widget",
                         "Tools",
                         models::Product::Status::ACTIVE);
        
        REQUIRE(p.getId() == "550e8400-e29b-41d4-a716-446655440000");
        REQUIRE(p.getSku() == "PROD-001");
        REQUIRE(p.getName() == "Widget");
        REQUIRE(p.getStatus() == models::Product::Status::ACTIVE);
    }
    
    SECTION("Empty id throws") {
        REQUIRE_THROWS_AS(
            models::Product("", "PROD-001", "Widget", std::nullopt, std::nullopt, models::Product::Status::ACTIVE),
            std::invalid_argument
        );
    }
    
    SECTION("Empty SKU throws") {
        REQUIRE_THROWS_AS(
            models::Product("550e8400-e29b-41d4-a716-446655440000", "", "Widget", std::nullopt, std::nullopt, models::Product::Status::ACTIVE),
            std::invalid_argument
        );
    }
    
    SECTION("Empty name throws") {
        REQUIRE_THROWS_AS(
            models::Product("550e8400-e29b-41d4-a716-446655440000", "PROD-001", "", std::nullopt, std::nullopt, models::Product::Status::ACTIVE),
            std::invalid_argument
        );
    }
}

TEST_CASE("Product serialization", "[product][model][json]") {
    models::Product p("550e8400-e29b-41d4-a716-446655440000",
                     "PROD-001",
                     "Widget",
                     "A useful widget",
                     "Tools",
                     models::Product::Status::ACTIVE);
    
    auto json = p.toJson();
    
    REQUIRE(json["id"] == "550e8400-e29b-41d4-a716-446655440000");
    REQUIRE(json["sku"] == "PROD-001");
    REQUIRE(json["name"] == "Widget");
    REQUIRE(json["description"] == "A useful widget");
    REQUIRE(json["category"] == "Tools");
    REQUIRE(json["status"] == "active");
}

TEST_CASE("Product deserialization", "[product][model][json]") {
    using json = nlohmann::json;
    
    json j = {
        {"id", "550e8400-e29b-41d4-a716-446655440000"},
        {"sku", "PROD-001"},
        {"name", "Widget"},
        {"description", "A useful widget"},
        {"category", "Tools"},
        {"status", "active"}
    };
    
    auto p = models::Product::fromJson(j);
    
    REQUIRE(p.getId() == "550e8400-e29b-41d4-a716-446655440000");
    REQUIRE(p.getSku() == "PROD-001");
    REQUIRE(p.getName() == "Widget");
    REQUIRE(p.getStatus() == models::Product::Status::ACTIVE);
}
