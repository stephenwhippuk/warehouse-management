#include <catch2/catch_all.hpp>
#include "contract-plugin/ClaimsService.hpp"
#include "contract-plugin/ContractConfig.hpp"
#include "MockClaimsLoader.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace contract;
using namespace contract::tests;

// Helper: Create valid claims JSON
json createValidClaims(const std::string& service = "test-service",
                       const std::string& version = "1.0.0") {
    return json{
        {"service", service},
        {"version", version},
        {"fulfilments", json::array()},
        {"references", json::array()},
        {"serviceContracts", json::array()}
    };
}

TEST_CASE("ClaimsService constructor", "[claims-service][constructor]") {
    ContractConfig config;
    config.claimsPath = "./claims.json";

    SECTION("Successfully loads claims from mock loader") {
        auto mockLoader = std::make_shared<MockClaimsLoader>();
        json mockClaims = createValidClaims("inventory-service", "1.0.0");
        mockLoader->setClaims("./claims.json", mockClaims);

        ClaimsService service(config, mockLoader);

        // Verify loader was called
        REQUIRE(mockLoader->getCallCount() == 1);
        REQUIRE(mockLoader->getLastPath() == "./claims.json");

        // Verify claims loaded
        auto claims = service.getAllClaims();
        REQUIRE(claims.at("service") == "inventory-service");
        REQUIRE(claims.at("version") == "1.0.0");
    }

    SECTION("Handles load failure - throws exception on access") {
        auto mockLoader = std::make_shared<MockClaimsLoader>();
        mockLoader->setFailure("./claims.json");

        ClaimsService service(config, mockLoader);

        // Verify loader was called
        REQUIRE(mockLoader->getCallCount() == 1);

        // Verify accessing claims throws exception
        REQUIRE_THROWS_AS(service.getAllClaims(), std::runtime_error);
        REQUIRE_THROWS_WITH(service.getAllClaims(), 
                           Catch::Matchers::ContainsSubstring("Claims not loaded"));
    }

    SECTION("Uses configured path") {
        auto mockLoader = std::make_shared<MockClaimsLoader>();
        json mockClaims = createValidClaims();
        mockLoader->setClaims("./custom/path.json", mockClaims);

        config.claimsPath = "./custom/path.json";
        ClaimsService service(config, mockLoader);

        REQUIRE(mockLoader->getLastPath() == "./custom/path.json");
    }
}

