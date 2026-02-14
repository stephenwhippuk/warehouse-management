#pragma once

#include "product/Server.hpp"
#include "product/services/ProductService.hpp"
#include <memory>
#include <string>

namespace product {

/**
 * @brief Main application class
 * 
 * Manages service initialization, database connection, and HTTP server
 */
class Application {
public:
    Application();
    
    int run(int argc, char* argv[]);

private:
    std::unique_ptr<Server> server_;
    std::shared_ptr<services::ProductService> productService_;
    
    void initialize();
    void start();
    void stop();
};

}  // namespace product
