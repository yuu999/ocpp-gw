#include "ocpp_gateway/common/i18n_manager.h"
#include "ocpp_gateway/common/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace ocpp_gateway {
namespace common {

I18nManager::I18nManager()
    : current_language_(Language::ENGLISH)
{
}

I18nManager::I18nManager(Language default_language)
    : current_language_(default_language)
{
}

I18nManager::~I18nManager() = default;

bool I18nManager::initialize() {
    // Initialize default translations
    initializeDefaultTranslations();
    
    // Set available languages
    available_languages_ = {Language::ENGLISH, Language::JAPANESE};
    
    LOG_INFO("I18n system initialized with language: {}", languageToString(current_language_));
    return true;
}

bool I18nManager::loadResources(const std::string& resource_dir) {
    if (!std::filesystem::exists(resource_dir)) {
        LOG_ERROR("Resource directory does not exist: {}", resource_dir);
        return false;
    }
    
    // Load English translations
    std::string en_file = resource_dir + "/en.json";
    if (std::filesystem::exists(en_file)) {
        if (!loadLanguageFile(Language::ENGLISH, en_file)) {
            LOG_WARNING("Failed to load English translations from: {}", en_file);
        }
    }
    
    // Load Japanese translations
    std::string ja_file = resource_dir + "/ja.json";
    if (std::filesystem::exists(ja_file)) {
        if (!loadLanguageFile(Language::JAPANESE, ja_file)) {
            LOG_WARNING("Failed to load Japanese translations from: {}", ja_file);
        }
    }
    
    LOG_INFO("Loaded language resources from: {}", resource_dir);
    return true;
}

bool I18nManager::setLanguage(Language language) {
    if (!isLanguageAvailable(language)) {
        LOG_ERROR("Language not available: {}", languageToString(language));
        return false;
    }
    
    current_language_ = language;
    LOG_INFO("Language set to: {}", languageToString(language));
    return true;
}

Language I18nManager::getCurrentLanguage() const {
    return current_language_;
}

std::string I18nManager::getText(const std::string& key) const {
    auto lang_it = translations_.find(current_language_);
    if (lang_it == translations_.end()) {
        // Fallback to English
        lang_it = translations_.find(Language::ENGLISH);
        if (lang_it == translations_.end()) {
            return key; // Return key if no translation found
        }
    }
    
    auto text_it = lang_it->second.find(key);
    if (text_it == lang_it->second.end()) {
        return key; // Return key if translation not found
    }
    
    return text_it->second;
}

std::string I18nManager::getText(const std::string& key, const std::map<std::string, std::string>& params) const {
    std::string text = getText(key);
    return replaceParameters(text, params);
}

std::vector<Language> I18nManager::getAvailableLanguages() const {
    return available_languages_;
}

bool I18nManager::isLanguageAvailable(Language language) const {
    return std::find(available_languages_.begin(), available_languages_.end(), language) != available_languages_.end();
}

std::string I18nManager::getLanguageName(Language language) const {
    switch (language) {
        case Language::ENGLISH:
            return "English";
        case Language::JAPANESE:
            return "日本語";
        default:
            return "Unknown";
    }
}

std::string I18nManager::getLanguageCode(Language language) const {
    switch (language) {
        case Language::ENGLISH:
            return "en";
        case Language::JAPANESE:
            return "ja";
        default:
            return "en";
    }
}

std::string I18nManager::languageToString(Language language) {
    switch (language) {
        case Language::ENGLISH:
            return "ENGLISH";
        case Language::JAPANESE:
            return "JAPANESE";
        default:
            return "ENGLISH";
    }
}

Language I18nManager::languageFromString(const std::string& language_string) {
    if (language_string == "ENGLISH" || language_string == "en") {
        return Language::ENGLISH;
    } else if (language_string == "JAPANESE" || language_string == "ja") {
        return Language::JAPANESE;
    }
    return Language::ENGLISH; // Default
}

std::string I18nManager::formatString(const std::string& format, const std::map<std::string, std::string>& params) {
    std::string result = format;
    
    for (const auto& param : params) {
        std::string placeholder = "{" + param.first + "}";
        size_t pos = result.find(placeholder);
        while (pos != std::string::npos) {
            result.replace(pos, placeholder.length(), param.second);
            pos = result.find(placeholder);
        }
    }
    
    return result;
}

Language I18nManager::getSystemDefaultLanguage() {
    // This is a simplified implementation
    // In a real implementation, you would check system locale
    
    // For now, return English as default
    return Language::ENGLISH;
}

bool I18nManager::loadLanguageFile(Language language, const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open language file: {}", file_path);
        return false;
    }
    
    // This is a simplified implementation
    // In a real implementation, you would parse JSON/YAML
    
    std::string line;
    while (std::getline(file, line)) {
        // Simple key=value format for now
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (!key.empty() && !value.empty()) {
                translations_[language][key] = value;
            }
        }
    }
    
    LOG_INFO("Loaded {} translations from: {}", languageToString(language), file_path);
    return true;
}

