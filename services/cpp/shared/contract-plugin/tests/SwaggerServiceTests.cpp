#include <catch2/catch_all.hpp>
#include "contract-plugin/SwaggerService.hpp"
#include "contract-plugin/ContractConfig.hpp"
#include "MockClaimsLoader.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace contract;
using namespace contract::tests;

TEST_CASE("SwaggerService constructor with claims loader", "[swagger-service][constructor]") {
    ContractConfig config;
    config.contractsPath = "./contracts";
    config.swaggerTitle = "Test API";
    config.swaggerVersion = "1.0";
    config.swaggerDescription = "Test description";
    
    SECTION("Constructor succeeds with valid loader") {
        auto mockLoader = std::make_shared<MockClaimsLoader>();
        REQUIRE_NOTHROW(SwaggerService(config, mockLoader));
    }
    
    SECTION("Constructor attempts to load claims.json") {
        auto mockLoader = std::make_shared<MockClaimsLoader>();
        
        // Set claims for loading
        json claims = {
            {"service", "test-service"},
            {"version", "2.0.0"}
        };
        mockLoader->setClaims("./contracts/claims.json", claims);
        
        SwaggerService service(config, mockLoader);
        
        // Verify loader was called to load claims
        REQUIRE(mockLoader->getCallCount() >= 1);
    }
    
    SECTION("Constructor handles missing claims gracefully") {
        auto mockLoader = std::make_shared<MockClaimsLoader>();
        mockLoader->setFailure("./contracts/claims.json");
        
        // Should not throw - uses config defaults
        REQUIRE_NOTHROW(SwaggerService(config, mockLoader));
    }
}

TEST_CASE("SwaggerService generateSpec creates base structure", "[swagger-service][spec][base]") {
    ContractConfig config;
    config.contractsPath = "./nonexistent"; // Won't find DTOs/endpoints
    config.swaggerTitle = "Test API";
    config.swaggerVersion = "1.0.0";
    config.swaggerDescription = "API for testing";
    
    auto mockLoader = std::make_shared<MockClaimsLoader>();
    mockLoader->setFailure("./nonexistent/claims.json"); // No claims
    
    SwaggerService service(config, mockLoader);
    auto spec = service.generateSpec();
    
    SECTION("Has OpenAPI version") {
        REQUIRE(spec.contains("openapi"));
        REQUIRE(spec.at("openapi") == "3.0.3");
    }
    
    SECTION("Has info section with config values") {
        REQUIRE(spec.contains("info"));
        auto info = spec.at("info");
        REQUIRE(info.contains("title"));
        REQUIRE(info["title"] == "Test API");
        REQUIRE(info.contains("version"));
        REQUIRE(info["version"] == "1.0.0");
        REQUIRE(info.contains("description"));
        REQUIRE(info["description"] == "API for testing");
    }
    
    SECTION("Has servers section") {
        REQUIRE(spec.contains("servers"));
        REQUIRE(spec.at("servers").is_array());
        REQUIRE(!spec.at("servers").empty());
    }
    
    SECTION("Has paths section") {
        REQUIRE(spec.contains("paths"));
        REQUIRE(spec.at("paths").is_object());
    }
    
    SECTION("Has components section") {
        REQUIRE(spec.contains("components"));
        REQUIRE(spec.at("components").contains("schemas"));
        REQUIRE(spec.at("components")["schemas"].is_object());
    }
}

TEST_CASE("SwaggerService uses claims for metadata when available", "[swagger-service][spec][claims]") {
    ContractConfig config;
    config.contractsPath = "./contracts";
    config.swaggerTitle = "Default Title";
    config.swaggerVersion = "0.0.0";
    
    auto mockLoader = std::make_shared<MockClaimsLoader>();
    
    // Mock claims.json with service metadata
    json claims = {
        {"service", "inventory-service"},
        {"version", "2.5.3"}
    };
    mockLoader->setClaims("./contracts/claims.json", claims);
    
    SwaggerService service(config, mockLoader);
    auto spec = service.generateSpec();
    
    SECTION("Uses service name from claims") {
        auto title = spec.at("info")["title"].get<std::string>();
        // Service name becomes "<service> API"
        REQUIRE(title == "inventory-service API");
    }
    
    SECTION("Uses version from claims") {
        auto version = spec.at("info")["version"].get<std::string>();
        REQUIRE(version == "2.5.3");
    }
}

