#ifndef CONTRACT_VALIDATOR_CODEVALIDATOR_HPP
#define CONTRACT_VALIDATOR_CODEVALIDATOR_HPP

#include "contract_validator/ContractValidator.hpp"
#include "contract_validator/CppCodeParser.hpp"
#include <string>
#include <vector>

namespace contract_validator {

/**
 * @brief Validates both contracts and code implementations
 * 
 * Combines contract validation with C++ code analysis to ensure
 * implementations match their contract declarations.
 */
class CodeValidator {
public:
    struct CodeValidationError {
        std::string className;
        std::string methodName;
        std::string contractName;
        std::string message;
        std::string severity; // "error", "warning", "info"
    };
    
    struct CodeValidationResult {
        std::vector<CodeValidationError> errors;
        std::vector<CodeValidationError> warnings;
        std::vector<std::string> info;
        
        bool hasErrors() const { return !errors.empty(); }
        bool hasWarnings() const { return !warnings.empty(); }
    };
    
    /**
     * @brief Construct a code validator
     * @param contractsRoot Path to global contracts directory
     * @param serviceContracts Path to service-specific contracts directory
     * @param claims Path to claims.json file
     * @param sourcePath Path to service source code directory
     */
    CodeValidator(const std::string& contractsRoot,
                  const std::string& serviceContracts,
                  const std::string& claims,
                  const std::string& sourcePath);
    
    /**
     * @brief Validate contracts first, then code implementations
     * @return Combined validation result
     */
    CodeValidationResult validateAll();
    
    /**
     * @brief Validate that model classes implement toJson according to DTOs
     * @return Validation result for model serialization
     */
    CodeValidationResult validateModelSerialization();
    
    /**
     * @brief Validate controller endpoints match endpoint contracts
     * @return Validation result for controller implementations
     */
    CodeValidationResult validateControllers();
    
private:
    ContractValidator contractValidator_;
    std::string sourcePath_;
};

} // namespace contract_validator

#endif // CONTRACT_VALIDATOR_CODEVALIDATOR_HPP
