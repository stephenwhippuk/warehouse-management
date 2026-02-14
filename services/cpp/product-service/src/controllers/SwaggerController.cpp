#include "product/controllers/SwaggerController.hpp"
#include "product/utils/SwaggerGenerator.hpp"
#include "product/utils/Logger.hpp"

namespace product::controllers {

void SwaggerController::handleRequest(Poco::Net::HTTPServerRequest& request,
                                     Poco::Net::HTTPServerResponse& response) {
    if (auto logger = utils::Logger::getLogger()) logger->debug("Swagger request: {} {}", request.getMethod(), request.getURI());
    
    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
    response.setContentType("application/json");
    
    auto spec = utils::SwaggerGenerator::generateSpec("1.0.0");
    response.send() << spec.dump(2);
}

}  // namespace product::controllers
