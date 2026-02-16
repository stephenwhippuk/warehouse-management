# ClaimsLoader Testing Guide

## Overview

The ClaimsLoader has been extracted into a mockable interface (`IClaimsLoader`) to enable comprehensive testing of ClaimsService without filesystem dependencies.

## Architecture

```
IClaimsLoader (interface)
    ↓
ClaimsLoader (filesystem implementation)
    ↓
ClaimsService (business logic)
    ↓
ClaimsController (HTTP handlers)
```

## Mocking ClaimsLoader

### Mock Implementation Example

```cpp
#include "contract-plugin/IClaimsLoader.hpp"
#include <nlohmann/json.hpp>
#include <gmock/gmock.h>

using json = nlohmann::json;

class MockClaimsLoader : public contract::IClaimsLoader {
public:
    MOCK_METHOD(std::optional<json>, loadClaims, (const std::string& claimsPath), (override));
};
```

### Test Example: Successful Load

```cpp
#include <catch2/catch_all.hpp>
#include "contract-plugin/ClaimsService.hpp"
#include "contract-plugin/ContractConfig.hpp"

TEST_CASE("ClaimsService loads claims successfully", "[claims-service]") {
    // Arrange
    auto mockLoader = std::make_shared<MockClaimsLoader>();
    
    json mockClaims = {
        {"service", "test-service"},
        {"version", "1.0.0"},
        {"fulfilments", json::array({
            {
                {"contract", "Product"},
                {"versions", {"1.0"}}
            }
        })},
        {"references", json::array()},
        {"serviceContracts", json::array()}
    };
    
    EXPECT_CALL(*mockLoader, loadClaims("./claims.json"))
        .WillOnce(testing::Return(mockClaims));
    
    contract::ContractConfig config;
    config.claimsPath = "./claims.json";
    
    // Act
    contract::ClaimsService service(config, mockLoader);
    
    // Assert
    auto claims = service.getAllClaims();
    REQUIRE(claims.has_value());
    REQUIRE(claims->at("service") == "test-service");
}
```

### Test Example: Load Failure

```cpp
TEST_CASE("ClaimsService handles load failure", "[claims-service]") {
    // Arrange
    auto mockLoader = std::make_shared<MockClaimsLoader>();
    
    EXPECT_CALL(*mockLoader, loadClaims("./missing.json"))
        .WillOnce(testing::Return(std::nullopt));
    
    contract::ContractConfig config;
    config.claimsPath = "./missing.json";
    
    // Act
    contract::ClaimsService service(config, mockLoader);
    
    // Assert
    auto claims = service.getAllClaims();
    REQUIRE_FALSE(claims.has_value());
}
```

### Test Example: Entity Support Check

```cpp
TEST_CASE("ClaimsService checks entity support", "[claims-service]") {
    // Arrange
    auto mockLoader = std::make_shared<MockClaimsLoader>();
    
    json mockClaims = {
        {"service", "inventory-service"},
        {"version", "1.0.0"},
        {"fulfilments", json::array({
            {
                {"contract", "Inventory"},
                {"versions", {"1.0", "1.1"}}
            }
        })},
        {"references", json::array({
            {
                {"contract", "Product"},
                {"versions", {"1.0"}}
            }
        })}
    };
    
    EXPECT_CALL(*mockLoader, loadClaims("./claims.json"))
        .WillOnce(testing::Return(mockClaims));
    
    contract::ContractConfig config;
    config.claimsPath = "./claims.json";
    contract::ClaimsService service(config, mockLoader);
    
    // Act & Assert
    SECTION("Fulfilled entity is supported") {
        auto result = service.checkSupport("entity", "Inventory", "1.0");
        REQUIRE(result["supported"] == true);
        REQUIRE(result["fulfilled"] == true);
    }
    
    SECTION("Referenced entity is supported") {
        auto result = service.checkSupport("entity", "Product", "1.0");
        REQUIRE(result["supported"] == true);
        REQUIRE(result["fulfilled"] == false);
    }
    
    SECTION("Unknown entity is not supported") {
        auto result = service.checkSupport("entity", "Unknown", "1.0");
        REQUIRE(result["supported"] == false);
    }
}
```

### Test Example: Filtered Claims

