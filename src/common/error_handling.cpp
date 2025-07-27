#include "ocpp_gateway/common/error_handling.h"
#include "ocpp_gateway/common/logger.h"
#include <chrono>
#include <json/json.h>
#include <sstream>

namespace ocpp_gateway {
namespace common {

// ErrorInfo implementation
ErrorInfo::ErrorInfo(ErrorCategory cat, ErrorLevel lvl, const std::string& code, 
                     const std::string& msg, const std::string& comp)
    : category(cat), level(lvl), error_code(code), message(msg), component(comp) {
    timestamp = getCurrentTimestampMs();
}

// GatewayException implementation
GatewayException::GatewayException(const ErrorInfo& error_info)
    : std::runtime_error(error_info.message), error_info_(error_info) {
}

GatewayException::GatewayException(ErrorCategory category, ErrorLevel level,
                                  const std::string& error_code, const std::string& message,
                                  const std::string& component)
    : std::runtime_error(message), error_info_(category, level, error_code, message, component) {
}

void GatewayException::addContext(const std::string& key, const std::string& value) {
    error_info_.context[key] = value;
}

void GatewayException::setDetails(const std::string& details) {
    error_info_.details = details;
}

// Specific exception implementations
ConfigurationException::ConfigurationException(const std::string& error_code, 
                                               const std::string& message,
                                               const std::string& component)
    : GatewayException(ErrorCategory::CONFIGURATION, ErrorLevel::ERROR, error_code, message, component) {
}

NetworkException::NetworkException(const std::string& error_code, 
                                  const std::string& message,
                                  const std::string& component)
    : GatewayException(ErrorCategory::NETWORK, ErrorLevel::ERROR, error_code, message, component) {
}

DeviceCommunicationException::DeviceCommunicationException(const std::string& error_code, 
                                                          const std::string& message,
                                                          const std::string& component)
    : GatewayException(ErrorCategory::DEVICE_COMMUNICATION, ErrorLevel::ERROR, error_code, message, component) {
}

OcppProtocolException::OcppProtocolException(const std::string& error_code, 
                                            const std::string& message,
                                            const std::string& component)
    : GatewayException(ErrorCategory::OCPP_PROTOCOL, ErrorLevel::ERROR, error_code, message, component) {
}

MappingException::MappingException(const std::string& error_code, 
                                  const std::string& message,
                                  const std::string& component)
    : GatewayException(ErrorCategory::MAPPING, ErrorLevel::ERROR, error_code, message, component) {
}

AuthenticationException::AuthenticationException(const std::string& error_code, 
                                                 const std::string& message,
                                                 const std::string& component)
    : GatewayException(ErrorCategory::AUTHENTICATION, ErrorLevel::ERROR, error_code, message, component) {
}

ValidationException::ValidationException(const std::string& error_code, 
                                         const std::string& message,
                                         const std::string& component)
    : GatewayException(ErrorCategory::VALIDATION, ErrorLevel::ERROR, error_code, message, component) {
}

SystemResourceException::SystemResourceException(const std::string& error_code, 
                                                 const std::string& message,
                                                 const std::string& component)
    : GatewayException(ErrorCategory::SYSTEM_RESOURCE, ErrorLevel::ERROR, error_code, message, component) {
}

UserInputException::UserInputException(const std::string& error_code, 
                                       const std::string& message,
                                       const std::string& component)
    : GatewayException(ErrorCategory::USER_INPUT, ErrorLevel::ERROR, error_code, message, component) {
}

// ErrorHandlingManager implementation
ErrorHandlingManager& ErrorHandlingManager::getInstance() {
    static ErrorHandlingManager instance;
    return instance;
}

void ErrorHandlingManager::registerHandler(ErrorCategory category, ErrorHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    category_handlers_[category].push_back(handler);
}

void ErrorHandlingManager::registerGlobalHandler(ErrorHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    global_handlers_.push_back(handler);
}

void ErrorHandlingManager::handleError(const ErrorInfo& error_info) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // エラー統計の更新
    error_counts_[error_info.category]++;
    
