#pragma once

#include <string>
#include <optional>

namespace contract {

/**
 * @brief Configuration for the Contract Plugin
 * 
 * Specifies paths to contract files and validation behavior.
 */
struct ContractConfig {
    // Path to claims.json file
    std::string claimsPath = "./claims.json";
    
    // Path to contracts directory (contains dtos/, requests/, events/, endpoints/)
    std::string contractsPath = "./contracts";
    
    // Path to global contracts directory (entities, types)
    std::string globalContractsPath = "../../contracts";
    
    // Validation behavior
    bool enableValidation = true;           // Enable contract validation middleware
    bool strictMode = false;                // Strict mode: 500 error on validation failure (vs warning)
    bool logViolations = true;              // Log validation violations
    bool includeStackTrace = false;         // Include stack trace in error responses
    
    // Swagger configuration
    bool enableSwagger = true;              // Enable /api/swagger.json endpoint
    std::string swaggerTitle = "API";       // OpenAPI title
    std::string swaggerVersion = "1.0.0";   // OpenAPI version
    std::string swaggerDescription = "";    // OpenAPI description
    
    // Claims configuration
    bool enableClaims = true;               // Enable /api/claims endpoint
    
    /**
     * @brief Load configuration from environment variables
     * 
     * Environment variables:
     * - CONTRACT_CLAIMS_PATH
     * - CONTRACT_CONTRACTS_PATH
     * - CONTRACT_GLOBAL_CONTRACTS_PATH
     * - CONTRACT_ENABLE_VALIDATION
     * - CONTRACT_STRICT_MODE
     * - CONTRACT_LOG_VIOLATIONS
     * - CONTRACT_ENABLE_SWAGGER
     * - CONTRACT_ENABLE_CLAIMS
     */
    static ContractConfig fromEnvironment();
    
    /**
     * @brief Load configuration from JSON
     * @param configJson JSON object with configuration
     */
    static ContractConfig fromJson(const std::string& configJson);
};

} // namespace contract