```cpp
TEST_CASE("ClaimsService returns filtered claims", "[claims-service]") {
    // Arrange
    auto mockLoader = std::make_shared<MockClaimsLoader>();
    
    json mockClaims = {
        {"service", "test-service"},
        {"version", "1.0.0"},
        {"fulfilments", json::array({
            {{"contract", "Entity1"}, {"versions", {"1.0"}}}
        })},
        {"references", json::array({
            {{"contract", "Entity2"}, {"versions", {"1.0"}}}
        })},
        {"serviceContracts", json::array({
            {{"contract", "Service1"}, {"versions", {"1.0"}}}
        })}
    };
    
    EXPECT_CALL(*mockLoader, loadClaims(testing::_))
        .WillOnce(testing::Return(mockClaims));
    
    contract::ContractConfig config;
    contract::ClaimsService service(config, mockLoader);
    
    // Act & Assert
    SECTION("getFulfilments returns only fulfilments") {
        auto result = service.getFulfilments();
        REQUIRE(result.has_value());
        REQUIRE(result->contains("fulfilments"));
        REQUIRE_FALSE(result->contains("references"));
        REQUIRE_FALSE(result->contains("serviceContracts"));
    }
    
    SECTION("getReferences returns only references") {
        auto result = service.getReferences();
        REQUIRE(result.has_value());
        REQUIRE(result->contains("references"));
        REQUIRE_FALSE(result->contains("fulfilments"));
    }
    
    SECTION("getServices returns only service contracts") {
        auto result = service.getServices();
        REQUIRE(result.has_value());
        REQUIRE(result->contains("serviceContracts"));
        REQUIRE_FALSE(result->contains("fulfilments"));
    }
}
```

## Integration Testing

For integration tests with real filesystem:

```cpp
TEST_CASE("ClaimsLoader loads real claims.json", "[claims-loader][integration]") {
    // Setup: Create temporary claims.json
    std::ofstream file("./test_claims.json");
    file << R"({
        "service": "test-service",
        "version": "1.0.0",
        "fulfilments": [],
        "references": []
    })";
    file.close();
    
    // Act
    contract::ClaimsLoader loader;
    auto result = loader.loadClaims("./test_claims.json");
    
    // Assert
    REQUIRE(result.has_value());
    REQUIRE(result->at("service") == "test-service");
    
    // Cleanup
    std::filesystem::remove("./test_claims.json");
}
```

## Benefits

1. **Fast Unit Tests**: No filesystem I/O, tests run in milliseconds
2. **Predictable State**: Mock returns exactly what you specify
3. **Error Testing**: Easily test failure scenarios (file not found, invalid JSON)
4. **Isolation**: ClaimsService logic tested independently of filesystem
5. **CI/CD Friendly**: No need for fixture files in test environments

## DI Configuration

The plugin registers ClaimsLoader as a singleton:

```cpp
// In ContractPlugin::registerServices()
services.addService<IClaimsLoader>(
    [](http::IServiceProvider&) {
        return std::make_shared<ClaimsLoader>();
    },
    http::ServiceLifetime::Singleton
);

services.addService<IClaimsService>(
    [config = this->config_](http::IServiceProvider& provider) {
        auto loader = provider.getService<IClaimsLoader>();
        return std::make_shared<ClaimsService>(config, loader);
    },
    http::ServiceLifetime::Transient
);
```

For tests, you can override with mock:

```cpp
http::ServiceCollection services;

// Register mock loader instead of real one
services.addService<IClaimsLoader>(
    [mockLoader](http::IServiceProvider&) {
        return mockLoader; // Your mock instance
    },
    http::ServiceLifetime::Singleton
);

// Service will receive mock loader
services.addService<IClaimsService>(
    [config](http::IServiceProvider& provider) {
        auto loader = provider.getService<IClaimsLoader>();
        return std::make_shared<ClaimsService>(config, loader);
    },
    http::ServiceLifetime::Transient
);
```

## Summary

The ClaimsLoader abstraction provides:
- ✅ Mockable interface for testing
- ✅ Separation of concerns (I/O vs business logic)
- ✅ Testable ClaimsService without filesystem dependencies
- ✅ Easy to add alternative loaders (network, database, cache)
- ✅ Production code uses real ClaimsLoader via DI
- ✅ Test code uses MockClaimsLoader for fast, reliable tests
