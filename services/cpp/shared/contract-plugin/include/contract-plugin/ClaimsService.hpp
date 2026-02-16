#ifndef CONTRACT_PLUGIN_CLAIMS_SERVICE_HPP
#define CONTRACT_PLUGIN_CLAIMS_SERVICE_HPP

#include "contract-plugin/IClaimsService.hpp"
#include "contract-plugin/IClaimsLoader.hpp"
#include "contract-plugin/ContractConfig.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <memory>

namespace contract {

/**
 * @brief Implementation of claims operations
 * 
 * Loads and caches claims.json from filesystem, provides support checking
 * for entities and service contracts.
 */
class ClaimsService : public IClaimsService {
public:
    /**
     * @brief Construct service and load claims
     * @param config Configuration with claimsPath
     * @param loader Claims loader for filesystem operations (mockable)
     */
    ClaimsService(const ContractConfig& config,
                  std::shared_ptr<IClaimsLoader> loader);

    nlohmann::json getAllClaims() override;
    nlohmann::json getFulfilments() override;
    nlohmann::json getReferences() override;
    nlohmann::json getServices() override;
    nlohmann::json checkSupport(const std::string& type,
                               const std::string& name,
                               const std::string& version) override;

private:
    ContractConfig config_;
    std::shared_ptr<IClaimsLoader> loader_;
    nlohmann::json claims_;
    bool claimsLoaded_ = false;

    /**
     * @brief Check if entity contract is supported
     * @param name Entity contract name
     * @param version Entity contract version
     * @param fulfilled Set to true if entity is fulfilled (vs referenced)
     * @return true if entity is supported (fulfilled or referenced)
     */
    bool supportsEntity(const std::string& name,
                       const std::string& version,
                       bool& fulfilled);

    /**
     * @brief Check if service contract is supported
     * @param name Service contract name
     * @param version Service contract version
     * @return true if service contract is supported
     */
    bool supportsService(const std::string& name,
                        const std::string& version);
};

} // namespace contract

#endif // CONTRACT_PLUGIN_CLAIMS_SERVICE_HPP
