#pragma once

#include <exception>
#include <string>
#include <Poco/Net/HTTPResponse.h>

namespace http {

/**
 * @brief Base class for HTTP exceptions
 * 
 * All HTTP exceptions should derive from this class. It provides
 * a consistent way to associate HTTP status codes with exceptions.
 */
class HttpException : public std::exception {
public:
    /**
     * @brief Constructor
     * @param message Error message
     * @param statusCode HTTP status code
     */
    explicit HttpException(const std::string& message, int statusCode)
        : message_(message), statusCode_(statusCode) {}
    
    /**
     * @brief Get error message
     */
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    /**
     * @brief Get HTTP status code
     */
    int getStatusCode() const noexcept {
        return statusCode_;
    }
    
    /**
     * @brief Get error message as string
     */
    std::string getMessage() const {
        return message_;
    }

protected:
    std::string message_;
    int statusCode_;
};

/**
 * @brief 400 Bad Request - Invalid input or malformed request
 */
class BadRequestException : public HttpException {
public:
    explicit BadRequestException(const std::string& message)
        : HttpException(message, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST) {}
};

/**
 * @brief 401 Unauthorized - Missing or invalid authentication
 */
class UnauthorizedException : public HttpException {
public:
    explicit UnauthorizedException(const std::string& message = "Missing or invalid authentication")
        : HttpException(message, Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED) {}
};

/**
 * @brief 403 Forbidden - Valid auth but insufficient permissions
 */
class ForbiddenException : public HttpException {
public:
    explicit ForbiddenException(const std::string& message = "Access forbidden")
        : HttpException(message, Poco::Net::HTTPResponse::HTTP_FORBIDDEN) {}
};

/**
 * @brief 404 Not Found - Resource doesn't exist
 */
class NotFoundException : public HttpException {
public:
    explicit NotFoundException(const std::string& message)
        : HttpException(message, Poco::Net::HTTPResponse::HTTP_NOT_FOUND) {}
};

/**
 * @brief 409 Conflict - Business rule violation or state conflict
 */
class ConflictException : public HttpException {
public:
    explicit ConflictException(const std::string& message)
        : HttpException(message, Poco::Net::HTTPResponse::HTTP_CONFLICT) {}
};

/**
 * @brief 422 Unprocessable Entity - Validation failed
 */
class ValidationException : public HttpException {
public:
    explicit ValidationException(const std::string& message)
        : HttpException(message, Poco::Net::HTTPResponse::HTTP_UNPROCESSABLE_ENTITY) {}
};

/**
 * @brief 500 Internal Server Error - Unexpected server error
 */
class InternalServerErrorException : public HttpException {
public:
    explicit InternalServerErrorException(const std::string& message = "Internal server error")
        : HttpException(message, Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR) {}
};

/**
 * @brief 503 Service Unavailable - Service temporarily unavailable
 */
class ServiceUnavailableException : public HttpException {
public:
    explicit ServiceUnavailableException(const std::string& message = "Service temporarily unavailable")
        : HttpException(message, Poco::Net::HTTPResponse::HTTP_SERVICE_UNAVAILABLE) {}
};

} // namespace http
