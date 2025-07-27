#pragma once

#include <stdexcept>
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <mutex>

namespace ocpp_gateway {
namespace common {

/**
 * @brief エラーカテゴリ列挙型
 */
enum class ErrorCategory {
    UNKNOWN = 0,
    CONFIGURATION,
    NETWORK,
    DEVICE_COMMUNICATION,
    OCPP_PROTOCOL,
    MAPPING,
    AUTHENTICATION,
    VALIDATION,
    SYSTEM_RESOURCE,
    USER_INPUT
};

/**
 * @brief エラーレベル列挙型
 */
enum class ErrorLevel {
    INFO = 0,
    WARNING,
    ERROR,
    CRITICAL
};

/**
 * @brief 詳細エラー情報構造体
 */
struct ErrorInfo {
    ErrorCategory category;
    ErrorLevel level;
    std::string error_code;
    std::string message;
    std::string details;
    std::map<std::string, std::string> context;
    std::string component;
    int64_t timestamp;
    
    ErrorInfo() = default;
    ErrorInfo(ErrorCategory cat, ErrorLevel lvl, const std::string& code, 
              const std::string& msg, const std::string& comp = "");
};

/**
 * @brief 基底例外クラス
 */
class GatewayException : public std::runtime_error {
public:
    explicit GatewayException(const ErrorInfo& error_info);
    explicit GatewayException(ErrorCategory category, ErrorLevel level,
                             const std::string& error_code, const std::string& message,
                             const std::string& component = "");
    
    const ErrorInfo& getErrorInfo() const { return error_info_; }
    ErrorCategory getCategory() const { return error_info_.category; }
    ErrorLevel getLevel() const { return error_info_.level; }
    const std::string& getErrorCode() const { return error_info_.error_code; }
    const std::string& getComponent() const { return error_info_.component; }
    
    void addContext(const std::string& key, const std::string& value);
    void setDetails(const std::string& details);

private:
    ErrorInfo error_info_;
};

/**
 * @brief 設定関連例外
 */
class ConfigurationException : public GatewayException {
public:
    explicit ConfigurationException(const std::string& error_code, const std::string& message,
                                   const std::string& component = "ConfigManager");
};

/**
 * @brief ネットワーク関連例外
 */
class NetworkException : public GatewayException {
public:
    explicit NetworkException(const std::string& error_code, const std::string& message,
                             const std::string& component = "Network");
};

/**
 * @brief デバイス通信関連例外
 */
class DeviceCommunicationException : public GatewayException {
public:
    explicit DeviceCommunicationException(const std::string& error_code, const std::string& message,
                                         const std::string& component = "DeviceAdapter");
};

/**
 * @brief OCPPプロトコル関連例外
 */
class OcppProtocolException : public GatewayException {
public:
    explicit OcppProtocolException(const std::string& error_code, const std::string& message,
                                  const std::string& component = "OcppClient");
};

/**
 * @brief マッピング関連例外
 */
class MappingException : public GatewayException {
public:
    explicit MappingException(const std::string& error_code, const std::string& message,
                             const std::string& component = "MappingEngine");
};

/**
 * @brief 認証関連例外
 */
class AuthenticationException : public GatewayException {
public:
    explicit AuthenticationException(const std::string& error_code, const std::string& message,
                                    const std::string& component = "Authentication");
};

/**
 * @brief バリデーション関連例外
 */
class ValidationException : public GatewayException {
public:
    explicit ValidationException(const std::string& error_code, const std::string& message,
                                const std::string& component = "Validation");
};

/**
 * @brief システムリソース関連例外
 */
class SystemResourceException : public GatewayException {
public:
    explicit SystemResourceException(const std::string& error_code, const std::string& message,
                                    const std::string& component = "System");
};

/**
 * @brief ユーザー入力関連例外
 */
class UserInputException : public GatewayException {
public:
    explicit UserInputException(const std::string& error_code, const std::string& message,
                               const std::string& component = "UserInput");
};

/**
 * @brief エラーハンドラー関数型
 */
using ErrorHandler = std::function<void(const ErrorInfo& error_info)>;

/**
 * @brief エラーハンドリングマネージャー
 */
class ErrorHandlingManager {
public:
    static ErrorHandlingManager& getInstance();
    
