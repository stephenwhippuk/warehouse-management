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

private:
    static std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace product::utils
