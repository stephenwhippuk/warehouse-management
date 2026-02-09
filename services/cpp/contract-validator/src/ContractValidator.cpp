#include "contract_validator/ContractValidator.hpp"
#include "contract_validator/Logger.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

namespace contract_validator {

using ValidationResult = ContractValidator::ValidationResult;
using ValidationError = ContractValidator::ValidationError;

std::string ValidationError::toString() const {
    std::stringstream ss;
    ss << "[";
    switch (severity) {
        case Severity::ERROR: ss << "ERROR"; break;
        case Severity::WARNING: ss << "WARNING"; break;
        case Severity::INFO: ss << "INFO"; break;
    }
    ss << "] " << category << " - " << message;
    if (!location.empty()) {
        ss << " (at: " << location << ")";
    }
    return ss.str();
}

std::string ValidationResult::summary() const {
    std::stringstream ss;
    ss << "Validation Summary:\n";
    ss << "  Errors: " << errors.size() << "\n";
    ss << "  Warnings: " << warnings.size() << "\n";
    ss << "  Info: " << info.size() << "\n";
    
    if (!errors.empty()) {
        ss << "\nErrors:\n";
        for (const auto& error : errors) {
            ss << "  - " << error.toString() << "\n";
        }
    }
    
    if (!warnings.empty()) {
        ss << "\nWarnings:\n";
        for (const auto& warning : warnings) {
            ss << "  - " << warning.toString() << "\n";
        }
    }
    
    return ss.str();
}

ContractValidator::ContractValidator(const std::string& contractsRootPath,
                                    const std::string& serviceContractsPath,
                                    const std::string& claimsPath)
    : contractsRootPath_(contractsRootPath)
    , serviceContractsPath_(serviceContractsPath)
    , claimsPath_(claimsPath)
    , initialized_(false) {
}

ValidationResult ContractValidator::validate() {
    if (!initialized_) {
        initialize();
    }

    ValidationResult result;

    // Run all validation checks
    auto fieldExposureErrors = validateFieldExposure();
    for (const auto& error : fieldExposureErrors) {
        result.addError(error);
    }

    auto identityFieldErrors = validateIdentityFields();
    for (const auto& error : identityFieldErrors) {
        result.addError(error);
    }

    auto dtoBasisErrors = validateDtoBasis();
    for (const auto& error : dtoBasisErrors) {
        result.addError(error);
    }

    auto requestBasisErrors = validateRequestBasis();
    for (const auto& error : requestBasisErrors) {
        result.addError(error);
    }

    auto namingErrors = validateNamingConventions();
    for (const auto& error : namingErrors) {
        result.addError(error);
    }

    auto endpointErrors = validateEndpoints();
    for (const auto& error : endpointErrors) {
        result.addError(error);
    }

    return result;
}

std::vector<ContractValidator::ValidationError> ContractValidator::validateFieldExposure() {
    std::vector<ValidationError> errors;

    // For each fulfilment, check that all fields are either:
    // 1. Exposed in at least one DTO, OR
    // 2. Marked as private in claims

    for (const auto& fulfilment : claims_.fulfilments) {
        // Load entity contract
        auto entityOpt = loadEntityContract(fulfilment.contract);
        if (!entityOpt) {
            errors.push_back({
                ValidationError::Severity::ERROR,
                "field_exposure",
                "Entity contract not found: " + fulfilment.contract,
                claimsPath_
            });
            continue;
        }

        const auto& entity = *entityOpt;

        // Collect all fields exposed in DTOs
        std::set<std::string> exposedFields;
        for (const auto& [dtoName, dto] : dtos_) {
            // Check if DTO has this entity in its basis
            if (dto.contains("basis") && dto["basis"].is_array()) {
                for (const auto& basisItem : dto["basis"]) {
                    if (basisItem.contains("entity") && basisItem["entity"] == fulfilment.contract) {
                        // This DTO includes data from this entity
                        auto dtoFields = getDtoFields(dto);
                        exposedFields.insert(dtoFields.begin(), dtoFields.end());
                        break;
                    }
                }
            }
        }

        // Check each field in the entity
        for (const auto& entityField : entity.fields) {
            // Check if field is in claims
            auto claimIt = std::find_if(fulfilment.fields.begin(), fulfilment.fields.end(),
                [&entityField](const FieldClaim& claim) {
                    return claim.name == entityField.name;
                });

            if (claimIt == fulfilment.fields.end()) {
                errors.push_back({
                    ValidationError::Severity::ERROR,
                    "field_exposure",
                    "Field '" + entityField.name + "' from entity '" + fulfilment.contract + 
                    "' is not declared in claims",
                    claimsPath_
                });
                continue;
            }

            // If field is marked private, that's OK
            if (claimIt->access == "private") {
                continue;
            }

            // If field is public, it must be exposed in at least one DTO
            if (exposedFields.find(entityField.name) == exposedFields.end()) {
                errors.push_back({
                    ValidationError::Severity::ERROR,
                    "field_exposure",
                    "Field '" + entityField.name + "' from entity '" + fulfilment.contract + 
                    "' is marked public but not exposed in any DTO",
                    claimsPath_
                });
            }
        }
    }

    return errors;
}

std::vector<ContractValidator::ValidationError> ContractValidator::validateIdentityFields() {
    std::vector<ValidationError> errors;

    // For each reference, check that all identity fields are included in DTOs that use that entity
    for (const auto& reference : claims_.references) {
        auto entityOpt = loadEntityContract(reference.contract);
        if (!entityOpt) {
            errors.push_back({
                ValidationError::Severity::ERROR,
                "identity_fields",
                "Referenced entity contract not found: " + reference.contract,
                claimsPath_
            });
            continue;
        }

        const auto& entity = *entityOpt;
        auto identityFields = getIdentityFields(entity);

        // Find DTOs that reference this entity
        for (const auto& [dtoName, dto] : dtos_) {
            bool referencesEntity = false;
            if (dto.contains("basis") && dto["basis"].is_array()) {
                for (const auto& basisItem : dto["basis"]) {
                    if (basisItem.contains("entity") && basisItem["entity"] == reference.contract &&
                        basisItem.contains("type") && basisItem["type"] == "reference") {
                        referencesEntity = true;
                        break;
                    }
                }
            }

            if (referencesEntity) {
                // Check that all identity fields are present (with entity prefix)
                auto dtoFields = getDtoFields(dto);
                
                for (const auto& identityField : identityFields) {
                    std::string prefixedField = reference.contract + identityField;
                    
                    if (dtoFields.find(identityField) == dtoFields.end() &&
                        dtoFields.find(prefixedField) == dtoFields.end()) {
                        errors.push_back({
                            ValidationError::Severity::ERROR,
                            "identity_fields",
                            "Identity field '" + identityField + "' from referenced entity '" + 
                            reference.contract + "' is missing in DTO '" + dtoName + 
                            "' (expected '" + prefixedField + "')",
                            serviceContractsPath_ + "/dtos/" + dtoName + ".json"
                        });
                    }
                }
            }
        }
    }

    return errors;
}

std::vector<ContractValidator::ValidationError> ContractValidator::validateDtoBasis() {
    std::vector<ValidationError> errors;

    for (const auto& [dtoName, dto] : dtos_) {
        if (!dto.contains("basis") || !dto["basis"].is_array()) {
            errors.push_back({
                ValidationError::Severity::WARNING,
                "dto_basis",
                "DTO '" + dtoName + "' has no basis declaration",
                serviceContractsPath_ + "/dtos/" + dtoName + ".json"
            });
            continue;
        }

        // Check each basis entry
        for (const auto& basisItem : dto["basis"]) {
            if (!basisItem.contains("entity") || !basisItem.contains("type")) {
                errors.push_back({
                    ValidationError::Severity::ERROR,
                    "dto_basis",
                    "DTO '" + dtoName + "' has invalid basis entry (missing entity or type)",
                    serviceContractsPath_ + "/dtos/" + dtoName + ".json"
                });
                continue;
            }

            std::string entity = basisItem["entity"];
            std::string type = basisItem["type"];

            // Validate that entity is either fulfilled or referenced
            if (type == "fulfilment") {
                if (!isFulfilledEntity(entity)) {
                    errors.push_back({
                        ValidationError::Severity::ERROR,
                        "dto_basis",
                        "DTO '" + dtoName + "' declares fulfilment basis for '" + entity + 
                        "' but service does not fulfill this entity",
                        serviceContractsPath_ + "/dtos/" + dtoName + ".json"
                    });
                }
            } else if (type == "reference") {
                if (!isReferencedEntity(entity)) {
                    errors.push_back({
                        ValidationError::Severity::ERROR,
                        "dto_basis",
                        "DTO '" + dtoName + "' declares reference basis for '" + entity + 
                        "' but service does not reference this entity",
                        serviceContractsPath_ + "/dtos/" + dtoName + ".json"
                    });
                }
            } else {
                errors.push_back({
                    ValidationError::Severity::ERROR,
                    "dto_basis",
                    "DTO '" + dtoName + "' has invalid basis type '" + type + 
                    "' (must be 'fulfilment' or 'reference')",
                    serviceContractsPath_ + "/dtos/" + dtoName + ".json"
                });
            }
        }
    }

    return errors;
}

std::vector<ContractValidator::ValidationError> ContractValidator::validateRequestBasis() {
    std::vector<ValidationError> errors;

    for (const auto& [requestName, request] : requests_) {
        if (!request.contains("basis") || !request["basis"].is_array()) {
            // Basis is optional for queries, but required for commands
            if (request.value("type", "") == "command") {
                errors.push_back({
                    ValidationError::Severity::ERROR,
                    "request_basis",
                    "Command Request '" + requestName + "' must have a basis declaration",
                    serviceContractsPath_ + "/requests/" + requestName + ".json"
                });
            }
            continue;
        }

        // Check each basis entity
        for (const auto& entity : request["basis"]) {
            std::string entityName = entity.get<std::string>();
            
            // Entity must be fulfilled or referenced
            if (!isFulfilledEntity(entityName) && !isReferencedEntity(entityName)) {
                errors.push_back({
                    ValidationError::Severity::ERROR,
                    "request_basis",
                    "Request '" + requestName + "' declares basis entity '" + entityName + 
                    "' but service neither fulfills nor references this entity",
                    serviceContractsPath_ + "/requests/" + requestName + ".json"
                });
            }
        }
    }

    return errors;
}

std::vector<ContractValidator::ValidationError> ContractValidator::validateNamingConventions() {
    std::vector<ValidationError> errors;

    for (const auto& [dtoName, dto] : dtos_) {
        if (!dto.contains("fields") || !dto["fields"].is_array()) {
            continue;
        }

        if (!dto.contains("basis") || !dto["basis"].is_array()) {
            continue;
        }

        // Collect referenced entities
        std::set<std::string> referencedEntities;
        for (const auto& basisItem : dto["basis"]) {
            if (basisItem.contains("type") && basisItem["type"] == "reference" &&
                basisItem.contains("entity")) {
                referencedEntities.insert(basisItem["entity"].get<std::string>());
            }
        }

        // Check each field
        for (const auto& field : dto["fields"]) {
            if (!field.contains("name") || !field.contains("source")) {
                continue;
            }

            std::string fieldName = field["name"];
            std::string source = field["source"];

            // Parse source (e.g., "Product.id" or "Inventory.quantity")
            auto dotPos = source.find('.');
            if (dotPos != std::string::npos) {
                std::string sourceEntity = source.substr(0, dotPos);
                
                // If source is from a referenced entity, field should be entity-prefixed
                if (referencedEntities.find(sourceEntity) != referencedEntities.end()) {
                    if (!isEntityPrefixedField(fieldName, sourceEntity)) {
                        errors.push_back({
                            ValidationError::Severity::ERROR,
                            "naming_convention",
                            "Field '" + fieldName + "' in DTO '" + dtoName + 
                            "' should be entity-prefixed (expected '" + sourceEntity + 
                            "' prefix) because it comes from referenced entity '" + sourceEntity + "'",
                            serviceContractsPath_ + "/dtos/" + dtoName + ".json"
                        });
                    }
                }
            }
        }
    }

    return errors;
}

std::vector<ContractValidator::ValidationError> ContractValidator::validateEndpoints() {
    std::vector<ValidationError> errors;

    for (const auto& endpoint : endpoints_) {
        std::string endpointName = endpoint.value("name", "unknown");
        std::string endpointFile = serviceContractsPath_ + "/endpoints/" + endpointName + ".json";

        // Validate that response DTOs exist
        if (endpoint.contains("responses") && endpoint["responses"].is_array()) {
            for (const auto& response : endpoint["responses"]) {
                if (response.contains("type") && !response["type"].get<std::string>().empty()) {
                    std::string responseType = response["type"];
                    
                    if (dtos_.find(responseType) == dtos_.end() && 
                        requests_.find(responseType) == requests_.end()) {
                        errors.push_back({
                            ValidationError::Severity::ERROR,
                            "endpoint_validation",
                            "Endpoint '" + endpointName + "' references undefined response type '" + 
                            responseType + "'",
                            endpointFile
                        });
                    }
                }
            }
        }

        // Validate that request body types exist
        if (endpoint.contains("parameters") && endpoint["parameters"].is_array()) {
            for (const auto& param : endpoint["parameters"]) {
                if (param.contains("location") && param["location"] == "Body" &&
                    param.contains("type")) {
                    std::string bodyType = param["type"];
                    
                    if (requests_.find(bodyType) == requests_.end() && 
                        dtos_.find(bodyType) == dtos_.end()) {
                        errors.push_back({
                            ValidationError::Severity::ERROR,
                            "endpoint_validation",
                            "Endpoint '" + endpointName + "' references undefined request body type '" + 
                            bodyType + "'",
                            endpointFile
                        });
                    }
                }
            }
        }
    }

    return errors;
}

std::optional<ContractValidator::EntityContract> ContractValidator::loadEntityContract(const std::string& entityName) {
    // Check cache
    if (entityContracts_.find(entityName) != entityContracts_.end()) {
        return entityContracts_[entityName];
    }

    // Load from file
    std::string path = contractsRootPath_ + "/entities/v1/" + entityName + ".json";
    if (!fs::exists(path)) {
        return std::nullopt;
    }

    try {
        json j = loadJsonFile(path);
        EntityContract contract = parseEntityContract(j);
        entityContracts_[entityName] = contract;
        return contract;
    } catch (const std::exception& e) {
        Logger::error("Failed to load entity contract {}: {}", entityName, e.what());
        return std::nullopt;
    }
}

void ContractValidator::initialize() {
    Logger::info("Initializing ContractValidator");
    
    claims_ = loadClaims();
    dtos_ = loadDtos();
    requests_ = loadRequests();
    endpoints_ = loadEndpoints();
    
    // Preload entity contracts for all fulfilments and references
    for (const auto& fulfilment : claims_.fulfilments) {
        loadEntityContract(fulfilment.contract);
    }
    for (const auto& reference : claims_.references) {
        loadEntityContract(reference.contract);
    }
    
    initialized_ = true;
    Logger::info("ContractValidator initialized: {} fulfilments, {} references, {} DTOs, {} requests, {} endpoints",
                claims_.fulfilments.size(), claims_.references.size(), dtos_.size(), 
                requests_.size(), endpoints_.size());
}

ContractValidator::ServiceClaims ContractValidator::loadClaims() {
    json j = loadJsonFile(claimsPath_);
    
    ServiceClaims claims;
    claims.service = j.value("service", "");
    claims.version = j.value("version", "");
    
    // Load fulfilments
    if (j.contains("fulfilments") && j["fulfilments"].is_array()) {
        for (const auto& fulfilmentJson : j["fulfilments"]) {
            FulfilmentClaim fulfilment;
            fulfilment.contract = fulfilmentJson.value("contract", "");
            fulfilment.status = fulfilmentJson.value("status", "");
            
            if (fulfilmentJson.contains("versions") && fulfilmentJson["versions"].is_array()) {
                for (const auto& version : fulfilmentJson["versions"]) {
                    fulfilment.versions.push_back(version.get<std::string>());
                }
            }
            
            if (fulfilmentJson.contains("fields") && fulfilmentJson["fields"].is_array()) {
                for (const auto& fieldJson : fulfilmentJson["fields"]) {
                    FieldClaim field;
                    field.name = fieldJson.value("name", "");
                    field.status = fieldJson.value("status", "");
                    field.method = fieldJson.value("method", "");
                    
                    if (fieldJson.contains("security")) {
                        field.access = fieldJson["security"].value("access", "public");
                        field.encrypt = fieldJson["security"].value("encrypt", false);
                    } else {
                        field.access = "public";
                        field.encrypt = false;
                    }
                    
                    fulfilment.fields.push_back(field);
                }
            }
            
            claims.fulfilments.push_back(fulfilment);
        }
    }
    
    // Load references
    if (j.contains("references") && j["references"].is_array()) {
        for (const auto& referenceJson : j["references"]) {
            ReferenceClaim reference;
            reference.contract = referenceJson.value("contract", "");
            
            if (referenceJson.contains("versions") && referenceJson["versions"].is_array()) {
                for (const auto& version : referenceJson["versions"]) {
                    reference.versions.push_back(version.get<std::string>());
                }
            }
            
            if (referenceJson.contains("requiredFields") && referenceJson["requiredFields"].is_array()) {
                for (const auto& field : referenceJson["requiredFields"]) {
                    reference.requiredFields.push_back(field.get<std::string>());
                }
            }
            
            if (referenceJson.contains("optionalFields") && referenceJson["optionalFields"].is_array()) {
                for (const auto& field : referenceJson["optionalFields"]) {
                    reference.optionalFields.push_back(field.get<std::string>());
                }
            }
            
            claims.references.push_back(reference);
        }
    }
    
    return claims;
}

std::map<std::string, json> ContractValidator::loadDtos() {
    std::map<std::string, json> dtos;
    std::string dtosPath = serviceContractsPath_ + "/dtos";
    
    if (!fs::exists(dtosPath)) {
        Logger::warn("DTOs directory not found: {}", dtosPath);
        return dtos;
    }
    
    for (const auto& entry : fs::directory_iterator(dtosPath)) {
        if (entry.path().extension() == ".json") {
            try {
                json dto = loadJsonFile(entry.path().string());
                std::string name = dto.value("name", "");
                if (!name.empty()) {
                    dtos[name] = dto;
                }
            } catch (const std::exception& e) {
                Logger::error("Failed to load DTO from {}: {}", entry.path().string(), e.what());
            }
        }
    }
    
    return dtos;
}

std::map<std::string, json> ContractValidator::loadRequests() {
    std::map<std::string, json> requests;
    std::string requestsPath = serviceContractsPath_ + "/requests";
    
    if (!fs::exists(requestsPath)) {
        Logger::warn("Requests directory not found: {}", requestsPath);
        return requests;
    }
    
    for (const auto& entry : fs::directory_iterator(requestsPath)) {
        if (entry.path().extension() == ".json") {
            try {
                json request = loadJsonFile(entry.path().string());
                std::string name = request.value("name", "");
                if (!name.empty()) {
                    requests[name] = request;
                }
            } catch (const std::exception& e) {
                Logger::error("Failed to load Request from {}: {}", entry.path().string(), e.what());
            }
        }
    }
    
    return requests;
}

std::vector<json> ContractValidator::loadEndpoints() {
    std::vector<json> endpoints;
    std::string endpointsPath = serviceContractsPath_ + "/endpoints";
    
    if (!fs::exists(endpointsPath)) {
        Logger::warn("Endpoints directory not found: {}", endpointsPath);
        return endpoints;
    }
    
    for (const auto& entry : fs::directory_iterator(endpointsPath)) {
        if (entry.path().extension() == ".json") {
            try {
                json endpoint = loadJsonFile(entry.path().string());
                endpoints.push_back(endpoint);
            } catch (const std::exception& e) {
                Logger::error("Failed to load Endpoint from {}: {}", entry.path().string(), e.what());
            }
        }
    }
    
    return endpoints;
}

ContractValidator::EntityContract ContractValidator::parseEntityContract(const json& j) {
    EntityContract contract;
    contract.name = j.value("name", "");
    contract.version = j.value("version", "");
    contract.owner = j.value("owner", "");
    
    if (j.contains("fields") && j["fields"].is_array()) {
        for (const auto& fieldJson : j["fields"]) {
            EntityField field;
            field.name = fieldJson.value("name", "");
            field.type = fieldJson.value("type", "");
            field.classification = fieldJson.value("classification", "complete");
            field.required = fieldJson.value("required", false);
            contract.fields.push_back(field);
        }
    }
    
    return contract;
}

std::set<std::string> ContractValidator::getDtoFields(const json& dto) {
    std::set<std::string> fields;
    
    if (dto.contains("fields") && dto["fields"].is_array()) {
        for (const auto& field : dto["fields"]) {
            if (field.contains("name")) {
                fields.insert(field["name"].get<std::string>());
            }
        }
    }
    
    return fields;
}

bool ContractValidator::isEntityPrefixedField(const std::string& fieldName, const std::string& entityName) {
    // Check if field name starts with entity name (case-sensitive)
    return fieldName.find(entityName) == 0 && fieldName.length() > entityName.length();
}

std::vector<std::string> ContractValidator::getIdentityFields(const EntityContract& entity) {
    std::vector<std::string> identityFields;
    
    for (const auto& field : entity.fields) {
        if (field.classification == "identity") {
            identityFields.push_back(field.name);
        }
    }
    
    return identityFields;
}

bool ContractValidator::isFulfilledEntity(const std::string& entityName) {
    for (const auto& fulfilment : claims_.fulfilments) {
        if (fulfilment.contract == entityName) {
            return true;
        }
    }
    return false;
}

bool ContractValidator::isReferencedEntity(const std::string& entityName) {
    for (const auto& reference : claims_.references) {
        if (reference.contract == entityName) {
            return true;
        }
    }
    return false;
}

json ContractValidator::loadJsonFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }
    
    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        throw std::runtime_error("JSON parse error in " + filePath + ": " + e.what());
    }
    
    return j;
}

} // namespace contract_validator
