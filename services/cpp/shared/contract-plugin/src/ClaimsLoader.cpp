#include "contract-plugin/ClaimsLoader.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace contract {

std::optional<json> ClaimsLoader::loadClaims(const std::string& claimsPath) {
    try {
        fs::path resolvedPath;
        
        // Try direct path first
        fs::path directPath(claimsPath);
        if (fs::exists(directPath)) {
            resolvedPath = directPath;
        } else {
            // Search up directory tree (up to 5 levels)
            fs::path searchPath = fs::current_path();
            for (int i = 0; i < 5; ++i) {
                fs::path candidatePath = searchPath / claimsPath;
                if (fs::exists(candidatePath)) {
                    resolvedPath = candidatePath;
                    break;
                }
                searchPath = searchPath.parent_path();
            }
        }

        if (resolvedPath.empty()) {
            spdlog::error("ClaimsLoader: claims.json not found at path: {}", claimsPath);
            return std::nullopt;
        }

        std::ifstream file(resolvedPath);
        if (!file.is_open()) {
            spdlog::error("ClaimsLoader: Failed to open claims.json at: {}", resolvedPath.string());
            return std::nullopt;
        }

        json claims = json::parse(file);
        spdlog::info("ClaimsLoader: Loaded claims from {}", resolvedPath.string());
        return claims;

    } catch (const json::exception& e) {
        spdlog::error("ClaimsLoader: JSON parse error: {}", e.what());
        return std::nullopt;
    } catch (const std::exception& e) {
        spdlog::error("ClaimsLoader: Error loading claims.json: {}", e.what());
        return std::nullopt;
    }
}

} // namespace contract
