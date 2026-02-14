#pragma once

#include <string>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

namespace product::utils {

/**
 * @brief Service-to-service authentication
 */
class Auth {
public:
    static void setServiceApiKey(const std::string& key);
    
    static bool authorizeServiceRequest(Poco::Net::HTTPServerRequest& request,
                                       Poco::Net::HTTPServerResponse& response);
    
private:
    static std::string serviceApiKey_;
    
    static std::string extractApiKey(Poco::Net::HTTPServerRequest& request);
};

}  // namespace product::utils
