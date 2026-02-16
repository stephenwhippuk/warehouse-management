#pragma once

#include <http-framework/Middleware.hpp>
#include "ContractConfig.hpp"
#include <nlohmann/json.hpp>
#include <memory>

namespace contract {

/**
 * @brief Middleware that validates HTTP responses against contract definitions
 * 
 * Intercepts responses and checks:
 * - Response DTOs match contract definitions
 * - Required fields are present
 * - Field types match contract types
 * - Referenced entity identity fields included
 * 
 * Behavior:
 * - strictMode=false: Logs violations as warnings, allows response through
 * - strictMode=true: Returns 500 error with validation details
 */
class ContractValidationMiddleware : public http::Middleware {
public:
    /**
     * @brief Create validation middleware
     * @param config Configuration specifying contract paths and behavior
     */
    explicit ContractValidationMiddleware(const ContractConfig& config);
    
    /**
     * @brief Process request and validate response
     * @param ctx HTTP context
     * @param next Next middleware in pipeline
     */
    void process(http::HttpContext& ctx, std::function<void()> next) override;

private:
    ContractConfig config_;
    
    /**
     * @brief Validate response JSON against contract
     * @param responseJson JSON response body
     * @param endpoint Request endpoint path
     * @param method HTTP method
     * @return True if valid, false otherwise
     */
    bool validateResponse(const nlohmann::json& responseJson,
                         const std::string& endpoint,
                         const std::string& method);
    
    /**
     * @brief Load contract definition for endpoint
     * @param endpoint Request endpoint path
     * @param method HTTP method
     * @return Contract JSON or nullopt if not found
     */
    std::optional<nlohmann::json> loadEndpointContract(const std::string& endpoint,
                                                        const std::string& method);
    
    /**
     * @brief Validate field types match contract
     * @param responseJson JSON response
     * @param contract Contract definition
     * @return Validation error messages (empty if valid)
     */
    std::vector<std::string> validateFieldTypes(const nlohmann::json& responseJson,
                                                 const nlohmann::json& contract);
};

} // namespace contract
