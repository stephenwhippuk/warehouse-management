#pragma once

#include <spdlog/spdlog.h>
#include <memory>
#include <string>

namespace inventory {
namespace utils {

class Logger {
public:
    static void init(const std::string& logLevel = "info");
    static std::shared_ptr<spdlog::logger> get();
    
    // Convenience methods
    template<typename... Args>
    static void info(const std::string& msg, Args&&... args) {
        get()->info(msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warn(const std::string& msg, Args&&... args) {
        get()->warn(msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(const std::string& msg, Args&&... args) {
        get()->error(msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void debug(const std::string& msg, Args&&... args) {
        get()->debug(msg, std::forward<Args>(args)...);
    }
    
private:
    static std::shared_ptr<spdlog::logger> logger_;
};

} // namespace utils
} // namespace inventory
