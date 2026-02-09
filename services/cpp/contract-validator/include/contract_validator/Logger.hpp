#ifndef CONTRACT_VALIDATOR_LOGGER_HPP
#define CONTRACT_VALIDATOR_LOGGER_HPP

#include <iostream>
#include <sstream>
#include <string>

namespace contract_validator {

/**
 * @brief Simple logger for contract validator library
 * 
 * Minimal logging utility that outputs to std::cerr.
 * Services using this library can provide their own logging by
 * setting a custom callback.
 */
class Logger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };
    
    static Level minLevel;
    
    template<typename... Args>
    static void debug(const std::string& format, Args&&... args) {
        if (minLevel <= Level::DEBUG) {
            log("DEBUG", format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    static void info(const std::string& format, Args&&... args) {
        if (minLevel <= Level::INFO) {
            log("INFO", format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    static void warn(const std::string& format, Args&&... args) {
        if (minLevel <= Level::WARN) {
            log("WARN", format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    static void error(const std::string& format, Args&&... args) {
        if (minLevel <= Level::ERROR) {
            log("ERROR", format, std::forward<Args>(args)...);
        }
    }
    
private:
    template<typename... Args>
    static void log(const std::string& level, const std::string& format, Args&&... args) {
        std::cerr << "[" << level << "] " << formatString(format, std::forward<Args>(args)...) << std::endl;
    }
    
    // Base case: no more arguments
    static std::string formatString(const std::string& format) {
        return format;
    }
    
    // Recursive case: replace first {} with argument
    template<typename T, typename... Args>
    static std::string formatString(const std::string& format, T&& value, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos == std::string::npos) {
            // No more placeholders, just return format + remaining args as string
            std::ostringstream oss;
            oss << format << " " << value;
            return formatString(oss.str(), std::forward<Args>(args)...);
        }
        
        std::ostringstream oss;
        oss << format.substr(0, pos) << value << format.substr(pos + 2);
        return formatString(oss.str(), std::forward<Args>(args)...);
    }
};

} // namespace contract_validator

#endif // CONTRACT_VALIDATOR_LOGGER_HPP
