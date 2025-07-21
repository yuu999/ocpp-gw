#include "ocpp_gateway/common/language_manager.h"
#include "ocpp_gateway/common/logger.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>
#include <json/json.h>

namespace fs = boost::filesystem;

namespace ocpp_gateway {
namespace common {

LanguageManager& LanguageManager::getInstance() {
    static LanguageManager instance;
    return instance;
}

LanguageManager::LanguageManager() 
    : current_language_("en") {
}

bool LanguageManager::initialize(const std::string& language, const std::string& resource_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    resource_dir_ = resource_dir;
    
    // Create resource directory if it doesn't exist
    if (!fs::exists(resource_dir_)) {
        try {
            fs::create_directories(resource_dir_);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create language resource directory: {}", e.what());
            return false;
        }
    }
    
    // Load all available translations
    if (!loadAllTranslations()) {
        LOG_WARN("No language resources found in {}", resource_dir_);
        
        // Create default English and Japanese resources if they don't exist
        createDefaultResources();
    }
    
    // Set the requested language if available, otherwise use English
    if (!setLanguage(language)) {
        LOG_WARN("Requested language '{}' not available, using English", language);
        current_language_ = "en";
    }
    
    LOG_INFO("Language manager initialized with language: {}", current_language_);
    return true;
}

bool LanguageManager::setLanguage(const std::string& language) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the language is available
    if (translations_.find(language) == translations_.end()) {
        LOG_WARN("Language '{}' not available", language);
        return false;
    }
    
    current_language_ = language;
    LOG_INFO("Language set to: {}", current_language_);
    return true;
}

std::string LanguageManager::getCurrentLanguage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_language_;
}

std::vector<std::string> LanguageManager::getAvailableLanguages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return available_languages_;
}

std::string LanguageManager::translate(const std::string& key, const std::string& default_value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the current language has the key
    auto lang_it = translations_.find(current_language_);
    if (lang_it != translations_.end()) {
        auto key_it = lang_it->second.find(key);
        if (key_it != lang_it->second.end()) {
            return key_it->second;
        }
    }
    
    // If not found in current language, try English
    if (current_language_ != "en") {
        auto en_it = translations_.find("en");
        if (en_it != translations_.end()) {
            auto key_it = en_it->second.find(key);
            if (key_it != en_it->second.end()) {
                return key_it->second;
            }
        }
    }
    
    // Return default value if key not found
    return default_value.empty() ? key : default_value;
}

template<typename... Args>
std::string LanguageManager::translateFormat(const std::string& key, Args... args) const {
    std::string translated = translate(key);
    
    // Format the string using fmt library
    try {
        return fmt::format(translated, args...);
    } catch (const std::exception& e) {
        LOG_ERROR("Error formatting translation for key '{}': {}", key, e.what());
        return translated;
    }
}

void LanguageManager::addTranslation(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    translations_[current_language_][key] = value;
}

bool LanguageManager::loadTranslationsFromFile(const std::string& file_path, const std::string& language) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string lang = language.empty() ? current_language_ : language;
    
    try {
        // Open the file
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open translation file: {}", file_path);
            return false;
        }
        
        // Parse JSON
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(file, root)) {
            LOG_ERROR("Failed to parse translation file: {}", reader.getFormattedErrorMessages());
            return false;
        }
        
        // Check if the file contains a language field
        if (root.isMember("language")) {
            lang = root["language"].asString();
        }
        
        // Load translations
        if (root.isMember("translations") && root["translations"].isObject()) {
            const Json::Value& translations = root["translations"];
            for (const auto& key : translations.getMemberNames()) {
                translations_[lang][key] = translations[key].asString();
            }
        }
        
        // Add language to available languages if not already there
        if (std::find(available_languages_.begin(), available_languages_.end(), lang) == available_languages_.end()) {
            available_languages_.push_back(lang);
        }
        
        LOG_INFO("Loaded {} translations for language '{}'", translations_[lang].size(), lang);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading translations from file '{}': {}", file_path, e.what());
        return false;
    }
}

