#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

namespace product::utils {

/**
 * @brief Logging utility wrapper around spdlog
 * 
 * Simple logger interface that forwards to spdlog's underlying logger.
 * Note: Uses spdlog's fmt library for formatting.
 */
class Logger {
public:
    /**
     * @brief Initialize the logger with the specified level
     */
    static void initialize(const std::string& level = "info");
    
    /**
     * @brief Get the underlying spdlog logger (for direct use)
     */
    static std::shared_ptr<spdlog::logger> getLogger() { return logger_; }
    
    // Convenience methods using runtime format strings
    template<typename... Args>
    static void info(const std::string& fmt, Args&&... args) {
        getLogger()->info(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warn(const std::string& fmt, Args&&... args) {
        getLogger()->warn(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(const std::string& fmt, Args&&... args) {
        getLogger()->error(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(const std::string& fmt, Args&&... args) {
        getLogger()->debug(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
    }

private:
    static std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace product::utils
