#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/error.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

namespace ocpp_gateway {
namespace common {

std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;
LogConfig Logger::current_config_;

bool Logger::initialize(const LogConfig& config) {
    try {
        current_config_ = config;
        
        // Create directory if it doesn't exist
        if (config.file_output && !config.log_file.empty()) {
            std::filesystem::path log_path(config.log_file);
            std::filesystem::create_directories(log_path.parent_path());
        }

        // Create sinks vector
        std::vector<spdlog::sink_ptr> sinks;
        
        // Add console sink if enabled
        if (config.console_output) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern(config.log_pattern);
            sinks.push_back(console_sink);
        }
        
        // Add file sink if enabled
        if (config.file_output && !config.log_file.empty()) {
            if (config.daily_rotation) {
                // Use daily rotation
                auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                    config.log_file, 0, 0);
                file_sink->set_pattern(config.log_pattern);
                sinks.push_back(file_sink);
            } else {
                // Use size-based rotation
                auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    config.log_file, 
                    config.max_size_mb * 1024 * 1024, 
                    config.max_files);
                file_sink->set_pattern(config.log_pattern);
                sinks.push_back(file_sink);
            }
        }
        
        // Create logger with all sinks
        logger_ = std::make_shared<spdlog::logger>("ocpp_gateway", sinks.begin(), sinks.end());
        
        // Set level
        setLevel(config.log_level);
        
        // Set as default logger
        spdlog::set_default_logger(logger_);
        
        // Compress old logs if enabled
        if (config.compress_logs && config.file_output && !config.log_file.empty()) {
            std::filesystem::path log_path(config.log_file);
            compressOldLogs(log_path.parent_path().string());
        }
        
        logger_->info("Logger initialized with level: {}", config.log_level);
        return true;
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        return false;
    } catch (const std::exception& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        return false;
    }
}

bool Logger::initialize(const std::string& log_level, 
                       const std::string& log_file,
                       size_t max_size_mb,
                       size_t max_files) {
    LogConfig config;
    config.log_level = log_level;
    config.log_file = log_file;
    config.max_size_mb = max_size_mb;
    config.max_files = max_files;
    return initialize(config);
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        // Return default logger if not initialized
        return spdlog::default_logger();
    }
    return logger_;
}

void Logger::setLevel(const std::string& level) {
    if (!logger_) {
        return;
    }
    
    logger_->set_level(stringToSpdLogLevel(level));
}

void Logger::setLevel(LogLevel level) {
    if (!logger_) {
        return;
    }
    
    switch (level) {
        case LogLevel::TRACE:
            logger_->set_level(spdlog::level::trace);
            break;
        case LogLevel::DEBUG:
            logger_->set_level(spdlog::level::debug);
            break;
        case LogLevel::INFO:
            logger_->set_level(spdlog::level::info);
            break;
        case LogLevel::WARNING:
            logger_->set_level(spdlog::level::warn);
            break;
        case LogLevel::ERROR:
            logger_->set_level(spdlog::level::err);
            break;
        case LogLevel::CRITICAL:
            logger_->set_level(spdlog::level::critical);
            break;
        default:
            logger_->warn("Unknown log level enum value. Using 'info'");
            logger_->set_level(spdlog::level::info);
            break;
    }
}

LogLevel Logger::stringToLogLevel(const std::string& level) {
    std::string level_lower = level;
    std::transform(level_lower.begin(), level_lower.end(), level_lower.begin(), 
                  [](unsigned char c) { return std::tolower(c); });
    
    if (level_lower == "trace") {
        return LogLevel::TRACE;
    } else if (level_lower == "debug") {
        return LogLevel::DEBUG;
    } else if (level_lower == "info") {
        return LogLevel::INFO;
    } else if (level_lower == "warn" || level_lower == "warning") {
        return LogLevel::WARNING;
    } else if (level_lower == "error" || level_lower == "err") {
        return LogLevel::ERROR;
    } else if (level_lower == "critical" || level_lower == "fatal") {
        return LogLevel::CRITICAL;
    } else {
        // Default to INFO
        return LogLevel::INFO;
    }
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:
            return "trace";
        case LogLevel::DEBUG:
            return "debug";
        case LogLevel::INFO:
            return "info";
        case LogLevel::WARNING:
            return "warn";
        case LogLevel::ERROR:
            return "error";
        case LogLevel::CRITICAL:
            return "critical";
        default:
            return "info";
    }
}