TEST_CASE("ClaimsService::getAllClaims", "[claims-service][get-all]") {
    ContractConfig config;
    config.claimsPath = "./claims.json";

    SECTION("Returns full claims when loaded") {
        auto mockLoader = std::make_shared<MockClaimsLoader>();
        json mockClaims = {
            {"service", "test-service"},
            {"version", "2.5.0"},
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
        mockLoader->setClaims("./claims.json", mockClaims);

        ClaimsService service(config, mockLoader);
        auto result = service.getAllClaims();

        REQUIRE(result.at("service") == "test-service");
        REQUIRE(result.at("version") == "2.5.0");
        REQUIRE(result.contains("fulfilments"));
        REQUIRE(result.contains("references"));
        REQUIRE(result.contains("serviceContracts"));
    }

    SECTION("Throws exception when claims not loaded") {
        auto mockLoader = std::make_shared<MockClaimsLoader>();
        mockLoader->setFailure("./claims.json");

        ClaimsService service(config, mockLoader);

        REQUIRE_THROWS_AS(service.getAllClaims(), std::runtime_error);
    }
}

TEST_CASE("ClaimsService::getFulfilments", "[claims-service][fulfilments]") {
    ContractConfig config;
    config.claimsPath = "./claims.json";
    auto mockLoader = std::make_shared<MockClaimsLoader>();

    json mockClaims = {
        {"service", "inventory-service"},
        {"version", "1.0.0"},
        {"fulfilments", json::array({
            {{"contract", "Inventory"}, {"versions", {"1.0", "1.1"}}}
        })},
        {"references", json::array({
            {{"contract", "Product"}, {"versions", {"1.0"}}}
        })},
        {"serviceContracts", json::array()}
    };
    mockLoader->setClaims("./claims.json", mockClaims);

    ClaimsService service(config, mockLoader);

    SECTION("Returns service, version, and fulfilments only") {
        auto result = service.getFulfilments();

        REQUIRE(result.at("service") == "inventory-service");
        REQUIRE(result.at("version") == "1.0.0");
        REQUIRE(result.contains("fulfilments"));
        REQUIRE_FALSE(result.contains("references"));
        REQUIRE_FALSE(result.contains("serviceContracts"));
    }

    SECTION("Fulfilments array contains expected contracts") {
        auto result = service.getFulfilments();

        auto& fulfilments = result.at("fulfilments");
        REQUIRE(fulfilments.is_array());
        REQUIRE(fulfilments.size() == 1);
        REQUIRE(fulfilments[0]["contract"] == "Inventory");
    }
}

TEST_CASE("ClaimsService::getReferences", "[claims-service][references]") {
    ContractConfig config;
    config.claimsPath = "./claims.json";
    auto mockLoader = std::make_shared<MockClaimsLoader>();

    json mockClaims = {
        {"service", "order-service"},
        {"version", "1.0.0"},
        {"fulfilments", json::array()},
        {"references", json::array({
            {{"contract", "Product"}, {"versions", {"1.0"}}},
            {{"contract", "Warehouse"}, {"versions", {"1.0"}}}
        })},
        {"serviceContracts", json::array()}
    };
    mockLoader->setClaims("./claims.json", mockClaims);

    ClaimsService service(config, mockLoader);

    SECTION("Returns service, version, and references only") {
        auto result = service.getReferences();

        REQUIRE(result.at("service") == "order-service");
        REQUIRE(result.at("version") == "1.0.0");
        REQUIRE(result.contains("references"));
        REQUIRE_FALSE(result.contains("fulfilments"));
        REQUIRE_FALSE(result.contains("serviceContracts"));
    }

    SECTION("References array contains expected contracts") {
        auto result = service.getReferences();

        auto& references = result.at("references");
        REQUIRE(references.is_array());
        REQUIRE(references.size() == 2);
        REQUIRE(references[0]["contract"] == "Product");
        REQUIRE(references[1]["contract"] == "Warehouse");
    }
}

TEST_CASE("ClaimsService::getServices", "[claims-service][services]") {
    ContractConfig config;
    config.claimsPath = "./claims.json";
    auto mockLoader = std::make_shared<MockClaimsLoader>();

    json mockClaims = {
        {"service", "api-gateway"},
        {"version", "2.0.0"},
        {"fulfilments", json::array()},
        {"references", json::array()},
        {"serviceContracts", json::array({
            {{"contract", "InventoryManagementService"}, {"versions", {"1.0"}}}
        })}
    };
    mockLoader->setClaims("./claims.json", mockClaims);

    ClaimsService service(config, mockLoader);

    SECTION("Returns service, version, and serviceContracts only") {
        auto result = service.getServices();

        REQUIRE(result.at("service") == "api-gateway");
        REQUIRE(result.at("version") == "2.0.0");
        REQUIRE(result.contains("serviceContracts"));
        REQUIRE_FALSE(result.contains("fulfilments"));
        REQUIRE_FALSE(result.contains("references"));
    }

    SECTION("ServiceContracts array contains expected contracts") {
        auto result = service.getServices();

        auto& services = result.at("serviceContracts");
        REQUIRE(services.is_array());
        REQUIRE(services.size() == 1);
        REQUIRE(services[0]["contract"] == "InventoryManagementService");
    }
}

TEST_CASE("ClaimsService::checkSupport for entities", "[claims-service][support][entity]") {
    ContractConfig config;
    config.claimsPath = "./claims.json";
    auto mockLoader = std::make_shared<MockClaimsLoader>();

    json mockClaims = {
        {"service", "inventory-service"},
        {"version", "1.0.0"},
        {"fulfilments", json::array({
            {{"contract", "Inventory"}, {"versions", {"1.0", "1.1", "2.0"}}}
        })},
        {"references", json::array({
            {{"contract", "Product"}, {"versions", {"1.0"}}},
            {{"contract", "Warehouse"}, {"versions", {"1.0", "1.5"}}}
        })},
        {"serviceContracts", json::array()}
    };
    mockLoader->setClaims("./claims.json", mockClaims);

    ClaimsService service(config, mockLoader);

    SECTION("Returns supported for fulfilled entity with matching version") {
        auto result = service.checkSupport("entity", "Inventory", "1.0");

        REQUIRE(result["type"] == "entity");
        REQUIRE(result["contract"] == "Inventory");
        REQUIRE(result["version"] == "1.0");
        REQUIRE(result["supported"] == true);
        REQUIRE(result["fulfilled"] == true);
    }

    SECTION("Returns supported for fulfilled entity with different matching version") {
        auto result = service.checkSupport("entity", "Inventory", "2.0");

        REQUIRE(result["supported"] == true);
        REQUIRE(result["fulfilled"] == true);
    }

    SECTION("Returns not supported for fulfilled entity with non-matching version") {
        auto result = service.checkSupport("entity", "Inventory", "3.0");

        REQUIRE(result["supported"] == false);
        REQUIRE_FALSE(result.contains("fulfilled"));
    }

    SECTION("Returns supported for referenced entity with matching version") {
        auto result = service.checkSupport("entity", "Product", "1.0");

        REQUIRE(result["type"] == "entity");
        REQUIRE(result["contract"] == "Product");
        REQUIRE(result["version"] == "1.0");
        REQUIRE(result["supported"] == true);
        REQUIRE(result["fulfilled"] == false);
    }

    SECTION("Returns supported for referenced entity with multiple versions") {
        auto result = service.checkSupport("entity", "Warehouse", "1.5");

        REQUIRE(result["supported"] == true);
        REQUIRE(result["fulfilled"] == false);
    }

    SECTION("Returns not supported for unknown entity") {
        auto result = service.checkSupport("entity", "Order", "1.0");

        REQUIRE(result["type"] == "entity");
        REQUIRE(result["contract"] == "Order");
        REQUIRE(result["version"] == "1.0");
        REQUIRE(result["supported"] == false);
    }
}

TEST_CASE("ClaimsService::checkSupport for services", "[claims-service][support][service]") {
    ContractConfig config;
    config.claimsPath = "./claims.json";
    auto mockLoader = std::make_shared<MockClaimsLoader>();

    json mockClaims = {
        {"service", "api-gateway"},
        {"version", "1.0.0"},
        {"fulfilments", json::array()},
        {"references", json::array()},
        {"serviceContracts", json::array({
            {{"contract", "InventoryManagementService"}, {"versions", {"1.0", "1.1"}}},
            {{"contract", "OrderManagementService"}, {"versions", {"2.0"}}}
        })}
    };
    mockLoader->setClaims("./claims.json", mockClaims);

    ClaimsService service(config, mockLoader);

    SECTION("Returns supported for service contract with matching version") {
        auto result = service.checkSupport("service", "InventoryManagementService", "1.0");

        REQUIRE(result["type"] == "service");
        REQUIRE(result["contract"] == "InventoryManagementService");
        REQUIRE(result["version"] == "1.0");
        REQUIRE(result["supported"] == true);
    }

    SECTION("Returns supported for service contract with multiple versions") {
        auto result = service.checkSupport("service", "InventoryManagementService", "1.1");

        REQUIRE(result["supported"] == true);
    }

    SECTION("Returns not supported for service contract with non-matching version") {
        auto result = service.checkSupport("service", "InventoryManagementService", "2.0");

        REQUIRE(result["supported"] == false);
    }

    SECTION("Returns not supported for unknown service contract") {
        auto result = service.checkSupport("service", "UnknownService", "1.0");

        REQUIRE(result["supported"] == false);
    }
}

TEST_CASE("ClaimsService::checkSupport with invalid type", "[claims-service][support][error]") {
    ContractConfig config;
    config.claimsPath = "./claims.json";
    auto mockLoader = std::make_shared<MockClaimsLoader>();

    json mockClaims = createValidClaims();
    mockLoader->setClaims("./claims.json", mockClaims);

    ClaimsService service(config, mockLoader);

    SECTION("Throws exception for invalid type") {
        REQUIRE_THROWS_AS(service.checkSupport("invalid", "SomeContract", "1.0"), std::invalid_argument);
        REQUIRE_THROWS_WITH(service.checkSupport("invalid", "SomeContract", "1.0"),
                           Catch::Matchers::ContainsSubstring("Invalid type"));
    }
}

TEST_CASE("ClaimsService::checkSupport when claims not loaded", "[claims-service][support][no-claims]") {
    ContractConfig config;
    config.claimsPath = "./claims.json";
    auto mockLoader = std::make_shared<MockClaimsLoader>();
    mockLoader->setFailure("./claims.json");

    ClaimsService service(config, mockLoader);

    SECTION("Throws exception when claims not loaded") {
        REQUIRE_THROWS_AS(service.checkSupport("entity", "Product", "1.0"), std::runtime_error);
        REQUIRE_THROWS_WITH(service.checkSupport("entity", "Product", "1.0"),
                           Catch::Matchers::ContainsSubstring("Claims not loaded"));
    }
}

TEST_CASE("ClaimsService with empty claims sections", "[claims-service][empty]") {
    ContractConfig config;
    config.claimsPath = "./claims.json";
    auto mockLoader = std::make_shared<MockClaimsLoader>();

    json mockClaims = {
        {"service", "minimal-service"},
        {"version", "1.0.0"}
        // No fulfilments, references, or serviceContracts
    };
    mockLoader->setClaims("./claims.json", mockClaims);

    ClaimsService service(config, mockLoader);

    SECTION("getFulfilments returns empty array") {
        auto result = service.getFulfilments();
        REQUIRE(result.at("fulfilments").is_array());
        REQUIRE(result.at("fulfilments").size() == 0);
    }

    SECTION("getReferences returns empty array") {
        auto result = service.getReferences();
        REQUIRE(result.at("references").is_array());
        REQUIRE(result.at("references").size() == 0);
    }

    SECTION("getServices returns empty array") {
        auto result = service.getServices();
        REQUIRE(result.at("serviceContracts").is_array());
        REQUIRE(result.at("serviceContracts").size() == 0);
    }

    SECTION("checkSupport returns not supported") {
        auto result = service.checkSupport("entity", "AnyEntity", "1.0");
        REQUIRE(result["supported"] == false);
    }
}
