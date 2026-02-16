#pragma once

/**
 * @brief Test calculator interface used by test plugin
 * 
 * This interface must be shared between the plugin and tests
 * to ensure type compatibility via std::type_index.
 */
class ICalculator {
public:
    virtual ~ICalculator() = default;
    virtual int add(int a, int b) const = 0;
    virtual int multiply(int a, int b) const = 0;
};