spdlog::level::level_enum Logger::stringToSpdLogLevel(const std::string& level) {
    std::string level_lower = level;
    std::transform(level_lower.begin(), level_lower.end(), level_lower.begin(), 
                  [](unsigned char c) { return std::tolower(c); });
    
    if (level_lower == "trace") {
        return spdlog::level::trace;
    } else if (level_lower == "debug") {
        return spdlog::level::debug;
    } else if (level_lower == "info") {
        return spdlog::level::info;
    } else if (level_lower == "warn" || level_lower == "warning") {
        return spdlog::level::warn;
    } else if (level_lower == "error" || level_lower == "err") {
        return spdlog::level::err;
    } else if (level_lower == "critical" || level_lower == "fatal") {
        return spdlog::level::critical;
    } else {
        // Default to INFO
        if (logger_) {
            logger_->warn("Unknown log level: {}. Using 'info'", level);
        }
        return spdlog::level::info;
    }
}

void Logger::flush() {
    if (logger_) {
        logger_->flush();
    }
}

// Helper function to check if string ends with suffix (C++17 compatible)
static bool endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

int Logger::compressOldLogs(const std::string& log_dir, const std::string& pattern) {
    try {
        int compressed_count = 0;
        
        // Check if directory exists
        if (!std::filesystem::exists(log_dir) || !std::filesystem::is_directory(log_dir)) {
            return 0;
        }
        
        // Find log files matching pattern
        for (const auto& entry : std::filesystem::directory_iterator(log_dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            
            std::string filename = entry.path().filename().string();
            
            // Skip already compressed files
            if (endsWith(filename, ".gz")) {
                continue;
            }
            
            // Check if file matches pattern (simple wildcard matching)
            if (pattern != "*" && pattern != "*.*") {
                // Convert wildcard pattern to regex-like check
                std::string pattern_regex = pattern;
                std::replace(pattern_regex.begin(), pattern_regex.end(), '*', '.');
                
                // Simple check - if pattern contains extension, check if filename ends with it
                size_t dot_pos = pattern_regex.find('.');
                if (dot_pos != std::string::npos) {
                    std::string ext = pattern_regex.substr(dot_pos);
                    if (!endsWith(filename, ext.substr(1))) {
                        continue;
                    }
                }
            }
            
            // Compress the file
            std::string input_file = entry.path().string();
            std::string output_file = input_file + ".gz";
            
            try {
                std::ifstream infile(input_file, std::ios_base::in | std::ios_base::binary);
                std::ofstream outfile(output_file, std::ios_base::out | std::ios_base::binary);
                
                boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
                in.push(boost::iostreams::gzip_compressor());
                in.push(infile);
                boost::iostreams::copy(in, outfile);
                
                // Close files
                outfile.close();
                infile.close();
                
                // Delete original file if compression was successful
                if (std::filesystem::exists(output_file) && 
                    std::filesystem::file_size(output_file) > 0) {
                    std::filesystem::remove(input_file);
                    compressed_count++;
                }
            } catch (const std::exception& ex) {
                if (logger_) {
                    logger_->warn("Failed to compress log file {}: {}", input_file, ex.what());
                }
                // Continue with next file
            }
        }
        
        return compressed_count;
    } catch (const std::exception& ex) {
        if (logger_) {
            logger_->error("Error compressing log files: {}", ex.what());
        }
        return -1;
    }
}

} // namespace common
} // namespace ocpp_gateway