#ifndef CONTRACT_PLUGIN_ICLAIMS_LOADER_HPP
#define CONTRACT_PLUGIN_ICLAIMS_LOADER_HPP

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace contract {

/**
 * @brief Interface for loading claims.json from filesystem
 * 
 * Abstracts filesystem operations to allow mocking in tests.
 */
class IClaimsLoader {
public:
    virtual ~IClaimsLoader() = default;

    /**
     * @brief Load claims.json from specified path
     * @param claimsPath Path to claims.json file (relative or absolute)
     * @return Claims JSON if successfully loaded, std::nullopt otherwise
     */
    virtual std::optional<nlohmann::json> loadClaims(const std::string& claimsPath) = 0;
};

} // namespace contract

#endif // CONTRACT_PLUGIN_ICLAIMS_LOADER_HPP
