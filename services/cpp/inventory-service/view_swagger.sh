#!/bin/bash
# Quick test script to view the generated swagger spec

cd "$(dirname "$0")"

# Create a simple test program that outputs the swagger spec
cat > /tmp/test_swagger.cpp << 'EOF'
#include "inventory/utils/SwaggerGenerator.hpp"
#include <iostream>

int main() {
    try {
        auto spec = inventory::utils::SwaggerGenerator::generateSpecFromContracts(
            "Inventory Service API",
            "1.0.0",
            "API for managing warehouse inventory",
            "contracts"
        );
        
        std::cout << spec.dump(2) << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
EOF

# Compile it
g++ -std=c++20 -I./include \
    /tmp/test_swagger.cpp \
    src/utils/SwaggerGenerator.cpp \
    src/utils/ContractReader.cpp \
    src/utils/Logger.cpp \
    src/utils/Config.cpp \
    -lspdlog -o /tmp/test_swagger

# Run it
/tmp/test_swagger
