#ifndef CONTRACT_PLUGIN_TESTS_MOCK_CLAIMS_LOADER_HPP
#define CONTRACT_PLUGIN_TESTS_MOCK_CLAIMS_LOADER_HPP

#include "contract-plugin/IClaimsLoader.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <map>

namespace contract::tests {

/**
 * @brief Mock implementation of IClaimsLoader for testing
 * 
 * Stores expected responses by path and returns them when loadClaims() is called.
 */
class MockClaimsLoader : public IClaimsLoader {
public:
    MockClaimsLoader() = default;

    /**
     * @brief Set the claims to return for a specific path
     */
    void setClaims(const std::string& path, const nlohmann::json& claims) {
        claims_[path] = claims;
    }

    /**
     * @brief Set loadClaims to fail for a specific path
     */
    void setFailure(const std::string& path) {
        claims_.erase(path);
        failures_.insert(path);
    }

    /**
     * @brief Load claims from mock storage
     */
    std::optional<nlohmann::json> loadClaims(const std::string& claimsPath) override {
        callCount_++;
        lastPath_ = claimsPath;

        if (failures_.count(claimsPath) > 0) {
            return std::nullopt;
        }

        auto it = claims_.find(claimsPath);
        if (it != claims_.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    /**
     * @brief Get number of times loadClaims was called
     */
    int getCallCount() const { return callCount_; }

    /**
     * @brief Get last path passed to loadClaims
     */
    std::string getLastPath() const { return lastPath_; }

    /**
     * @brief Reset mock state
     */
    void reset() {
        claims_.clear();
        failures_.clear();
        callCount_ = 0;
        lastPath_ = "";
    }

private:
    std::map<std::string, nlohmann::json> claims_;
    std::set<std::string> failures_;
    int callCount_ = 0;
    std::string lastPath_;
};

} // namespace contract::tests

#endif // CONTRACT_PLUGIN_TESTS_MOCK_CLAIMS_LOADER_HPP
