#include "order/utils/Logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace order::utils {

std::shared_ptr<spdlog::logger> Logger::logger_;

void Logger::init(const std::string& logFile, Level level, bool consoleOutput) {
    std::vector<spdlog::sink_ptr> sinks;
    
    // Console sink
    if (consoleOutput) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(convertLevel(level));
        sinks.push_back(console_sink);
    }
    
    // File sink
    if (!logFile.empty()) {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFile, 1024 * 1024 * 10, 3); // 10MB, 3 files
        file_sink->set_level(convertLevel(level));
        sinks.push_back(file_sink);
    }
    
    // Create logger
    logger_ = std::make_shared<spdlog::logger>("order", sinks.begin(), sinks.end());
    logger_->set_level(convertLevel(level));
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");
    
    spdlog::register_logger(logger_);
    spdlog::set_default_logger(logger_);
}

std::shared_ptr<spdlog::logger> Logger::get(const std::string& name) {
    if (!logger_) {
        init();
    }
    return logger_;
}

void Logger::setLevel(Level level) {
    if (logger_) {
        logger_->set_level(convertLevel(level));
    }
}

spdlog::level::level_enum Logger::convertLevel(Level level) {
    switch (level) {
        case Level::Trace: return spdlog::level::trace;
        case Level::Debug: return spdlog::level::debug;
        case Level::Info: return spdlog::level::info;
        case Level::Warn: return spdlog::level::warn;
        case Level::Error: return spdlog::level::err;
        case Level::Critical: return spdlog::level::critical;
    }
    return spdlog::level::info;
}

} // namespace order::utils
