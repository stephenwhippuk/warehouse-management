#include "inventory/controllers/ClaimsController.hpp"
#include "inventory/utils/Logger.hpp"
#include <Poco/URI.h>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

namespace inventory {
namespace controllers {

ClaimsController::ClaimsController() {
    if (!loadClaims()) {
        utils::Logger::error("Failed to load claims.json");
    }
}

void ClaimsController::handleRequest(Poco::Net::HTTPServerRequest& request,
                                     Poco::Net::HTTPServerResponse& response) {
    const std::string& uri = request.getURI();
    const std::string& method = request.getMethod();

    utils::Logger::debug("ClaimsController: {} {}", method, uri);

    // Only support GET requests
    if (method != "GET") {
        sendErrorResponse(response, "Method not allowed", 405);
        return;
    }

    Poco::URI pocoUri(uri);
    std::vector<std::string> pathSegments;
    pocoUri.getPathSegments(pathSegments);

    // Expected paths:
    // /api/v1/claims
    // /api/v1/claims/fulfilments
    // /api/v1/claims/references
    // /api/v1/claims/services
    // /api/v1/claims/supports/{type}/{name}/{version}

    if (pathSegments.size() < 3) {
        sendErrorResponse(response, "Invalid path", 400);
        return;
    }

    // pathSegments: [api, v1, claims, ...]
    if (pathSegments[0] != "api" || pathSegments[1] != "v1" || pathSegments[2] != "claims") {
        sendErrorResponse(response, "Invalid path", 400);
        return;
    }

    // /api/v1/claims
    if (pathSegments.size() == 3) {
        handleGetAllClaims(response);
        return;
    }

    const std::string& subPath = pathSegments[3];

    // /api/v1/claims/fulfilments
    if (subPath == "fulfilments" && pathSegments.size() == 4) {
        handleGetFulfilments(response);
        return;
    }

    // /api/v1/claims/references
    if (subPath == "references" && pathSegments.size() == 4) {
        handleGetReferences(response);
        return;
    }

    // /api/v1/claims/services
    if (subPath == "services" && pathSegments.size() == 4) {
        handleGetServices(response);
        return;
    }

    // /api/v1/claims/supports/{type}/{name}/{version}
    if (subPath == "supports" && pathSegments.size() == 7) {
        const std::string& type = pathSegments[4];
        const std::string& name = pathSegments[5];
        const std::string& version = pathSegments[6];
        handleSupportsCheck(type, name, version, response);
        return;
    }

    sendErrorResponse(response, "Not found", 404);
}

void ClaimsController::handleGetAllClaims(Poco::Net::HTTPServerResponse& response) {
    if (claims_.empty()) {
        sendErrorResponse(response, "Claims not loaded", 500);
        return;
    }

    sendJsonResponse(response, claims_, 200);
}

void ClaimsController::handleGetFulfilments(Poco::Net::HTTPServerResponse& response) {
    if (claims_.empty() || !claims_.contains("fulfilments")) {
        sendErrorResponse(response, "Fulfilments not found", 500);
        return;
    }

    json result;
    result["service"] = claims_["service"];
    result["version"] = claims_["version"];
    result["fulfilments"] = claims_["fulfilments"];

    sendJsonResponse(response, result, 200);
}

void ClaimsController::handleGetReferences(Poco::Net::HTTPServerResponse& response) {
    if (claims_.empty() || !claims_.contains("references")) {
        sendErrorResponse(response, "References not found", 500);
        return;
    }

    json result;
    result["service"] = claims_["service"];
    result["version"] = claims_["version"];
    result["references"] = claims_["references"];

    sendJsonResponse(response, result, 200);
}

void ClaimsController::handleGetServices(Poco::Net::HTTPServerResponse& response) {
    if (claims_.empty()) {
        sendErrorResponse(response, "Service contracts not found", 500);
        return;
    }

    json result;
    result["service"] = claims_["service"];
    result["version"] = claims_["version"];
    result["serviceContracts"] = claims_.value("serviceContracts", json::array());

    sendJsonResponse(response, result, 200);
}

void ClaimsController::handleSupportsCheck(const std::string& type,
                                           const std::string& name,
                                           const std::string& version,
                                           Poco::Net::HTTPServerResponse& response) {
    if (claims_.empty()) {
        sendErrorResponse(response, "Claims not loaded", 500);
        return;
    }

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
        sendErrorResponse(response, "Invalid type. Must be 'entity' or 'service'", 400);
        return;
    }

    result["supported"] = supported;
    result["supportType"] = supportType;
    result["service"] = claims_["service"];
    result["serviceVersion"] = claims_["version"];

    sendJsonResponse(response, result, 200);
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

void ClaimsController::sendJsonResponse(Poco::Net::HTTPServerResponse& response,
                                        const json& jsonData,
                                        int statusCode) {
    response.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(statusCode));
    response.setContentType("application/json");
    response.setChunkedTransferEncoding(true);

    std::ostream& out = response.send();
    out << jsonData.dump(2);
}

void ClaimsController::sendErrorResponse(Poco::Net::HTTPServerResponse& response,
                                         const std::string& message,
                                         int statusCode) {
    json error;
    error["error"] = message;
    error["status"] = statusCode;

    sendJsonResponse(response, error, statusCode);
}

} // namespace controllers
} // namespace inventory
