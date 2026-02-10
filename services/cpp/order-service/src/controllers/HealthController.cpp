#include "order/controllers/HealthController.hpp"
#include "order/utils/Logger.hpp"
#include <Poco/Net/HTTPResponse.h>
#include <chrono>
#include <sstream>

namespace order {
namespace controllers {

void HealthController::handleRequest(
    Poco::Net::HTTPServerRequest& request,
    Poco::Net::HTTPServerResponse& response
) {
    utils::Logger::debug("Health check requested");
    sendHealthResponse(response);
}

void HealthController::sendHealthResponse(Poco::Net::HTTPServerResponse& response) {
    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
    response.setContentType("application/json");
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    
    std::ostringstream oss;
    oss << "{"
        << R"("status":"healthy",)"
        << R"("service":"order-service",)"
        << R"("timestamp":)" << timestamp << ","
        << R"("version":"1.0.0")"
        << "}";
    
    std::ostream& out = response.send();
    out << oss.str();
}

} // namespace controllers
} // namespace order
