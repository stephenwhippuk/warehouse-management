#include "http-framework/ExceptionFilter.hpp"
#include <iostream>

namespace http {

// ============================================================================
// DefaultExceptionFilter Implementation
// ============================================================================

bool DefaultExceptionFilter::handleException(HttpContext& ctx, const std::exception& e) {
    int statusCode = Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR;
    std::string message = "Internal server error";
    
    // Try to cast to HttpException first
    if (const auto* httpEx = dynamic_cast<const HttpException*>(&e)) {
        statusCode = httpEx->getStatusCode();
        message = httpEx->getMessage();
    }
    // Handle JSON parsing exceptions
    else if (const auto* jsonParseEx = dynamic_cast<const json::parse_error*>(&e)) {
        statusCode = Poco::Net::HTTPResponse::HTTP_BAD_REQUEST;
        message = std::string("Invalid JSON: ") + jsonParseEx->what();
    }
    // Handle JSON type exceptions
    else if (const auto* jsonTypeEx = dynamic_cast<const json::type_error*>(&e)) {
        statusCode = Poco::Net::HTTPResponse::HTTP_BAD_REQUEST;
        message = std::string("JSON type error: ") + jsonTypeEx->what();
    }
    // Handle standard validation exceptions
    else if (const auto* invalidArgEx = dynamic_cast<const std::invalid_argument*>(&e)) {
        statusCode = Poco::Net::HTTPResponse::HTTP_BAD_REQUEST;
        message = invalidArgEx->what();
    }
    // Handle runtime errors
    else if (const auto* runtimeEx = dynamic_cast<const std::runtime_error*>(&e)) {
        statusCode = Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR;
        message = runtimeEx->what();
    }
    // Generic exception
    else {
        statusCode = Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR;
        message = std::string("Unexpected error: ") + e.what();
    }
    
    // Log the error (in production, use proper logging)
    std::cerr << "[ERROR] " << ctx.getMethod() << " " << ctx.getPath() 
              << " - " << statusCode << ": " << message << std::endl;
    
    // Create error response
    json errorJson = createErrorResponse(message, statusCode, ctx.getPath());
    
    // Send response
    ctx.response.setStatus(static_cast<Poco::Net::HTTPResponse::HTTPStatus>(statusCode));
    ctx.sendJson(errorJson);
    
    return true;  // Always handles exceptions
}

json DefaultExceptionFilter::createErrorResponse(const std::string& message, 
                                                  int status, 
                                                  const std::string& path) const {
    return json{
        {"error", message},
        {"status", status},
        {"path", path},
        {"timestamp", std::time(nullptr)}
    };
}

// ============================================================================
// CompositeExceptionFilter Implementation
// ============================================================================

void CompositeExceptionFilter::addFilter(std::shared_ptr<IExceptionFilter> filter) {
    filters_.push_back(filter);
}

bool CompositeExceptionFilter::handleException(HttpContext& ctx, const std::exception& e) {
    // Try each filter in order
    for (const auto& filter : filters_) {
        if (filter->handleException(ctx, e)) {
            return true;  // Filter handled the exception
        }
    }
    
    // No filter handled it
    return false;
}

size_t CompositeExceptionFilter::size() const {
    return filters_.size();
}

} // namespace http
