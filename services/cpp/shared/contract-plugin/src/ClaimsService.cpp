#include "contract-plugin/ClaimsService.hpp"
#include <spdlog/spdlog.h>

using json = nlohmann::json;

namespace contract {

ClaimsService::ClaimsService(const ContractConfig& config,
                             std::shared_ptr<IClaimsLoader> loader)
    : config_(config)
    , loader_(loader) {
    
    // Load claims using the loader
    auto loadedClaims = loader_->loadClaims(config_.claimsPath);
    if (loadedClaims) {
        claims_ = *loadedClaims;
        claimsLoaded_ = true;
    } else {
        spdlog::warn("ClaimsService: Failed to load claims.json from {}", config_.claimsPath);
        claimsLoaded_ = false;
    }
}

json ClaimsService::getAllClaims() {
    if (!claimsLoaded_) {
        throw std::runtime_error("Claims not loaded from " + config_.claimsPath);
    }
    return claims_;
}

json ClaimsService::getFulfilments() {
    if (!claimsLoaded_) {
        throw std::runtime_error("Claims not loaded from " + config_.claimsPath);
    }

    json result;
    if (claims_.contains("service")) {
        result["service"] = claims_["service"];
    }
    if (claims_.contains("version")) {
        result["version"] = claims_["version"];
    }
    if (claims_.contains("fulfilments")) {
        result["fulfilments"] = claims_["fulfilments"];
    } else {
        result["fulfilments"] = json::array();
    }

    return result;
}

json ClaimsService::getReferences() {
    if (!claimsLoaded_) {
        throw std::runtime_error("Claims not loaded from " + config_.claimsPath);
    }

    json result;
    if (claims_.contains("service")) {
        result["service"] = claims_["service"];
    }
    if (claims_.contains("version")) {
        result["version"] = claims_["version"];
    }
    if (claims_.contains("references")) {
        result["references"] = claims_["references"];
    } else {
        result["references"] = json::array();
    }

    return result;
}

json ClaimsService::getServices() {
    if (!claimsLoaded_) {
        throw std::runtime_error("Claims not loaded from " + config_.claimsPath);
    }

    json result;
    if (claims_.contains("service")) {
        result["service"] = claims_["service"];
    }
    if (claims_.contains("version")) {
        result["version"] = claims_["version"];
    }
    if (claims_.contains("serviceContracts")) {
        result["serviceContracts"] = claims_["serviceContracts"];
    } else {
        result["serviceContracts"] = json::array();
    }

    return result;
}

json ClaimsService::checkSupport(const std::string& type,
                                const std::string& name,
                                const std::string& version) {
    if (!claimsLoaded_) {
        throw std::runtime_error("Claims not loaded from " + config_.claimsPath);
    }
    
    // Validate type parameter
    if (type != "entity" && type != "service") {
        throw std::invalid_argument("Invalid type '" + type + "' - must be 'entity' or 'service'");
    }

    json response;
    response["type"] = type;
    response["contract"] = name;
    response["version"] = version;

    if (type == "entity") {
        bool fulfilled = false;
        bool supported = supportsEntity(name, version, fulfilled);
        response["supported"] = supported;
        if (supported) {
            response["fulfilled"] = fulfilled;
        }
    } else if (type == "service") {
        bool supported = supportsService(name, version);
        response["supported"] = supported;
    }

    return response;
}

bool ClaimsService::supportsEntity(const std::string& name,
                                  const std::string& version,
                                  bool& fulfilled) {
    fulfilled = false;

    // Check fulfilments
    if (claims_.contains("fulfilments")) {
        for (const auto& fulfilment : claims_["fulfilments"]) {
            if (fulfilment["contract"] == name) {
                const auto& versions = fulfilment["versions"];
                for (const auto& v : versions) {
                    if (v == version) {
                        fulfilled = true;
                        return true;
                    }
                }
            }
        }
    }

    // Check references
    if (claims_.contains("references")) {
        for (const auto& reference : claims_["references"]) {
            if (reference["contract"] == name) {
                const auto& versions = reference["versions"];
                for (const auto& v : versions) {
                    if (v == version) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool ClaimsService::supportsService(const std::string& name,
                                   const std::string& version) {
    if (!claims_.contains("serviceContracts")) {
        return false;
    }

    for (const auto& service : claims_["serviceContracts"]) {
        if (service["contract"] == name) {
            const auto& versions = service["versions"];
            for (const auto& v : versions) {
                if (v == version) {
                    return true;
                }
            }
        }
    }

    return false;
}

} // namespace contract
