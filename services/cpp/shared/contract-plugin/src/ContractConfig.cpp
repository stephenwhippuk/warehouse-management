#include "contract-plugin/ContractConfig.hpp"
#include <cstdlib>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace contract {

ContractConfig ContractConfig::fromEnvironment() {
    ContractConfig config;
    
    if (const char* val = std::getenv("CONTRACT_CLAIMS_PATH")) {
        config.claimsPath = val;
    }
    
    if (const char* val = std::getenv("CONTRACT_CONTRACTS_PATH")) {
        config.contractsPath = val;
    }
    
    if (const char* val = std::getenv("CONTRACT_GLOBAL_CONTRACTS_PATH")) {
        config.globalContractsPath = val;
    }
    
    if (const char* val = std::getenv("CONTRACT_ENABLE_VALIDATION")) {
        config.enableValidation = (std::string(val) == "true" || std::string(val) == "1");
    }
    
    if (const char* val = std::getenv("CONTRACT_STRICT_MODE")) {
        config.strictMode = (std::string(val) == "true" || std::string(val) == "1");
    }
    
    if (const char* val = std::getenv("CONTRACT_LOG_VIOLATIONS")) {
        config.logViolations = (std::string(val) == "true" || std::string(val) == "1");
    }
    
    if (const char* val = std::getenv("CONTRACT_ENABLE_SWAGGER")) {
        config.enableSwagger = (std::string(val) == "true" || std::string(val) == "1");
    }
    
    if (const char* val = std::getenv("CONTRACT_ENABLE_CLAIMS")) {
        config.enableClaims = (std::string(val) == "true" || std::string(val) == "1");
    }
    
    return config;
}

ContractConfig ContractConfig::fromJson(const std::string& configJson) {
    ContractConfig config;
    
    try {
        json j = json::parse(configJson);
        
        if (j.contains("claimsPath")) config.claimsPath = j["claimsPath"];
        if (j.contains("contractsPath")) config.contractsPath = j["contractsPath"];
        if (j.contains("globalContractsPath")) config.globalContractsPath = j["globalContractsPath"];
        if (j.contains("enableValidation")) config.enableValidation = j["enableValidation"];
        if (j.contains("strictMode")) config.strictMode = j["strictMode"];
        if (j.contains("logViolations")) config.logViolations = j["logViolations"];
        if (j.contains("includeStackTrace")) config.includeStackTrace = j["includeStackTrace"];
        if (j.contains("enableSwagger")) config.enableSwagger = j["enableSwagger"];
        if (j.contains("swaggerTitle")) config.swaggerTitle = j["swaggerTitle"];
        if (j.contains("swaggerVersion")) config.swaggerVersion = j["swaggerVersion"];
        if (j.contains("swaggerDescription")) config.swaggerDescription = j["swaggerDescription"];
        if (j.contains("enableClaims")) config.enableClaims = j["enableClaims"];
    } catch (const json::exception& e) {
        // Return default config on parse error
    }
    
    return config;
}

} // namespace contract
