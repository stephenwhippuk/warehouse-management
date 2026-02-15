#include "http-framework/HttpContext.hpp"
#include <istream>
#include <sstream>

namespace http {

// ============================================================================
// QueryParams Implementation
// ============================================================================

QueryParams::QueryParams(const Poco::URI::QueryParameters& params) {
    for (const auto& param : params) {
        params_[param.first] = param.second;
    }
}

std::string QueryParams::get(const std::string& key, const std::string& defaultValue) const {
    auto it = params_.find(key);
    return (it != params_.end()) ? it->second : defaultValue;
}

bool QueryParams::has(const std::string& key) const {
    return params_.find(key) != params_.end();
}

const std::map<std::string, std::string>& QueryParams::all() const {
    return params_;
}

std::optional<int> QueryParams::getInt(const std::string& key) const {
    auto it = params_.find(key);
    if (it == params_.end()) {
        return std::nullopt;
    }
    try {
        return std::stoi(it->second);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> QueryParams::getBool(const std::string& key) const {
    auto it = params_.find(key);
    if (it == params_.end()) {
        return std::nullopt;
    }
    const std::string& value = it->second;
    if (value == "true" || value == "1" || value == "yes") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no") {
        return false;
    }
    return std::nullopt;
}

// ============================================================================
// HttpContext Implementation
// ============================================================================

HttpContext::HttpContext(Poco::Net::HTTPServerRequest& req,
                        Poco::Net::HTTPServerResponse& res,
                        const Poco::URI::QueryParameters& queryParams)
    : request(req)
    , response(res)
    , queryParams(queryParams)
    , uri(req.getURI()) {
}

std::string HttpContext::getBodyAsString() {
    if (bodyRead_) {
        return bodyCache_;
    }
    
    std::istream& bodyStream = request.stream();
    std::ostringstream oss;
    oss << bodyStream.rdbuf();
    bodyCache_ = oss.str();
    bodyRead_ = true;
    
    return bodyCache_;
}

json HttpContext::getBodyAsJson() {
    std::string body = getBodyAsString();
    if (body.empty()) {
        return json::object();
    }
    return json::parse(body);
}

void HttpContext::sendJson(const std::string& jsonStr,
                           Poco::Net::HTTPResponse::HTTPStatus status) {
    response.setStatus(status);
    response.setContentType("application/json");
    response.setContentLength(jsonStr.length());
    
    std::ostream& out = response.send();
    out << jsonStr;
    out.flush();
}

void HttpContext::sendJson(const json& jsonObj,
                           Poco::Net::HTTPResponse::HTTPStatus status) {
    sendJson(jsonObj.dump(), status);
}

void HttpContext::sendError(const std::string& message,
                            Poco::Net::HTTPResponse::HTTPStatus status,
                            const std::string& requestId) {
    json errorJson = {
        {"error", true},
        {"message", message},
        {"status", static_cast<int>(status)},
        {"path", getPath()}
    };
    
    if (!requestId.empty()) {
        errorJson["requestId"] = requestId;
    }
    
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    errorJson["timestamp"] = ss.str();
    
    sendJson(errorJson, status);
}

void HttpContext::sendNotFound(const std::string& message) {
    sendError(message, Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
}

void HttpContext::sendCreated(const std::string& location, const std::string& jsonStr) {
    response.set("Location", location);
    sendJson(jsonStr, Poco::Net::HTTPResponse::HTTP_CREATED);
}

void HttpContext::sendNoContent() {
    response.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
    response.send();
}

std::string HttpContext::getPath() const {
    return uri.getPath();
}

std::string HttpContext::getMethod() const {
    return request.getMethod();
}

void HttpContext::setStatus(Poco::Net::HTTPResponse::HTTPStatus status) {
    response.setStatus(status);
}

void HttpContext::setHeader(const std::string& name, const std::string& value) {
    response.set(name, value);
}

std::string HttpContext::getHeader(const std::string& name, const std::string& defaultValue) const {
    return request.has(name) ? request.get(name) : defaultValue;
}

bool HttpContext::hasHeader(const std::string& name) const {
    return request.has(name);
}

} // namespace http
