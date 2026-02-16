#include "contract-plugin/ContractValidationMiddleware.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <filesystem>

using json = nlohmann::json;

namespace contract {

ContractValidationMiddleware::ContractValidationMiddleware(const ContractConfig& config)
    : config_(config) {
    spdlog::info("ContractValidationMiddleware initialized (validation={}, strict={})",
                 config_.enableValidation, config_.strictMode);
}

void ContractValidationMiddleware::process(http::HttpContext& ctx, std::function<void()> next) {
    if (!config_.enableValidation) {
        next();
        return;
    }
    
    // Call next middleware/controller
    next();
    
    // Validate response (after handler has executed)
    try {
        // Get response body (if JSON)
        std::string contentType = ctx.getHeader("Content-Type", "");
        if (contentType.find("application/json") == std::string::npos) {
            return; // Skip non-JSON responses
        }
        
        // Get response body (TODO: This requires framework support to capture response body)
        // For now, we'll do a placeholder validation
        
        std::string endpoint = ctx.request.getURI();
        std::string method = ctx.request.getMethod();
        
        spdlog::debug("Validating response for {} {}", method, endpoint);
        
        // TODO: Implement actual validation against contract files
        // This is a placeholder that demonstrates the pattern
        
    } catch (const std::exception& e) {
        if (config_.strictMode) {
            ctx.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            json errorJson = {
                {"error", "Contract validation failed"},
                {"message", e.what()},
                {"endpoint", ctx.request.getURI()}
            };
            
            std::ostream& out = ctx.response.send();
            out << errorJson.dump();
        } else {
            spdlog::warn("Contract validation warning for {} {}: {}",
                        ctx.request.getMethod(),
                        ctx.request.getURI(),
                        e.what());
        }
    }
}

bool ContractValidationMiddleware::validateResponse(const json& responseJson,
                                                    const std::string& endpoint,
                                                    const std::string& method) {
    auto contract = loadEndpointContract(endpoint, method);
    if (!contract) {
        spdlog::debug("No contract found for {} {}", method, endpoint);
        return true; // No contract to validate against
    }
    
    auto errors = validateFieldTypes(responseJson, *contract);
    
    if (!errors.empty()) {
        if (config_.logViolations) {
            for (const auto& error : errors) {
                spdlog::warn("Contract violation: {}", error);
            }
        }
        return false;
    }
    
    return true;
}

std::optional<json> ContractValidationMiddleware::loadEndpointContract(
    const std::string& endpoint,
    const std::string& method) {
    
    // Construct path to endpoint contract
    // Example: contracts/endpoints/GetInventory.json
    std::filesystem::path endpointsDir = std::filesystem::path(config_.contractsPath) / "endpoints";
    
    if (!std::filesystem::exists(endpointsDir)) {
        return std::nullopt;
    }
    
    // Scan for matching endpoint
    for (const auto& entry : std::filesystem::directory_iterator(endpointsDir)) {
        if (entry.path().extension() != ".json") continue;
        
        try {
            std::ifstream file(entry.path());
            json contractJson;
            file >> contractJson;
            
            if (contractJson.contains("uri") && contractJson.contains("method")) {
                std::string contractUri = contractJson["uri"];
                std::string contractMethod = contractJson["method"];
                
                // TODO: Implement proper URI pattern matching
                if (contractMethod == method && endpoint.find(contractUri) != std::string::npos) {
                    return contractJson;
                }
            }
        } catch (const std::exception& e) {
            spdlog::warn("Failed to parse contract {}: {}", entry.path().string(), e.what());
        }
    }
    
    return std::nullopt;
}

std::vector<std::string> ContractValidationMiddleware::validateFieldTypes(
    const json& responseJson,
    const json& contract) {
    
    (void)contract;  // Suppress unused parameter warning - TODO: implement
    
    std::vector<std::string> errors;
    
    // TODO: Implement comprehensive field type validation
    // - Check required fields present
    // - Check field types match contract types (UUID, DateTime, etc.)
    // - Check referenced entity identity fields included
    // - Check enum values are valid
    
    // Placeholder implementation
    if (!responseJson.is_object() && !responseJson.is_array()) {
        errors.push_back("Response must be object or array");
    }
    
    return errors;
}

} // namespace contract