    // ログ出力
    switch (error_info.level) {
        case ErrorLevel::INFO:
            LOG_INFO("[{}] {}: {} ({})", 
                    errorCategoryToString(error_info.category),
                    error_info.error_code,
                    error_info.message,
                    error_info.component);
            break;
        case ErrorLevel::WARNING:
            LOG_WARN("[{}] {}: {} ({})", 
                    errorCategoryToString(error_info.category),
                    error_info.error_code,
                    error_info.message,
                    error_info.component);
            break;
        case ErrorLevel::ERROR:
            LOG_ERROR("[{}] {}: {} ({})", 
                     errorCategoryToString(error_info.category),
                     error_info.error_code,
                     error_info.message,
                     error_info.component);
            break;
        case ErrorLevel::CRITICAL:
            LOG_CRITICAL("[{}] {}: {} ({})", 
                        errorCategoryToString(error_info.category),
                        error_info.error_code,
                        error_info.message,
                        error_info.component);
            break;
    }
    
    // 詳細情報とコンテキストの出力
    if (!error_info.details.empty()) {
        LOG_DEBUG("Details: {}", error_info.details);
    }
    
    if (!error_info.context.empty()) {
        std::ostringstream oss;
        oss << "Context: ";
        for (const auto& [key, value] : error_info.context) {
            oss << key << "=" << value << " ";
        }
        LOG_DEBUG("{}", oss.str());
    }
    
    // カテゴリ別ハンドラーの実行
    auto category_it = category_handlers_.find(error_info.category);
    if (category_it != category_handlers_.end()) {
        for (const auto& handler : category_it->second) {
            try {
                handler(error_info);
            } catch (const std::exception& e) {
                LOG_ERROR("Error in category handler: {}", e.what());
            } catch (...) {
                LOG_ERROR("Unknown error in category handler");
            }
        }
    }
    
    // グローバルハンドラーの実行
    for (const auto& handler : global_handlers_) {
        try {
            handler(error_info);
        } catch (const std::exception& e) {
            LOG_ERROR("Error in global handler: {}", e.what());
        } catch (...) {
            LOG_ERROR("Unknown error in global handler");
        }
    }
}

void ErrorHandlingManager::handleError(const GatewayException& exception) {
    handleError(exception.getErrorInfo());
}

std::map<ErrorCategory, int> ErrorHandlingManager::getErrorStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return error_counts_;
}

void ErrorHandlingManager::resetErrorStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    error_counts_.clear();
}

// Utility functions
std::string errorCategoryToString(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::UNKNOWN: return "UNKNOWN";
        case ErrorCategory::CONFIGURATION: return "CONFIGURATION";
        case ErrorCategory::NETWORK: return "NETWORK";
        case ErrorCategory::DEVICE_COMMUNICATION: return "DEVICE_COMMUNICATION";
        case ErrorCategory::OCPP_PROTOCOL: return "OCPP_PROTOCOL";
        case ErrorCategory::MAPPING: return "MAPPING";
        case ErrorCategory::AUTHENTICATION: return "AUTHENTICATION";
        case ErrorCategory::VALIDATION: return "VALIDATION";
        case ErrorCategory::SYSTEM_RESOURCE: return "SYSTEM_RESOURCE";
        case ErrorCategory::USER_INPUT: return "USER_INPUT";
        default: return "UNKNOWN";
    }
}

std::string errorLevelToString(ErrorLevel level) {
    switch (level) {
        case ErrorLevel::INFO: return "INFO";
        case ErrorLevel::WARNING: return "WARNING";
        case ErrorLevel::ERROR: return "ERROR";
        case ErrorLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

int64_t getCurrentTimestampMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return milliseconds.count();
}

std::string errorInfoToJson(const ErrorInfo& error_info) {
    Json::Value root;
    root["category"] = errorCategoryToString(error_info.category);
    root["level"] = errorLevelToString(error_info.level);
    root["error_code"] = error_info.error_code;
    root["message"] = error_info.message;
    root["details"] = error_info.details;
    root["component"] = error_info.component;
    root["timestamp"] = static_cast<Json::Int64>(error_info.timestamp);
    
    // コンテキスト情報の追加
    if (!error_info.context.empty()) {
        Json::Value context_obj;
        for (const auto& [key, value] : error_info.context) {
            context_obj[key] = value;
        }
        root["context"] = context_obj;
    }
    
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

} // namespace common
} // namespace ocpp_gateway 