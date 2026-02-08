#include "inventory/utils/Logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace inventory {
namespace utils {

std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;

void Logger::init(const std::string& logLevel) {
    if (!logger_) {
        // Reuse existing logger instance if it was already created in this process,
        // otherwise create a new one. This avoids duplicate logger name errors when
        // init() is called multiple times (e.g., during configuration loading and
        // later explicit initialization).
        auto existing = spdlog::get("inventory_service");
        if (existing) {
            logger_ = existing;
        } else {
            logger_ = spdlog::stdout_color_mt("inventory_service");
        }
    }
    
    // Set log level
    if (logLevel == "trace") logger_->set_level(spdlog::level::trace);
    else if (logLevel == "debug") logger_->set_level(spdlog::level::debug);
    else if (logLevel == "info") logger_->set_level(spdlog::level::info);
    else if (logLevel == "warn") logger_->set_level(spdlog::level::warn);
    else if (logLevel == "error") logger_->set_level(spdlog::level::err);
    else logger_->set_level(spdlog::level::info);
    
    logger_->info("Logger initialized with level: {}", logLevel);
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        init();
    }
    return logger_;
}

} // namespace utils
} // namespace inventory
