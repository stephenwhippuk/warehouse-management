#ifndef CONTRACT_VALIDATOR_CONTRACTVALIDATOR_HPP
#define CONTRACT_VALIDATOR_CONTRACTVALIDATOR_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace contract_validator {

/**
 * @brief Validates service contracts against entity contracts and claims
 * 
 * This validator ensures:
 * 1. All fulfilled fields are exposed in DTOs or marked private
 * 2. All identity fields from referenced entities are included in DTOs
 * 3. DTO basis matches service claims
 * 4. Request basis only includes fulfilled/referenced entities
 * 5. Naming conventions are followed (entity-prefixed fields)
 */
class ContractValidator {
public:
    struct ValidationError {
        enum class Severity {
            ERROR,
            WARNING,
            INFO
        };
        
        Severity severity;
        std::string category;  // "field_exposure", "identity_fields", "naming", "basis", etc.
        std::string message;
        std::string location;  // File or component where error occurred
        
        std::string toString() const;
    };

    struct ValidationResult {
        std::vector<ValidationError> errors;
        std::vector<ValidationError> warnings;
        std::vector<ValidationError> info;
        
        bool hasErrors() const { return !errors.empty(); }
        bool hasWarnings() const { return !warnings.empty(); }
        bool isValid() const { return !hasErrors(); }
        
        void addError(const ValidationError& error) {
            if (error.severity == ValidationError::Severity::ERROR) {
                errors.push_back(error);
            } else if (error.severity == ValidationError::Severity::WARNING) {
                warnings.push_back(error);
            } else {
                info.push_back(error);
            }
        }
        
        std::string summary() const;
    };

    struct EntityField {
        std::string name;
        std::string type;
        std::string classification;  // "identity", "core", "complete"
        bool required;
    };

    struct EntityContract {
        std::string name;
        std::string version;
        std::string owner;
        std::vector<EntityField> fields;
    };

    struct FieldClaim {
        std::string name;
        std::string status;  // "provided", "absent"
        std::string method;  // "direct", "derived"
        std::string access;  // "public", "private"
        bool encrypt;
    };

    struct FulfilmentClaim {
        std::string contract;
        std::vector<std::string> versions;
        std::string status;  // "fulfilled", "partial", "absent"
        std::vector<FieldClaim> fields;
    };

    struct ReferenceClaim {
        std::string contract;
        std::vector<std::string> versions;
        std::vector<std::string> requiredFields;
        std::vector<std::string> optionalFields;
    };

    struct ServiceClaims {
        std::string service;
        std::string version;
        std::vector<FulfilmentClaim> fulfilments;
        std::vector<ReferenceClaim> references;
    };

    /**
     * @brief Construct a validator with paths to contracts and claims
     * @param contractsRootPath Path to global contracts directory
     * @param serviceContractsPath Path to service's contracts directory
     * @param claimsPath Path to claims.json
     */
    ContractValidator(const std::string& contractsRootPath,
                     const std::string& serviceContractsPath,
                     const std::string& claimsPath);

    /**
     * @brief Run all validation checks
     * @return Validation result with errors, warnings, and info
     */
    ValidationResult validate();

    /**
     * @brief Validate field exposure rule
     * All fulfilled fields must be in DTOs or marked private
     * @return Validation errors
     */
    std::vector<ValidationError> validateFieldExposure();

    /**
     * @brief Validate identity field requirement
     * All identity fields from referenced entities must be in DTOs
     * @return Validation errors
     */
    std::vector<ValidationError> validateIdentityFields();

    /**
     * @brief Validate DTO basis against claims
     * DTO basis must reference fulfilled or referenced entities
     * @return Validation errors
     */
    std::vector<ValidationError> validateDtoBasis();

    /**
     * @brief Validate Request basis against claims
     * Request basis must only include fulfilled/referenced entities
     * @return Validation errors
     */
    std::vector<ValidationError> validateRequestBasis();

    /**
     * @brief Validate naming conventions
     * Entity-prefixed fields for referenced data
     * @return Validation errors
     */
    std::vector<ValidationError> validateNamingConventions();

    /**
     * @brief Validate endpoint definitions
     * Check parameters, responses, and consistency
     * @return Validation errors
     */
    std::vector<ValidationError> validateEndpoints();

    /**
     * @brief Load entity contract from global contracts
     * @param entityName Name of the entity
     * @return Entity contract or nullopt if not found
     */
    std::optional<EntityContract> loadEntityContract(const std::string& entityName);

private:
    std::string contractsRootPath_;
    std::string serviceContractsPath_;
    std::string claimsPath_;
    
    ServiceClaims claims_;
    std::map<std::string, EntityContract> entityContracts_;
    std::map<std::string, json> dtos_;
    std::map<std::string, json> requests_;
    std::vector<json> endpoints_;
    
    bool initialized_;

    /**
     * @brief Load all necessary data
     */
    void initialize();

    /**
     * @brief Load claims.json
     */
    ServiceClaims loadClaims();

    /**
     * @brief Load DTOs from service contracts
     */
    std::map<std::string, json> loadDtos();

    /**
     * @brief Load Requests from service contracts
     */
    std::map<std::string, json> loadRequests();

    /**
     * @brief Load Endpoints from service contracts
     */
    std::vector<json> loadEndpoints();

    /**
     * @brief Parse entity contract JSON
     */
    EntityContract parseEntityContract(const json& j);

    /**
     * @brief Get all fields from a DTO
     */
    std::set<std::string> getDtoFields(const json& dto);

    /**
     * @brief Check if a field name follows entity-prefixed convention
     */
    bool isEntityPrefixedField(const std::string& fieldName, const std::string& entityName);

    /**
     * @brief Get identity fields for an entity
     */
    std::vector<std::string> getIdentityFields(const EntityContract& entity);

    /**
     * @brief Check if entity is fulfilled by service
     */
    bool isFulfilledEntity(const std::string& entityName);

    /**
     * @brief Check if entity is referenced by service
     */
    bool isReferencedEntity(const std::string& entityName);

    /**
     * @brief Load JSON file
     */
    static json loadJsonFile(const std::string& filePath);
};

} // namespace contract_validator

#endif // CONTRACT_VALIDATOR_CONTRACTVALIDATOR_HPP
