#include "http-framework/Router.hpp"
#include <sstream>
#include <algorithm>

namespace http {

// ============================================================================
// Route Implementation
// ============================================================================

Route::Route(const std::string& method, 
            const std::string& pattern,
            EndpointHandler handler)
    : method_(method)
    , pattern_(pattern)
    , handler_(handler) {
    parsePattern();
}

bool Route::matches(const std::string& method, const std::string& path) const {
    if (method_ != method) {
        return false;
    }
    
    return std::regex_match(path, regex_);
}

std::map<std::string, std::string> Route::extractParameters(const std::string& path) const {
    std::map<std::string, std::string> result;
    
    std::smatch match;
    if (!std::regex_match(path, match, regex_)) {
        return result;
    }
    
    // First capture group is the whole match, skip it
    for (size_t i = 0; i < parameters_.size() && i + 1 < match.size(); ++i) {
        result[parameters_[i].name] = match[i + 1].str();
    }
    
    return result;
}

const EndpointHandler& Route::getHandler() const {
    return handler_;
}

const std::string& Route::getMethod() const {
    return method_;
}

const std::string& Route::getPattern() const {
    return pattern_;
}

const std::vector<RouteParameter>& Route::getParameters() const {
    return parameters_;
}

void Route::parsePattern() {
    // Split pattern into segments
    std::istringstream iss(pattern_);
    std::string segment;
    while (std::getline(iss, segment, '/')) {
        if (!segment.empty()) {
            segments_.push_back(segment);
        }
    }
    
    // Find parameters (enclosed in {})
    for (const auto& segment : segments_) {
        if (segment.front() == '{' && segment.back() == '}') {
            std::string paramDef = segment.substr(1, segment.length() - 2);
            
            // Check for constraint (e.g., {id:uuid})
            size_t colonPos = paramDef.find(':');
            RouteParameter param;
            if (colonPos != std::string::npos) {
                param.name = paramDef.substr(0, colonPos);
                std::string constraintStr = paramDef.substr(colonPos + 1);
                param.constraint = parseConstraint(constraintStr);
            } else {
                param.name = paramDef;
                param.constraint = RouteConstraint::None;
            }
            
            parameters_.push_back(param);
        }
    }
    
    // Build regex pattern
    std::string regexPattern = patternToRegex();
    regex_ = std::regex(regexPattern);
}

std::string Route::patternToRegex() const {
    std::string result = "^";
    
    for (const auto& segment : segments_) {
        result += "/";
        
        if (segment.front() == '{' && segment.back() == '}') {
            // Parameter segment
            std::string paramDef = segment.substr(1, segment.length() - 2);
            
            // Extract constraint
            RouteConstraint constraint = RouteConstraint::None;
            size_t colonPos = paramDef.find(':');
            if (colonPos != std::string::npos) {
                std::string constraintStr = paramDef.substr(colonPos + 1);
                constraint = parseConstraint(constraintStr);
            }
            
            // Add regex for parameter with constraint
            result += "(" + getConstraintRegex(constraint) + ")";
        } else {
            // Literal segment - escape special regex characters
            std::string escaped = segment;
            // Simple escape for common characters
            for (char c : {'$', '^', '.', '*', '+', '?', '(', ')', '[', ']', '{', '}', '|', '\\'}) {
                size_t pos = 0;
                while ((pos = escaped.find(c, pos)) != std::string::npos) {
                    escaped.insert(pos, 1, '\\');
                    pos += 2;
                }
            }
            result += escaped;
        }
    }
    
    result += "$";
    return result;
}

RouteConstraint Route::parseConstraint(const std::string& constraintStr) const {
    if (constraintStr == "uuid") return RouteConstraint::Uuid;
    if (constraintStr == "int") return RouteConstraint::Int;
    if (constraintStr == "alpha") return RouteConstraint::Alpha;
    if (constraintStr == "alphanum" || constraintStr == "alphanumeric") return RouteConstraint::AlphaNum;
    return RouteConstraint::None;
}

std::string Route::getConstraintRegex(RouteConstraint constraint) const {
    switch (constraint) {
        case RouteConstraint::Uuid:
            return "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}";
        case RouteConstraint::Int:
            return "[0-9]+";
        case RouteConstraint::Alpha:
            return "[a-zA-Z]+";
        case RouteConstraint::AlphaNum:
            return "[a-zA-Z0-9]+";
        case RouteConstraint::None:
        default:
            return "[^/]+";  // Match anything except /
    }
}

// ============================================================================
// Router Implementation
// ============================================================================

void Router::addRoute(const std::string& method, 
                     const std::string& pattern, 
                     EndpointHandler handler) {
    routes_.push_back(std::make_shared<Route>(method, pattern, handler));
}

void Router::addRoute(std::shared_ptr<Route> route) {
    routes_.push_back(route);
}

std::shared_ptr<Route> Router::findRoute(const std::string& method, const std::string& path) const {
    for (const auto& route : routes_) {
        if (route->matches(method, path)) {
            return route;
        }
    }
    return nullptr;
}

const std::vector<std::shared_ptr<Route>>& Router::getRoutes() const {
    return routes_;
}

void Router::clear() {
    routes_.clear();
}

size_t Router::size() const {
    return routes_.size();
}

// ============================================================================
// RouteBuilder Implementation
// ============================================================================

RouteBuilder::RouteBuilder(Router& router) : router_(router) {}

RouteBuilder& RouteBuilder::get(const std::string& pattern, EndpointHandler handler) {
    router_.addRoute("GET", pattern, handler);
    return *this;
}

RouteBuilder& RouteBuilder::post(const std::string& pattern, EndpointHandler handler) {
    router_.addRoute("POST", pattern, handler);
    return *this;
}

RouteBuilder& RouteBuilder::put(const std::string& pattern, EndpointHandler handler) {
    router_.addRoute("PUT", pattern, handler);
    return *this;
}

RouteBuilder& RouteBuilder::del(const std::string& pattern, EndpointHandler handler) {
    router_.addRoute("DELETE", pattern, handler);
    return *this;
}

RouteBuilder& RouteBuilder::patch(const std::string& pattern, EndpointHandler handler) {
    router_.addRoute("PATCH", pattern, handler);
    return *this;
}

RouteBuilder& RouteBuilder::options(const std::string& pattern, EndpointHandler handler) {
    router_.addRoute("OPTIONS", pattern, handler);
    return *this;
}

} // namespace http
