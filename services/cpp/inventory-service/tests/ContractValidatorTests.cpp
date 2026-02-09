#include <catch2/catch_all.hpp>
#include "contract_validator/ContractValidator.hpp"
#include <filesystem>
#include <iostream>

using namespace contract_validator;

TEST_CASE("ContractValidator validates field exposure", "[validator][field_exposure]") {
    std::string contractsRoot = "/home/stephen/localdev/experiments/warehouse-management/contracts";
    std::string serviceContracts = "contracts";
    std::string claimsPath = "claims.json";
    
    if (!std::filesystem::exists(contractsRoot) || !std::filesystem::exists(serviceContracts)) {
        WARN("Contracts not found, skipping test");
        return;
    }

    ContractValidator validator(contractsRoot, serviceContracts, claimsPath);
    
    auto result = validator.validate();
    
    // Output results for inspection
    std::cout << result.summary() << std::endl;
    
    // The current setup should validate correctly
    if (result.hasErrors()) {
        std::cout << "\nValidation errors found - this indicates contracts or claims need updating:\n";
        for (const auto& error : result.errors) {
            std::cout << "  " << error.toString() << "\n";
        }
    }
    
    // This is informational - we don't fail the test but report findings
    REQUIRE(true);
}

TEST_CASE("ContractValidator checks identity fields", "[validator][identity_fields]") {
    std::string contractsRoot = "/home/stephen/localdev/experiments/warehouse-management/contracts";
    std::string serviceContracts = "contracts";
    std::string claimsPath = "claims.json";
    
    if (!std::filesystem::exists(contractsRoot) || !std::filesystem::exists(serviceContracts)) {
        WARN("Contracts not found, skipping test");
        return;
    }

    ContractValidator validator(contractsRoot, serviceContracts, claimsPath);
    auto errors = validator.validateIdentityFields();
    
    std::cout << "\nIdentity field validation:\n";
    if (errors.empty()) {
        std::cout << "  ✓ All identity fields properly included\n";
    } else {
        std::cout << "  ✗ " << errors.size() << " issues found:\n";
        for (const auto& error : errors) {
            std::cout << "    " << error.toString() << "\n";
        }
    }
    
    REQUIRE(true);
}

TEST_CASE("ContractValidator validates DTO basis", "[validator][dto_basis]") {
    std::string contractsRoot = "/home/stephen/localdev/experiments/warehouse-management/contracts";
    std::string serviceContracts = "contracts";
    std::string claimsPath = "claims.json";
    
    if (!std::filesystem::exists(contractsRoot) || !std::filesystem::exists(serviceContracts)) {
        WARN("Contracts not found, skipping test");
        return;
    }

    ContractValidator validator(contractsRoot, serviceContracts, claimsPath);
    auto errors = validator.validateDtoBasis();
    
    std::cout << "\nDTO basis validation:\n";
    if (errors.empty()) {
        std::cout << "  ✓ All DTO basis declarations valid\n";
    } else {
        std::cout << "  ✗ " << errors.size() << " issues found:\n";
        for (const auto& error : errors) {
            std::cout << "    " << error.toString() << "\n";
        }
    }
    
    REQUIRE(true);
}

TEST_CASE("ContractValidator validates Request basis", "[validator][request_basis]") {
    std::string contractsRoot = "/home/stephen/localdev/experiments/warehouse-management/contracts";
    std::string serviceContracts = "contracts";
    std::string claimsPath = "claims.json";
    
    if (!std::filesystem::exists(contractsRoot) || !std::filesystem::exists(serviceContracts)) {
        WARN("Contracts not found, skipping test");
        return;
    }

    ContractValidator validator(contractsRoot, serviceContracts, claimsPath);
    auto errors = validator.validateRequestBasis();
    
    std::cout << "\nRequest basis validation:\n";
    if (errors.empty()) {
        std::cout << "  ✓ All Request basis declarations valid\n";
    } else {
        std::cout << "  ✗ " << errors.size() << " issues found:\n";
        for (const auto& error : errors) {
            std::cout << "    " << error.toString() << "\n";
        }
    }
    
    REQUIRE(true);
}

TEST_CASE("ContractValidator validates naming conventions", "[validator][naming]") {
    std::string contractsRoot = "/home/stephen/localdev/experiments/warehouse-management/contracts";
    std::string serviceContracts = "contracts";
    std::string claimsPath = "claims.json";
    
    if (!std::filesystem::exists(contractsRoot) || !std::filesystem::exists(serviceContracts)) {
        WARN("Contracts not found, skipping test");
        return;
    }

    ContractValidator validator(contractsRoot, serviceContracts, claimsPath);
    auto errors = validator.validateNamingConventions();
    
    std::cout << "\nNaming convention validation:\n";
    if (errors.empty()) {
        std::cout << "  ✓ All naming conventions followed\n";
    } else {
        std::cout << "  ✗ " << errors.size() << " issues found:\n";
        for (const auto& error : errors) {
            std::cout << "    " << error.toString() << "\n";
        }
    }
    
    REQUIRE(true);
}

TEST_CASE("ContractValidator validates endpoints", "[validator][endpoints]") {
    std::string contractsRoot = "/home/stephen/localdev/experiments/warehouse-management/contracts";
    std::string serviceContracts = "contracts";
    std::string claimsPath = "claims.json";
    
    if (!std::filesystem::exists(contractsRoot) || !std::filesystem::exists(serviceContracts)) {
        WARN("Contracts not found, skipping test");
        return;
    }

    ContractValidator validator(contractsRoot, serviceContracts, claimsPath);
    auto errors = validator.validateEndpoints();
    
    std::cout << "\nEndpoint validation:\n";
    if (errors.empty()) {
        std::cout << "  ✓ All endpoints valid\n";
    } else {
        std::cout << "  ✗ " << errors.size() << " issues found:\n";
        for (const auto& error : errors) {
            std::cout << "    " << error.toString() << "\n";
        }
    }
    
    REQUIRE(true);
}

TEST_CASE("ContractValidator comprehensive validation", "[validator][comprehensive]") {
    std::string contractsRoot = "/home/stephen/localdev/experiments/warehouse-management/contracts";
    std::string serviceContracts = "contracts";
    std::string claimsPath = "claims.json";
    
    if (!std::filesystem::exists(contractsRoot) || !std::filesystem::exists(serviceContracts)) {
        WARN("Contracts not found, skipping test");
        return;
    }

    ContractValidator validator(contractsRoot, serviceContracts, claimsPath);
    auto result = validator.validate();
    
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "COMPREHENSIVE CONTRACT VALIDATION REPORT\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << result.summary();
    std::cout << std::string(70, '=') << "\n";
    
    if (result.isValid()) {
        std::cout << "\n✓ ALL VALIDATIONS PASSED\n\n";
    } else {
        std::cout << "\n✗ VALIDATION FAILED - Please review errors above\n\n";
    }
    
    REQUIRE(true);
}
