#pragma once

#include <string>
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>

namespace ocpp_gateway {
namespace common {

/**
 * @enum LogLevel
 * @brief Enumeration of log levels
 */
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

/**
 * @struct LogConfig
 * @brief Configuration for the logger
 */
struct LogConfig {
    std::string log_level = "info";
    std::string log_file;
    size_t max_size_mb = 10;
    size_t max_files = 5;
    bool console_output = true;
    bool file_output = true;
    bool daily_rotation = false;
    bool compress_logs = true;
    std::string log_pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v";
};

/**
 * @class Logger
 * @brief Enhanced wrapper around spdlog for application logging
 */
class Logger {
public:
    /**
     * @brief Initialize the logger with detailed configuration
     * @param config Logger configuration
     * @return true if initialization was successful, false otherwise
     */
    static bool initialize(const LogConfig& config);

    /**
     * @brief Initialize the logger with basic configuration
     * @param log_level Log level (trace, debug, info, warn, error, critical)
     * @param log_file Path to log file
     * @param max_size_mb Maximum log file size in MB
     * @param max_files Maximum number of log files to keep
     * @return true if initialization was successful, false otherwise
     */
    static bool initialize(const std::string& log_level, 
                          const std::string& log_file,
                          size_t max_size_mb = 10,
                          size_t max_files = 5);

    /**
     * @brief Get the logger instance
     * @return Shared pointer to spdlog logger
     */
    static std::shared_ptr<spdlog::logger> get();

    /**
     * @brief Set the log level
     * @param level Log level (trace, debug, info, warn, error, critical)
     */
    static void setLevel(const std::string& level);
    
    /**
     * @brief Set the log level
     * @param level Log level enum
     */
    static void setLevel(LogLevel level);
    
    /**
     * @brief Convert string log level to enum
     * @param level Log level string
     * @return LogLevel enum
     */
    static LogLevel stringToLogLevel(const std::string& level);
    
    /**
     * @brief Convert enum log level to string
     * @param level LogLevel enum
     * @return Log level string
     */
    static std::string logLevelToString(LogLevel level);
    
    /**
     * @brief Flush all log messages
     */
    static void flush();
    
    /**
     * @brief Compress old log files
     * @param log_dir Directory containing log files
     * @param pattern Pattern to match log files
     * @return Number of files compressed
     */
    static int compressOldLogs(const std::string& log_dir, const std::string& pattern = "*.log.*");

private:
    static std::shared_ptr<spdlog::logger> logger_;
    static LogConfig current_config_;
    
    /**
     * @brief Convert string log level to spdlog level
     * @param level Log level string
     * @return spdlog::level::level_enum
     */
    static spdlog::level::level_enum stringToSpdLogLevel(const std::string& level);
};

/**
 * @brief Macro for logging with source file and line information
 */
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(ocpp_gateway::common::Logger::get(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(ocpp_gateway::common::Logger::get(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(ocpp_gateway::common::Logger::get(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(ocpp_gateway::common::Logger::get(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(ocpp_gateway::common::Logger::get(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(ocpp_gateway::common::Logger::get(), __VA_ARGS__)

} // namespace common
} // namespace ocpp_gateway