void I18nManager::initializeDefaultTranslations() {
    // English translations
    translations_[Language::ENGLISH] = {
        // System messages
        {"system.startup", "OCPP Gateway starting up..."},
        {"system.shutdown", "OCPP Gateway shutting down..."},
        {"system.error", "System error occurred"},
        {"system.warning", "System warning"},
        {"system.info", "System information"},
        
        // Device messages
        {"device.connected", "Device connected"},
        {"device.disconnected", "Device disconnected"},
        {"device.error", "Device error"},
        {"device.timeout", "Device timeout"},
        {"device.not_found", "Device not found"},
        
        // OCPP messages
        {"ocpp.connected", "OCPP connection established"},
        {"ocpp.disconnected", "OCPP connection lost"},
        {"ocpp.message_sent", "OCPP message sent"},
        {"ocpp.message_received", "OCPP message received"},
        {"ocpp.error", "OCPP error"},
        
        // Configuration messages
        {"config.loaded", "Configuration loaded"},
        {"config.error", "Configuration error"},
        {"config.reloaded", "Configuration reloaded"},
        {"config.invalid", "Invalid configuration"},
        
        // Security messages
        {"security.auth_success", "Authentication successful"},
        {"security.auth_failed", "Authentication failed"},
        {"security.unauthorized", "Unauthorized access"},
        {"security.permission_denied", "Permission denied"},
        
        // UI messages
        {"ui.login", "Login"},
        {"ui.logout", "Logout"},
        {"ui.username", "Username"},
        {"ui.password", "Password"},
        {"ui.submit", "Submit"},
        {"ui.cancel", "Cancel"},
        {"ui.save", "Save"},
        {"ui.delete", "Delete"},
        {"ui.edit", "Edit"},
        {"ui.add", "Add"},
        {"ui.search", "Search"},
        {"ui.refresh", "Refresh"},
        {"ui.back", "Back"},
        {"ui.next", "Next"},
        {"ui.previous", "Previous"},
        {"ui.home", "Home"},
        {"ui.dashboard", "Dashboard"},
        {"ui.devices", "Devices"},
        {"ui.configuration", "Configuration"},
        {"ui.logs", "Logs"},
        {"ui.metrics", "Metrics"},
        {"ui.users", "Users"},
        {"ui.security", "Security"},
        
        // Status messages
        {"status.online", "Online"},
        {"status.offline", "Offline"},
        {"status.available", "Available"},
        {"status.occupied", "Occupied"},
        {"status.faulted", "Faulted"},
        {"status.reserved", "Reserved"},
        {"status.unavailable", "Unavailable"},
        
        // Error messages
        {"error.network", "Network error"},
        {"error.timeout", "Timeout error"},
        {"error.invalid_input", "Invalid input"},
        {"error.file_not_found", "File not found"},
        {"error.permission_denied", "Permission denied"},
        {"error.internal", "Internal error"},
        {"error.unknown", "Unknown error"}
    };
    
    // Japanese translations
    translations_[Language::JAPANESE] = {
        // System messages
        {"system.startup", "OCPPゲートウェイを起動中..."},
        {"system.shutdown", "OCPPゲートウェイを終了中..."},
        {"system.error", "システムエラーが発生しました"},
        {"system.warning", "システム警告"},
        {"system.info", "システム情報"},
        
        // Device messages
        {"device.connected", "デバイスが接続されました"},
        {"device.disconnected", "デバイスが切断されました"},
        {"device.error", "デバイスエラー"},
        {"device.timeout", "デバイスタイムアウト"},
        {"device.not_found", "デバイスが見つかりません"},
        
        // OCPP messages
        {"ocpp.connected", "OCPP接続が確立されました"},
        {"ocpp.disconnected", "OCPP接続が切断されました"},
        {"ocpp.message_sent", "OCPPメッセージを送信しました"},
        {"ocpp.message_received", "OCPPメッセージを受信しました"},
        {"ocpp.error", "OCPPエラー"},
        
        // Configuration messages
        {"config.loaded", "設定を読み込みました"},
        {"config.error", "設定エラー"},
        {"config.reloaded", "設定を再読み込みしました"},
        {"config.invalid", "無効な設定"},
        
        // Security messages
        {"security.auth_success", "認証に成功しました"},
        {"security.auth_failed", "認証に失敗しました"},
        {"security.unauthorized", "認証されていません"},
        {"security.permission_denied", "権限がありません"},
        
        // UI messages
        {"ui.login", "ログイン"},
        {"ui.logout", "ログアウト"},
        {"ui.username", "ユーザー名"},
        {"ui.password", "パスワード"},
        {"ui.submit", "送信"},
        {"ui.cancel", "キャンセル"},
        {"ui.save", "保存"},
        {"ui.delete", "削除"},
        {"ui.edit", "編集"},
        {"ui.add", "追加"},
        {"ui.search", "検索"},
        {"ui.refresh", "更新"},
        {"ui.back", "戻る"},
        {"ui.next", "次へ"},
        {"ui.previous", "前へ"},
        {"ui.home", "ホーム"},
        {"ui.dashboard", "ダッシュボード"},
        {"ui.devices", "デバイス"},
        {"ui.configuration", "設定"},
        {"ui.logs", "ログ"},
        {"ui.metrics", "メトリクス"},
        {"ui.users", "ユーザー"},
        {"ui.security", "セキュリティ"},
        
        // Status messages
        {"status.online", "オンライン"},
        {"status.offline", "オフライン"},
        {"status.available", "利用可能"},
        {"status.occupied", "使用中"},
        {"status.faulted", "故障"},
        {"status.reserved", "予約済み"},
        {"status.unavailable", "利用不可"},
        
        // Error messages
        {"error.network", "ネットワークエラー"},
        {"error.timeout", "タイムアウトエラー"},
        {"error.invalid_input", "無効な入力"},
        {"error.file_not_found", "ファイルが見つかりません"},
        {"error.permission_denied", "権限がありません"},
        {"error.internal", "内部エラー"},
        {"error.unknown", "不明なエラー"}
    };
}

std::string I18nManager::replaceParameters(const std::string& text, const std::map<std::string, std::string>& params) const {
    std::string result = text;
    
    for (const auto& param : params) {
        std::string placeholder = "{" + param.first + "}";
        size_t pos = result.find(placeholder);
        while (pos != std::string::npos) {
            result.replace(pos, placeholder.length(), param.second);
            pos = result.find(placeholder);
        }
    }
    
    return result;
}

} // namespace common
} // namespace ocpp_gateway 