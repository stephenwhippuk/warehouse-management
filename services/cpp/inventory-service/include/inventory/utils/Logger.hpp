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
    
    // Convenience methods using runtime format strings to avoid
    // compile-time format string constraints in spdlog.
    template<typename... Args>
    static void info(const std::string& fmt, Args&&... args) {
        get()->info(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warn(const std::string& fmt, Args&&... args) {
        get()->warn(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(const std::string& fmt, Args&&... args) {
        get()->error(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(const std::string& fmt, Args&&... args) {
        get()->debug(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }
    
private:
    static std::shared_ptr<spdlog::logger> logger_;
};

} // namespace utils
} // namespace inventory
