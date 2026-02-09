# Contract Validator Library

A C++ library for validating contract compliance across microservices in the Warehouse Management System.

## Features

- **Contract Validation**: Validates service claims against entity contracts
- **Code Analysis**: Parses C++ code to verify implementations match contracts  
- **Field Exposure Validation**: Ensures fulfilled entity fields are exposed in DTOs
- **Identity Field Validation**: Verifies referenced entity identity fields are included
- **DTO Basis Validation**: Validates DTOs reference valid entities
- **Request Basis Validation**: Checks requests only modify declared entities
- **Naming Convention Validation**: Enforces entity-prefixed field naming
- **Endpoint Validation**: Validates endpoint request/response types exist

## Building

```bash
mkdir build && cd build
cmake ..
make
```

### Options

- `BUILD_SHARED_LIBS=ON` - Build as shared library (default)
- `BUILD_EXECUTABLE=ON` - Build validate-contracts tool (default)
- `BUILD_TESTS=OFF` - Build test suite (default: OFF)

## Using as a Library

### CMake Integration

```cmake
find_package(contract-validator REQUIRED)

add_executable(my-service main.cpp)
target_link_libraries(my-service
    PRIVATE
        ContractValidator::contract-validator
)
```

### Code Example

```cpp
#include <contract_validator/ContractValidator.hpp>

using namespace contract_validator;

int main() {
    ContractValidator validator(
        "/path/to/contracts",
        "/path/to/service/contracts",
        "/path/to/claims.json"
    );
    
    auto result = validator.validateAll();
    
    if (result.hasErrors()) {
        std::cerr << "Validation failed!" << std::endl;
        for (const auto& error : result.errors) {
            std::cerr << error.message << std::endl;
        }
        return 1;
    }
    
    return 0;
}
```

## Using the CLI Tool

```bash
# Validate a service
./validate-contracts \
    --contracts-root /path/to/contracts \
    --service-contracts /path/to/service/contracts \
    --claims /path/to/claims.json \
    --verbose

# JSON output for CI/CD
./validate-contracts --json > validation-report.json

# Fail on warnings
./validate-contracts --fail-on-warnings
```

## Dependencies

- C++20 compiler
- nlohmann/json
- spdlog

## Architecture

### Components

- **ContractReader**: Loads contract definitions (DTOs, Requests, Endpoints) from JSON
- **ContractValidator**: Validates claims against contracts
- **CppCodeParser**: Parses C++ source code to extract class and method information
- **CodeValidator**: Combines contract and code validation

### Validation Rules

1. **Field Exposure**: All fulfilled entity fields must be in DTOs or marked private
2. **Identity Fields**: Referenced entity identity fields must be included with proper naming
3. **DTO Basis**: DTOs must reference valid fulfilled/referenced entities
4. **Request Basis**: Command requests must only affect declared entities
5. **Naming Conventions**: Entity-prefixed fields must follow EntityField pattern
6. **Endpoint Types**: Response/request types must exist in service contracts

## License

See LICENSE file in repository root.
