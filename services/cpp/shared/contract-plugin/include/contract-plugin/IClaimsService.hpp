#ifndef CONTRACT_PLUGIN_ICLAIMS_SERVICE_HPP
#define CONTRACT_PLUGIN_ICLAIMS_SERVICE_HPP

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace contract {

/**
 * @brief Interface for claims operations
 * 
 * Provides access to service claims information including fulfilments,
 * references, and service contracts. Used by ClaimsController to delegate
 * business logic.
 */
class IClaimsService {
public:
    virtual ~IClaimsService() = default;

    /**
     * @brief Get full claims document
     * @return Complete claims JSON
     * @throws std::runtime_error if claims not loaded
     */
    virtual nlohmann::json getAllClaims() = 0;

    /**
     * @brief Get fulfilments section of claims
     * @return JSON with service, version, and fulfilments array
     * @throws std::runtime_error if claims not loaded
     */
    virtual nlohmann::json getFulfilments() = 0;

    /**
     * @brief Get references section of claims
     * @return JSON with service, version, and references array
     * @throws std::runtime_error if claims not loaded
     */
    virtual nlohmann::json getReferences() = 0;

    /**
     * @brief Get service contracts section of claims
     * @return JSON with service, version, and serviceContracts array
     * @throws std::runtime_error if claims not loaded
     */
    virtual nlohmann::json getServices() = 0;

    /**
     * @brief Check if service supports a specific contract
     * @param type Contract type ("entity" or "service")
     * @param name Contract name
     * @param version Contract version
     * @return JSON with support results
     */
    virtual nlohmann::json checkSupport(const std::string& type,
                                       const std::string& name,
                                       const std::string& version) = 0;
};

} // namespace contract

#endif // CONTRACT_PLUGIN_ICLAIMS_SERVICE_HPP