    /**
     * @brief エラーハンドラーの登録
     * @param category エラーカテゴリ
     * @param handler ハンドラー関数
     */
    void registerHandler(ErrorCategory category, ErrorHandler handler);
    
    /**
     * @brief 全カテゴリ対応のエラーハンドラーの登録
     * @param handler ハンドラー関数
     */
    void registerGlobalHandler(ErrorHandler handler);
    
    /**
     * @brief エラーの処理
     * @param error_info エラー情報
     */
    void handleError(const ErrorInfo& error_info);
    
    /**
     * @brief エラーの処理（例外から）
     * @param exception 例外オブジェクト
     */
    void handleError(const GatewayException& exception);
    
    /**
     * @brief エラー統計の取得
     * @return カテゴリ別エラー件数
     */
    std::map<ErrorCategory, int> getErrorStatistics() const;
    
    /**
     * @brief エラー統計のリセット
     */
    void resetErrorStatistics();

private:
    ErrorHandlingManager() = default;
    
    std::map<ErrorCategory, std::vector<ErrorHandler>> category_handlers_;
    std::vector<ErrorHandler> global_handlers_;
    std::map<ErrorCategory, int> error_counts_;
    mutable std::mutex mutex_;
};

/**
 * @brief エラーカテゴリの文字列変換
 */
std::string errorCategoryToString(ErrorCategory category);

/**
 * @brief エラーレベルの文字列変換
 */
std::string errorLevelToString(ErrorLevel level);

/**
 * @brief 現在時刻の取得（ミリ秒）
 */
int64_t getCurrentTimestampMs();

/**
 * @brief エラー情報のJSON形式変換
 */
std::string errorInfoToJson(const ErrorInfo& error_info);

/**
 * @brief 安全な例外キャッチマクロ
 */
#define SAFE_CATCH_AND_LOG(action) \
    do { \
        try { \
            action; \
        } catch (const ocpp_gateway::common::GatewayException& e) { \
            LOG_ERROR("Gateway exception in {}: [{}] {}", __FUNCTION__, e.getErrorCode(), e.what()); \
            ocpp_gateway::common::ErrorHandlingManager::getInstance().handleError(e); \
        } catch (const std::exception& e) { \
            LOG_ERROR("Standard exception in {}: {}", __FUNCTION__, e.what()); \
            auto error_info = ocpp_gateway::common::ErrorInfo( \
                ocpp_gateway::common::ErrorCategory::UNKNOWN, \
                ocpp_gateway::common::ErrorLevel::ERROR, \
                "UNKNOWN_EXCEPTION", \
                e.what(), \
                __FUNCTION__ \
            ); \
            ocpp_gateway::common::ErrorHandlingManager::getInstance().handleError(error_info); \
        } catch (...) { \
            LOG_ERROR("Unknown exception in {}", __FUNCTION__); \
            auto error_info = ocpp_gateway::common::ErrorInfo( \
                ocpp_gateway::common::ErrorCategory::UNKNOWN, \
                ocpp_gateway::common::ErrorLevel::CRITICAL, \
                "UNKNOWN_EXCEPTION", \
                "Unknown exception caught", \
                __FUNCTION__ \
            ); \
            ocpp_gateway::common::ErrorHandlingManager::getInstance().handleError(error_info); \
        } \
    } while (0)

/**
 * @brief リソース安全管理マクロ（RAII補助）
 */
#define SAFE_RESOURCE_ACTION(resource_action, cleanup_action) \
    do { \
        try { \
            resource_action; \
        } catch (...) { \
            try { \
                cleanup_action; \
            } catch (...) { \
                LOG_ERROR("Exception during cleanup in {}", __FUNCTION__); \
            } \
            throw; \
        } \
    } while (0)

} // namespace common
} // namespace ocpp_gateway 