#include <catch2/catch_all.hpp>
#include "product/utils/DtoMapper.hpp"
#include "product/models/Product.hpp"
#include "product/dtos/ProductItemDto.hpp"
#include "product/dtos/ProductListDto.hpp"

using namespace product;

// Helper: Create valid product model
models::Product createValidProduct() {
    return models::Product(
        "550e8400-e29b-41d4-a716-446655440000",
        "PROD-001",
        "Widget",
        "A useful widget",
        "Tools",
        models::Product::Status::ACTIVE
    );
}

TEST_CASE("DtoMapper converts valid product", "[dto][mapper]") {
    auto product = createValidProduct();
    
    SECTION("Successful conversion with all fields") {
        auto dto = utils::DtoMapper::toProductItemDto(product);
        
        REQUIRE(dto.getId() == product.getId());
        REQUIRE(dto.getSku() == product.getSku());
        REQUIRE(dto.getName() == product.getName());
        REQUIRE(dto.getStatus() == "active");
        REQUIRE(dto.getDescription() == product.getDescription());
        REQUIRE(dto.getCategory() == product.getCategory());
    }
    
    SECTION("Conversion with optional fields absent") {
        models::Product p("550e8400-e29b-41d4-a716-446655440000",
                         "PROD-002",
                         "Gadget",
                         std::nullopt,
                         std::nullopt,
                         models::Product::Status::INACTIVE);
        
        auto dto = utils::DtoMapper::toProductItemDto(p);
        
        REQUIRE(dto.getId() == p.getId());
        REQUIRE(dto.getDescription() == std::nullopt);
        REQUIRE(dto.getCategory() == std::nullopt);
        REQUIRE(dto.getStatus() == "inactive");
    }
}

TEST_CASE("DtoMapper handles all enum values", "[dto][mapper][enum]") {
    SECTION("Active status") {
        auto product = createValidProduct();
        product.setStatus(models::Product::Status::ACTIVE);
        auto dto = utils::DtoMapper::toProductItemDto(product);
        REQUIRE(dto.getStatus() == "active");
    }
    
    SECTION("Inactive status") {
        auto product = createValidProduct();
        product.setStatus(models::Product::Status::INACTIVE);
        auto dto = utils::DtoMapper::toProductItemDto(product);
        REQUIRE(dto.getStatus() == "inactive");
    }
    
    SECTION("Discontinued status") {
        auto product = createValidProduct();
        product.setStatus(models::Product::Status::DISCONTINUED);
        auto dto = utils::DtoMapper::toProductItemDto(product);
        REQUIRE(dto.getStatus() == "discontinued");
    }
}

TEST_CASE("ProductItemDto constructor validates fields", "[dto][validation]") {
    SECTION("Valid construction succeeds") {
        REQUIRE_NOTHROW(
            dtos::ProductItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "PROD-001",
                "Widget",
                "A useful widget",
                "Tools",
                "active"
            )
        );
    }
    
    SECTION("Invalid UUID throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductItemDto(
                "not-a-uuid",
                "PROD-001",
                "Widget",
                "A useful widget",
                "Tools",
                "active"
            ),
            Catch::Matchers::ContainsSubstring("valid UUID")
        );
    }
    
    SECTION("Empty SKU throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "",
                "Widget",
                "A useful widget",
                "Tools",
                "active"
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("SKU too long throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                std::string(101, 'A'),
                "Widget",
                std::nullopt,
                std::nullopt,
                "active"
            ),
            Catch::Matchers::ContainsSubstring("at most 100")
        );
    }
    
    SECTION("Invalid SKU format throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "prod-001",  // lowercase not allowed
                "Widget",
                std::nullopt,
                std::nullopt,
                "active"
            ),
            Catch::Matchers::ContainsSubstring("uppercase")
        );
    }
    
    SECTION("Empty name throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "PROD-001",
                "",
                std::nullopt,
                std::nullopt,
                "active"
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Name too long throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "PROD-001",
                std::string(201, 'A'),
                std::nullopt,
                std::nullopt,
                "active"
            ),
            Catch::Matchers::ContainsSubstring("at most 200")
        );
    }
    
    SECTION("Description too long throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "PROD-001",
                "Widget",
                std::string(2001, 'A'),
                std::nullopt,
                "active"
            ),
            Catch::Matchers::ContainsSubstring("at most 2000")
        );
    }
    
    SECTION("Category too long throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "PROD-001",
                "Widget",
                std::nullopt,
                std::string(101, 'A'),
                "active"
            ),
            Catch::Matchers::ContainsSubstring("at most 100")
        );
    }
    
    SECTION("Invalid status throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductItemDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "PROD-001",
                "Widget",
                std::nullopt,
                std::nullopt,
                "invalid-status"
            ),
            Catch::Matchers::ContainsSubstring("status")
        );
    }
}

TEST_CASE("ProductListDto pagination", "[dto][list]") {
    std::vector<dtos::ProductItemDto> items;
    items.push_back(dtos::ProductItemDto(
        "550e8400-e29b-41d4-a716-446655440000",
        "PROD-001",
        "Widget",
        std::nullopt,
        std::nullopt,
        "active"
    ));
    
    SECTION("Valid list construction") {
        dtos::ProductListDto list(items, 1, 1, 50, 1);
        REQUIRE(list.getItems().size() == 1);
        REQUIRE(list.getTotalCount() == 1);
        REQUIRE(list.getPage() == 1);
        REQUIRE(list.getPageSize() == 50);
        REQUIRE(list.getTotalPages() == 1);
    }
    
    SECTION("Negative total count throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductListDto(items, -1, 1, 50, 1),
            Catch::Matchers::ContainsSubstring("non-negative")
        );
    }
    
    SECTION("Invalid page number throws") {
        REQUIRE_THROWS_WITH(
            dtos::ProductListDto(items, 1, 0, 50, 1),
            Catch::Matchers::ContainsSubstring("at least 1")
        );
    }
}
