#ifndef CONTRACT_PLUGIN_CLAIMS_LOADER_HPP
#define CONTRACT_PLUGIN_CLAIMS_LOADER_HPP

#include "contract-plugin/IClaimsLoader.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace contract {

/**
 * @brief Implementation of claims loader that reads from filesystem
 * 
 * Searches for claims.json in current directory and up to 5 parent directories.
 */
class ClaimsLoader : public IClaimsLoader {
public:
    ClaimsLoader() = default;

    /**
     * @brief Load claims.json from filesystem
     * 
     * Search strategy:
     * 1. Try direct path first (absolute or relative to cwd)
     * 2. Search up directory tree (up to 5 levels from cwd)
     * 
     * @param claimsPath Path to claims.json (e.g., "./claims.json", "claims.json")
     * @return Claims JSON if found and valid, std::nullopt otherwise
     */
    std::optional<nlohmann::json> loadClaims(const std::string& claimsPath) override;
};

} // namespace contract

#endif // CONTRACT_PLUGIN_CLAIMS_LOADER_HPP
