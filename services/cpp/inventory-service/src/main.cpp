#include "inventory/Application.hpp"
#include <iostream>
#include <exception>
#include <cstdlib>

int main(int argc, char** argv) {
    try {
        std::string configPath = "config/application.json";
        if (argc > 1) {
            configPath = argv[1];
        }
        
        inventory::Application app;
        app.initialize(configPath);
        app.run();
        
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
