#include "contract_validator/CodeValidator.hpp"
#include "contract_validator/Logger.hpp"

namespace contract_validator {

CodeValidator::CodeValidator(const std::string& contractsRoot,
                             const std::string& serviceContracts,
                             const std::string& claims,
                             const std::string& sourcePath)
    : contractValidator_(contractsRoot, serviceContracts, claims)
    , sourcePath_(sourcePath) {
}

CodeValidator::CodeValidationResult CodeValidator::validateAll() {
    CodeValidationResult result;
    
    // First validate contracts
    auto contractResult = contractValidator_.validate();
    
    // Convert contract errors to code validation errors
    for (const auto& error : contractResult.errors) {
        CodeValidationError codeError;
        codeError.message = error.message;
        codeError.severity = "error";
        codeError.contractName = error.location;
        result.errors.push_back(codeError);
    }
    
    for (const auto& warning : contractResult.warnings) {
        CodeValidationError codeWarning;
        codeWarning.message = warning.message;
        codeWarning.severity = "warning";
        codeWarning.contractName = warning.location;
        result.warnings.push_back(codeWarning);
    }
    
    // If contracts are invalid, don't proceed with code validation
    if (contractResult.hasErrors()) {
        Logger::error("Contract validation failed, skipping code validation");
        return result;
    }
    
    // TODO: Add code validation
    Logger::info("Code validation not yet implemented");
    
    return result;
}

CodeValidator::CodeValidationResult CodeValidator::validateModelSerialization() {
    CodeValidationResult result;
    
    // TODO: Implement model serialization validation
    // 1. Load DTOs from service contracts
    // 2. Parse model C++ files
    // 3. Compare toJson() fields against DTO definitions
    // 4. Report missing fields, extra fields, type mismatches
    
    Logger::info("Model serialization validation not yet implemented");
    
    return result;
}

CodeValidator::CodeValidationResult CodeValidator::validateControllers() {
    CodeValidationResult result;
    
    // TODO: Implement controller validation
    // 1. Load endpoint contracts
    // 2. Parse controller C++ files
    // 3. Verify endpoint methods exist
    // 4. Check return types match DTOs
    // 5. Verify parameter types match Requests
    
    Logger::info("Controller validation not yet implemented");
    
    return result;
}

} // namespace contract_validator