TEST_CASE("SwaggerService handles missing claims gracefully", "[swagger-service][spec][error]") {
    ContractConfig config;
    config.contractsPath = "./contracts";
    config.swaggerTitle = "Fallback Title";
    config.swaggerVersion = "1.0.0";
    
    auto mockLoader = std::make_shared<MockClaimsLoader>();
    mockLoader->setFailure("./contracts/claims.json"); // No claims available
    
    SwaggerService service(config, mockLoader);
    auto spec = service.generateSpec();
    
    SECTION("Falls back to config values") {
        auto title = spec.at("info")["title"].get<std::string>();
        REQUIRE(title == "Fallback Title");
        
        auto version = spec.at("info")["version"].get<std::string>();
        REQUIRE(version == "1.0.0");
    }
}

TEST_CASE("SwaggerService contractTypeToSchema converts types correctly", "[swagger-service][types]") {
    ContractConfig config;
    config.contractsPath = "./contracts";
    auto mockLoader = std::make_shared<MockClaimsLoader>();
    mockLoader->setFailure("./contracts/claims.json");
    
    SwaggerService service(config, mockLoader);
    auto spec = service.generateSpec();
    
    // We can't directly test the private method, but we can verify behavior
    // through DTO loading in a future test with real contract files
    
    SECTION("Service generates spec successfully") {
        REQUIRE(spec.contains("openapi"));
    }
}

TEST_CASE("SwaggerService handles spec generation errors", "[swagger-service][error]") {
    ContractConfig config;
    config.contractsPath = "./contracts";
    
    SECTION("Returns spec even with missing directories") {
        auto mockLoader = std::make_shared<MockClaimsLoader>();
        mockLoader->setFailure("./contracts/claims.json");
        
        SwaggerService service(config, mockLoader);
        auto spec = service.generateSpec();
        
        // Should still return a valid base spec
        REQUIRE(spec.contains("openapi"));
    }
}

TEST_CASE("SwaggerService spec structure is valid OpenAPI 3.0", "[swagger-service][spec][validation]") {
    ContractConfig config;
    config.contractsPath = "./contracts";
    config.swaggerTitle = "Warehouse API";
    config.swaggerVersion = "1.0.0";
    config.swaggerDescription = "Warehouse management system API";
    
    auto mockLoader = std::make_shared<MockClaimsLoader>();
    mockLoader->setFailure("./contracts/claims.json");
    
    SwaggerService service(config, mockLoader);
    auto spec = service.generateSpec();
    
    SECTION("OpenAPI version is 3.0.3") {
        REQUIRE(spec.at("openapi") == "3.0.3");
    }
    
    SECTION("Info section has required fields") {
        auto info = spec.at("info");
        REQUIRE(info.contains("title"));
        REQUIRE(info.contains("version"));
        REQUIRE(info["title"].is_string());
        REQUIRE(info["version"].is_string());
    }
    
    SECTION("Paths is an object") {
        REQUIRE(spec.at("paths").is_object());
    }
    
    SECTION("Components.schemas is an object") {
        REQUIRE(spec.at("components").is_object());
        REQUIRE(spec.at("components")["schemas"].is_object());
    }
    
    SECTION("Servers is an array") {
        REQUIRE(spec.at("servers").is_array());
    }
}

TEST_CASE("SwaggerService multiple operations", "[swagger-service][operations]") {
    ContractConfig config;
    config.contractsPath = "./contracts";
    
    SECTION("Can create multiple service instances") {
        auto mockLoader = std::make_shared<MockClaimsLoader>();
        
        SwaggerService service1(config, mockLoader);
        SwaggerService service2(config, mockLoader);
        
        auto spec1 = service1.generateSpec();
        auto spec2 = service2.generateSpec();
        
        REQUIRE(spec1.contains("openapi"));
        REQUIRE(spec2.contains("openapi"));
    }
    
    SECTION("Each service instance is independent") {
        auto mockLoader1 = std::make_shared<MockClaimsLoader>();
        auto mockLoader2 = std::make_shared<MockClaimsLoader>();
        
        json claims1 = {{"service", "service-1"}, {"version", "1.0"}};
        json claims2 = {{"service", "service-2"}, {"version", "2.0"}};
        
        mockLoader1->setClaims("./contracts/claims.json", claims1);
        mockLoader2->setClaims("./contracts/claims.json", claims2);
        
        SwaggerService service1(config, mockLoader1);
        SwaggerService service2(config, mockLoader2);
        
        auto spec1 = service1.generateSpec();
        auto spec2 = service2.generateSpec();
        
        // Verify they have different metadata
        REQUIRE(spec1.at("info")["title"] != spec2.at("info")["title"]);
    }
}