bool LanguageManager::loadAllTranslations() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Clear existing translations
        translations_.clear();
        available_languages_.clear();
        
        // Check if resource directory exists
        if (!fs::exists(resource_dir_) || !fs::is_directory(resource_dir_)) {
            LOG_ERROR("Language resource directory does not exist: {}", resource_dir_);
            return false;
        }
        
        // Load all JSON files in the resource directory
        bool any_loaded = false;
        for (const auto& entry : fs::directory_iterator(resource_dir_)) {
            if (fs::is_regular_file(entry) && entry.path().extension() == ".json") {
                if (loadTranslationsFromFile(entry.path().string())) {
                    any_loaded = true;
                }
            }
        }
        
        return any_loaded;
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading translations: {}", e.what());
        return false;
    }
}

void LanguageManager::createDefaultResources() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Create English resource file
        Json::Value en_root;
        en_root["language"] = "en";
        Json::Value& en_translations = en_root["translations"];
        
        // Add English translations
        en_translations["dashboard"] = "Dashboard";
        en_translations["devices"] = "Devices";
        en_translations["config"] = "Configuration";
        en_translations["logs"] = "Logs";
        en_translations["system_status"] = "System Status";
        en_translations["device_status"] = "Device Status";
        en_translations["recent_events"] = "Recent Events";
        en_translations["status"] = "Status";
        en_translations["uptime"] = "Uptime";
        en_translations["version"] = "Version";
        en_translations["total_devices"] = "Total Devices";
        en_translations["active"] = "Active";
        en_translations["error"] = "Error";
        en_translations["system_config"] = "System Configuration";
        en_translations["csms_config"] = "CSMS Configuration";
        en_translations["log_level"] = "Log Level";
        en_translations["max_charge_points"] = "Maximum Charge Points";
        en_translations["url"] = "URL";
        en_translations["reconnect_interval"] = "Reconnect Interval";
        en_translations["max_reconnect_attempts"] = "Maximum Reconnect Attempts";
        en_translations["seconds"] = "seconds";
        en_translations["config_operations"] = "Configuration Operations";
        en_translations["reload_config"] = "Reload Configuration";
        en_translations["validate_config"] = "Validate Configuration";
        en_translations["backup_config"] = "Backup Configuration";
        en_translations["device_management"] = "Device Management";
        en_translations["registered_devices"] = "Registered Devices";
        en_translations["id"] = "ID";
        en_translations["name"] = "Name";
        en_translations["protocol"] = "Protocol";
        en_translations["template"] = "Template";
        en_translations["state"] = "State";
        en_translations["actions"] = "Actions";
        en_translations["details"] = "Details";
        en_translations["edit"] = "Edit";
        en_translations["no_devices"] = "No devices registered";
        en_translations["latest_logs"] = "Latest Logs";
        en_translations["language"] = "Language";
        en_translations["english"] = "English";
        en_translations["japanese"] = "Japanese";
        en_translations["page_not_found"] = "Page not found";
        en_translations["authentication_required"] = "Authentication required";
        en_translations["internal_server_error"] = "Internal server error";
        en_translations["file_read_error"] = "File read error";
        en_translations["back_to_dashboard"] = "Back to Dashboard";
        en_translations["system_started"] = "System started";
        en_translations["config_loaded"] = "Configuration loaded";
        en_translations["admin_api_started"] = "Admin API server started";
        en_translations["metrics_initialized"] = "Metrics collection initialized";
        en_translations["enabled"] = "Enabled";
        en_translations["disabled"] = "Disabled";
        
        // Write English resource file
        std::string en_file_path = (fs::path(resource_dir_) / "en.json").string();
        std::ofstream en_file(en_file_path);
        if (en_file.is_open()) {
            Json::StyledWriter writer;
            en_file << writer.write(en_root);
            en_file.close();
            
            // Add English to available languages
            if (std::find(available_languages_.begin(), available_languages_.end(), "en") == available_languages_.end()) {
                available_languages_.push_back("en");
            }
            
            // Load English translations into memory
            for (const auto& key : en_translations.getMemberNames()) {
                translations_["en"][key] = en_translations[key].asString();
            }
            
            LOG_INFO("Created default English language resource file: {}", en_file_path);
        } else {
            LOG_ERROR("Failed to create English language resource file: {}", en_file_path);
        }
        
        // Create Japanese resource file
        Json::Value ja_root;
        ja_root["language"] = "ja";
        Json::Value& ja_translations = ja_root["translations"];
        
        // Add Japanese translations
        ja_translations["dashboard"] = "ダッシュボード";
        ja_translations["devices"] = "デバイス";
        ja_translations["config"] = "設定";
        ja_translations["logs"] = "ログ";
        ja_translations["system_status"] = "システム状態";
        ja_translations["device_status"] = "デバイス状態";
        ja_translations["recent_events"] = "最近のイベント";
        ja_translations["status"] = "状態";
        ja_translations["uptime"] = "稼働時間";
        ja_translations["version"] = "バージョン";
        ja_translations["total_devices"] = "総デバイス数";
        ja_translations["active"] = "アクティブ";
        ja_translations["error"] = "エラー";
        ja_translations["system_config"] = "システム設定";
        ja_translations["csms_config"] = "CSMS設定";
        ja_translations["log_level"] = "ログレベル";
        ja_translations["max_charge_points"] = "最大充電ポイント数";
        ja_translations["url"] = "URL";
        ja_translations["reconnect_interval"] = "再接続間隔";
        ja_translations["max_reconnect_attempts"] = "最大再接続回数";
        ja_translations["seconds"] = "秒";
        ja_translations["config_operations"] = "設定操作";
        ja_translations["reload_config"] = "設定再読み込み";
        ja_translations["validate_config"] = "設定検証";
        ja_translations["backup_config"] = "設定バックアップ";
        ja_translations["device_management"] = "デバイス管理";
        ja_translations["registered_devices"] = "登録デバイス一覧";
        ja_translations["id"] = "ID";
        ja_translations["name"] = "名前";
        ja_translations["protocol"] = "プロトコル";
        ja_translations["template"] = "テンプレート";
        ja_translations["state"] = "状態";
        ja_translations["actions"] = "操作";
        ja_translations["details"] = "詳細";
        ja_translations["edit"] = "編集";
        ja_translations["no_devices"] = "登録されているデバイスはありません";
        ja_translations["latest_logs"] = "最新ログ";
        ja_translations["language"] = "言語";
        ja_translations["english"] = "英語";
        ja_translations["japanese"] = "日本語";
        ja_translations["page_not_found"] = "ページが見つかりません";
        ja_translations["authentication_required"] = "認証が必要です";
        ja_translations["internal_server_error"] = "内部サーバーエラー";
        ja_translations["file_read_error"] = "ファイル読み取りエラー";
        ja_translations["back_to_dashboard"] = "ダッシュボードに戻る";
        ja_translations["system_started"] = "システム開始";
        ja_translations["config_loaded"] = "設定読み込み完了";
        ja_translations["admin_api_started"] = "管理APIサーバー開始";
        ja_translations["metrics_initialized"] = "メトリクス収集を初期化しました";
        ja_translations["enabled"] = "有効";
        ja_translations["disabled"] = "無効";
        
        // Write Japanese resource file
        std::string ja_file_path = (fs::path(resource_dir_) / "ja.json").string();
        std::ofstream ja_file(ja_file_path);
        if (ja_file.is_open()) {
            Json::StyledWriter writer;
            ja_file << writer.write(ja_root);
            ja_file.close();
            
            // Add Japanese to available languages
            if (std::find(available_languages_.begin(), available_languages_.end(), "ja") == available_languages_.end()) {
                available_languages_.push_back("ja");
            }
            
            // Load Japanese translations into memory
            for (const auto& key : ja_translations.getMemberNames()) {
                translations_["ja"][key] = ja_translations[key].asString();
            }
            
            LOG_INFO("Created default Japanese language resource file: {}", ja_file_path);
        } else {
            LOG_ERROR("Failed to create Japanese language resource file: {}", ja_file_path);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error creating default language resources: {}", e.what());
    }
}

} // namespace common
} // namespace ocpp_gateway