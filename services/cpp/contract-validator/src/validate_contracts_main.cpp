/**
 * @file validate_contracts_main.cpp
 * @brief Command-line tool for validating service contracts
 * 
 * Usage: ./validate-contracts [options]
 * 
 * Options:
 *   --contracts-root <path>    Path to global contracts directory
 *   --service-contracts <path>  Path to service contracts directory (default: contracts)
 *   --claims <path>             Path to claims.json (default: claims.json)
 *   --fail-on-warnings         Exit with error code if warnings are found
 *   --json                     Output results in JSON format
 *   --verbose                  Enable verbose output
 */

#include "contract_validator/ContractValidator.hpp"
#include "contract_validator/Logger.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

struct Args {
    std::string contractsRoot;
    std::string serviceContracts = "contracts";
    std::string claims = "claims.json";
    bool failOnWarnings = false;
    bool json = false;
    bool verbose = false;
};

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --contracts-root <path>     Path to global contracts directory (REQUIRED)\n";
    std::cout << "  --service-contracts <path>  Path to service contracts directory (default: contracts)\n";
    std::cout << "  --claims <path>             Path to claims.json (default: claims.json)\n";
    std::cout << "  --fail-on-warnings          Exit with error code if warnings are found\n";
    std::cout << "  --json                      Output results in JSON format\n";
    std::cout << "  --verbose                   Enable verbose output\n";
    std::cout << "  --help                      Show this help message\n";
}

Args parseArgs(int argc, char* argv[]) {
    Args args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        } else if (arg == "--contracts-root" && i + 1 < argc) {
            args.contractsRoot = argv[++i];
        } else if (arg == "--service-contracts" && i + 1 < argc) {
            args.serviceContracts = argv[++i];
        } else if (arg == "--claims" && i + 1 < argc) {
            args.claims = argv[++i];
        } else if (arg == "--fail-on-warnings") {
            args.failOnWarnings = true;
        } else if (arg == "--json") {
            args.json = true;
        } else if (arg == "--verbose") {
            args.verbose = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            exit(1);
        }
    }
    
    return args;
}

std::string resultToJson(const contract_validator::ContractValidator::ValidationResult& result) {
    using json = nlohmann::json;
    
    json j;
    j["valid"] = result.isValid();
    j["errorCount"] = result.errors.size();
    j["warningCount"] = result.warnings.size();
    j["infoCount"] = result.info.size();
    
    j["errors"] = json::array();
    for (const auto& error : result.errors) {
        j["errors"].push_back({
            {"severity", "error"},
            {"category", error.category},
            {"message", error.message},
            {"location", error.location}
        });
    }
    
    j["warnings"] = json::array();
    for (const auto& warning : result.warnings) {
        j["warnings"].push_back({
            {"severity", "warning"},
            {"category", warning.category},
            {"message", warning.message},
            {"location", warning.location}
        });
    }
    
    j["info"] = json::array();
    for (const auto& info : result.info) {
        j["info"].push_back({
            {"severity", "info"},
            {"category", info.category},
            {"message", info.message},
            {"location", info.location}
        });
    }
    
    return j.dump(2);
}

int main(int argc, char* argv[]) {
    Args args = parseArgs(argc, argv);
    
    // contracts-root is required
    if (args.contractsRoot.empty()) {
        std::cerr << "Error: --contracts-root is required\n";
        printUsage(argv[0]);
        return 1;
    }
    
    // Set log level
    if (args.verbose) {
        contract_validator::Logger::minLevel = contract_validator::Logger::Level::DEBUG;
        contract_validator::Logger::info("Contract Validator");
        contract_validator::Logger::info("Contracts root: {}", args.contractsRoot);
        contract_validator::Logger::info("Service contracts: {}", args.serviceContracts);
        contract_validator::Logger::info("Claims file: {}", args.claims);
    }
    
    try {
        contract_validator::ContractValidator validator(
            args.contractsRoot,
            args.serviceContracts,
            args.claims
        );
        
        auto result = validator.validate();
        
        if (args.json) {
            std::cout << resultToJson(result) << std::endl;
        } else {
            std::cout << "\n" << std::string(70, '=') << "\n";
            std::cout << "CONTRACT VALIDATION REPORT\n";
            std::cout << std::string(70, '=') << "\n";
            std::cout << result.summary();
            std::cout << std::string(70, '=') << "\n";
            
            if (result.isValid()) {
                std::cout << "\n✓ ALL VALIDATIONS PASSED\n\n";
            } else {
                std::cout << "\n✗ VALIDATION FAILED\n\n";
            }
        }
        
        // Determine exit code
        if (!result.isValid()) {
            return 1;
        }
        if (args.failOnWarnings && result.hasWarnings()) {
            return 2;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        if (args.json) {
            nlohmann::json error = {
                {"valid", false},
                {"error", e.what()}
            };
            std::cout << error.dump(2) << std::endl;
        } else {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        return 3;
    }
}
