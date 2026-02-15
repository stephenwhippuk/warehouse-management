#include "inventory/controllers/ClaimsController.hpp"
#include "inventory/utils/Logger.hpp"
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

namespace inventory {
namespace controllers {

ClaimsController::ClaimsController() : ControllerBase("/api/v1/claims") {
    if (!loadClaims()) {
        utils::Logger::error("Failed to load claims.json");
    }

    // Register routes
    Get("/", [this](http::HttpContext& ctx) {
        return handleGetAllClaims(ctx);
    });

    Get("/fulfilments", [this](http::HttpContext& ctx) {
        return handleGetFulfilments(ctx);
    });

    Get("/references", [this](http::HttpContext& ctx) {
        return handleGetReferences(ctx);
    });

    Get("/services", [this](http::HttpContext& ctx) {
        return handleGetServices(ctx);
    });

    Get("/supports/{type:alpha}/{name:alphanum}/{version:alphanum}", [this](http::HttpContext& ctx) {
        return handleSupportsCheck(ctx);
    });
}

std::string ClaimsController::handleGetAllClaims(http::HttpContext& ctx) {
    (void)ctx;
    if (claims_.empty()) {
        ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        return json{{"error", "Claims not loaded"}, {"status", 500}}.dump();
    }

    return claims_.dump(2);
}

std::string ClaimsController::handleGetFulfilments(http::HttpContext& ctx) {
    (void)ctx;
    if (claims_.empty() || !claims_.contains("fulfilments")) {
        ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        return json{{"error", "Fulfilments not found"}, {"status", 500}}.dump();
    }

    json result;
    result["service"] = claims_["service"];
    result["version"] = claims_["version"];
    result["fulfilments"] = claims_["fulfilments"];

    return result.dump(2);
}

std::string ClaimsController::handleGetReferences(http::HttpContext& ctx) {
    (void)ctx;
    if (claims_.empty() || !claims_.contains("references")) {
        ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        return json{{"error", "References not found"}, {"status", 500}}.dump();
    }

    json result;
    result["service"] = claims_["service"];
    result["version"] = claims_["version"];
    result["references"] = claims_["references"];

    return result.dump(2);
}

std::string ClaimsController::handleGetServices(http::HttpContext& ctx) {
    (void)ctx;
    if (claims_.empty()) {
        ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        return json{{"error", "Service contracts not found"}, {"status", 500}}.dump();
    }

    json result;
    result["service"] = claims_["service"];
    result["version"] = claims_["version"];
    result["serviceContracts"] = claims_.value("serviceContracts", json::array());

    return result.dump(2);
}

std::string ClaimsController::handleSupportsCheck(http::HttpContext& ctx) {
    if (claims_.empty()) {
        ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        return json{{"error", "Claims not loaded"}, {"status", 500}}.dump();
    }

    std::string type = ctx.routeParams["type"];
    std::string name = ctx.routeParams["name"];
    std::string version = ctx.routeParams["version"];

    json result;
    result["requested"] = {
        {"type", type},
        {"name", name},
        {"version", version}
    };

    bool supported = false;
    bool fulfilled = false;
    std::string supportType = "none";

    if (type == "entity") {
        supported = supportsEntity(name, version, fulfilled);
        if (supported) {
            supportType = fulfilled ? "fulfilled" : "referenced";
        }
    } else if (type == "service") {
        supported = supportsService(name, version);
        if (supported) {
            supportType = "fulfilled";
        }
    } else {
        ctx.response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        return json{{"error", "Invalid type. Must be 'entity' or 'service'"}, {"status", 400}}.dump();
    }

    result["supported"] = supported;
    result["supportType"] = supportType;
    result["service"] = claims_["service"];
    result["serviceVersion"] = claims_["version"];

    return result.dump(2);
}

bool ClaimsController::loadClaims() {
    try {
        // Try to find claims.json in the current directory or parent directories
        std::filesystem::path searchPath = std::filesystem::current_path();
        std::filesystem::path claimsPath;

        // Search up to 5 levels up for claims.json
        for (int i = 0; i < 5; ++i) {
            auto candidate = searchPath / "claims.json";
            if (std::filesystem::exists(candidate)) {
                claimsPath = candidate;
                break;
            }
            searchPath = searchPath.parent_path();
        }

        if (claimsPath.empty()) {
            utils::Logger::error("claims.json not found");
            return false;
        }

        std::ifstream file(claimsPath);
        if (!file.is_open()) {
            utils::Logger::error("Failed to open claims.json");
            return false;
        }

        claims_ = json::parse(file);
        utils::Logger::info("Loaded claims from {}", claimsPath.string());
        return true;

    } catch (const std::exception& e) {
        utils::Logger::error("Error loading claims.json: {}", e.what());
        return false;
    }
}

bool ClaimsController::supportsEntity(const std::string& name,
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

bool ClaimsController::supportsService(const std::string& name, const std::string& version) {
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

} // namespace controllers
} // namespace inventory
