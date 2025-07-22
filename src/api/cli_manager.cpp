#include "ocpp_gateway/api/cli_manager.h"
#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/metrics_collector.h"
#include "ocpp_gateway/common/language_manager.h"
#include <json/json.h>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace ocpp_gateway {
namespace api {

CliManager::CliManager() : config_manager_(config::ConfigManager::getInstance()) {
    // Initialize language manager if not already initialized
    if (common::LanguageManager::getInstance().getAvailableLanguages().empty()) {
        common::LanguageManager::getInstance().initialize("en", "resources/lang");
    }
    
    // 基本コマンドの登録
    registerCommand("help", translate("cmd_help_desc", "ヘルプメッセージを表示"), 
                   [this](const std::vector<std::string>& args) { return handleHelp(args); });
    
    registerCommand("version", translate("cmd_version_desc", "バージョン情報を表示"),
                   [this](const std::vector<std::string>& args) { return handleVersion(args); });
    
    registerCommand("status", translate("cmd_status_desc", "システム状態を表示"),
                   [this](const std::vector<std::string>& args) { return handleStatus(args); });
    
    registerCommand("config", translate("cmd_config_desc", "設定管理コマンド"),
                   [this](const std::vector<std::string>& args) { return handleConfig(args); });
    
    registerCommand("device", translate("cmd_device_desc", "デバイス管理コマンド"),
                   [this](const std::vector<std::string>& args) { return handleDevice(args); });
    
    registerCommand("mapping", translate("cmd_mapping_desc", "マッピング管理コマンド"),
                   [this](const std::vector<std::string>& args) { return handleMapping(args); });
    
    registerCommand("metrics", translate("cmd_metrics_desc", "メトリクス管理コマンド"),
                   [this](const std::vector<std::string>& args) { return handleMetrics(args); });
    
    registerCommand("health", translate("cmd_health_desc", "ヘルスチェック"),
                   [this](const std::vector<std::string>& args) { return handleHealth(args); });
    
    registerCommand("log", translate("cmd_log_desc", "ログ管理コマンド"),
                   [this](const std::vector<std::string>& args) { return handleLog(args); });
                   
    // Add language command for internationalization
    registerCommand("language", translate("cmd_language_desc", "言語設定コマンド"),
                   [this](const std::vector<std::string>& args) { return handleLanguage(args); });
}

CliManager::~CliManager() {
}

std::string CliManager::translate(const std::string& key, const std::string& default_value) const {
    return common::LanguageManager::getInstance().translate(key, default_value);
}

bool CliManager::setLanguage(const std::string& language) {
    return common::LanguageManager::getInstance().setLanguage(language);
}

std::string CliManager::getCurrentLanguage() const {
    return common::LanguageManager::getInstance().getCurrentLanguage();
}

CliResult CliManager::handleLanguage(const std::vector<std::string>& args) {
    // If no arguments, show current language and available languages
    if (args.size() == 1) {
        std::ostringstream oss;
        std::string current_lang = getCurrentLanguage();
        std::vector<std::string> available_langs = common::LanguageManager::getInstance().getAvailableLanguages();
        
        oss << translate("current_language", "現在の言語") << ": ";
        if (current_lang == "en") {
            oss << translate("english", "英語") << " (English)\n";
        } else if (current_lang == "ja") {
            oss << translate("japanese", "日本語") << " (Japanese)\n";
        } else {
            oss << current_lang << "\n";
        }
        
        oss << "\n" << translate("available_languages", "利用可能な言語") << ":\n";
        for (const auto& lang : available_langs) {
            oss << "- ";
            if (lang == "en") {
                oss << "English (" << translate("english", "英語") << ")";
            } else if (lang == "ja") {
                oss << "Japanese (" << translate("japanese", "日本語") << ")";
            } else {
                oss << lang;
            }
            
            if (lang == current_lang) {
                oss << " [" << translate("current", "現在") << "]";
            }
            oss << "\n";
        }
        
        oss << "\n" << translate("language_usage", "使用法: language <set|list>") << "\n";
        oss << "  set <lang>  - " << translate("language_set_desc", "言語を設定 (例: en, ja)") << "\n";
        oss << "  list        - " << translate("language_list_desc", "利用可能な言語を一覧表示") << "\n";
        
        return CliResult(true, "", oss.str());
    }
    
    // Handle subcommands
    std::string subcommand = args[1];
    
    if (subcommand == "set") {
        if (args.size() < 3) {
            return CliResult(false, translate("language_set_usage", "使用法: language set <lang>"));
        }
        
        std::string lang = args[2];
        if (setLanguage(lang)) {
            return CliResult(true, translate("language_set_success", "言語を設定しました: ") + lang);
        } else {
            return CliResult(false, translate("language_set_error", "言語の設定に失敗しました: ") + lang);
        }
    } else if (subcommand == "list") {
        std::ostringstream oss;
        std::vector<std::string> available_langs = common::LanguageManager::getInstance().getAvailableLanguages();
        
        oss << translate("available_languages", "利用可能な言語") << ":\n";
        for (const auto& lang : available_langs) {
            oss << "- ";
            if (lang == "en") {
                oss << "English (" << translate("english", "英語") << ")";
            } else if (lang == "ja") {
                oss << "Japanese (" << translate("japanese", "日本語") << ")";
            } else {
                oss << lang;
            }
            
            if (lang == getCurrentLanguage()) {
                oss << " [" << translate("current", "現在") << "]";
            }
            oss << "\n";
        }
        
        return CliResult(true, "", oss.str());
    } else {
        return CliResult(false, translate("unknown_subcommand", "不明なサブコマンドです: ") + subcommand);
    }
}

CliResult CliManager::executeCommand(const std::vector<std::string>& args) {
    if (args.empty()) {
        return CliResult(false, translate("no_command", "コマンドが指定されていません。'help'でヘルプを確認してください。"));
    }

    std::string command = args[0];
    auto it = commands_.find(command);
    
    if (it == commands_.end()) {
        return CliResult(false, translate("unknown_command", "不明なコマンドです: ") + command + translate("check_help", "。'help'でヘルプを確認してください。"));
    }

    try {
        return it->second.handler(args);
    } catch (const std::exception& e) {
        LOG_ERROR(translate("cli_command_error", "CLIコマンド実行エラー [{}]: {}"), command, e.what());
        return CliResult(false, translate("command_execution_error", "コマンド実行中にエラーが発生しました: ") + std::string(e.what()));
    }
}

void CliManager::registerCommand(const std::string& command, 
                                const std::string& description,
                                CliCommandHandler handler) {
    commands_[command] = {description, handler};
}

std::string CliManager::getHelpMessage() const {
    std::ostringstream oss;
    oss << "OCPP 2.0.1 " << translate("gateway_middleware", "ゲートウェイ・ミドルウェア") << " CLI\n\n";
    oss << translate("available_commands", "利用可能なコマンド") << ":\n";
    
    // 最大コマンド長を計算
    size_t max_cmd_length = 0;
    for (const auto& cmd : commands_) {
        max_cmd_length = std::max(max_cmd_length, cmd.first.length());
    }
    
    for (const auto& cmd : commands_) {
        oss << "  " << std::setw(static_cast<int>(max_cmd_length)) << std::left 
            << cmd.first << " - " << cmd.second.description << "\n";
    }
    
    oss << "\n" << translate("detailed_help", "詳細なヘルプは 'help <コマンド>' で確認できます。") << "\n";
    return oss.str();
}

CliResult CliManager::handleHelp(const std::vector<std::string>& args) {
    if (args.size() == 1) {
        return CliResult(true, "", getHelpMessage());
    } else if (args.size() == 2) {
        std::string command = args[1];
        auto it = commands_.find(command);
        
        if (it == commands_.end()) {
            return CliResult(false, translate("unknown_command", "不明なコマンドです: ") + command);
        }
        
        std::ostringstream oss;
        oss << translate("command", "コマンド") << ": " << command << "\n";
        oss << translate("description", "説明") << ": " << it->second.description << "\n\n";
        
        // コマンド固有のヘルプ
        if (command == "config") {
            oss << translate("subcommands", "サブコマンド") << ":\n";
            oss << "  show     - " << translate("config_show_desc", "現在の設定を表示") << "\n";
            oss << "  reload   - " << translate("config_reload_desc", "設定を再読み込み") << "\n";
            oss << "  validate - " << translate("config_validate_desc", "設定を検証") << "\n";
            oss << "  backup   - " << translate("config_backup_desc", "設定をバックアップ") << "\n";
            oss << "  restore  - " << translate("config_restore_desc", "設定を復元") << "\n";
        } else if (command == "device") {
            oss << translate("subcommands", "サブコマンド") << ":\n";
            oss << "  list              - " << translate("device_list_desc", "デバイス一覧を表示") << "\n";
            oss << "  show <id>         - " << translate("device_show_desc", "デバイス詳細を表示") << "\n";
            oss << "  add <file>        - " << translate("device_add_desc", "デバイスを追加") << "\n";
            oss << "  update <id> <file> - " << translate("device_update_desc", "デバイス設定を更新") << "\n";
            oss << "  delete <id>       - " << translate("device_delete_desc", "デバイスを削除") << "\n";
            oss << "  test <id>         - " << translate("device_test_desc", "デバイス通信をテスト") << "\n";
        } else if (command == "mapping") {
            oss << translate("subcommands", "サブコマンド") << ":\n";
            oss << "  list               - " << translate("mapping_list_desc", "マッピング一覧を表示") << "\n";
            oss << "  show <template>    - " << translate("mapping_show_desc", "マッピング詳細を表示") << "\n";
            oss << "  test <device_id>   - " << translate("mapping_test_desc", "マッピングをテスト") << "\n";
            oss << "  validate <file>    - " << translate("mapping_validate_desc", "マッピングファイルを検証") << "\n";
        } else if (command == "metrics") {
            oss << translate("subcommands", "サブコマンド") << ":\n";
            oss << "  show       - " << translate("metrics_show_desc", "メトリクスを表示") << "\n";
            oss << "  reset      - " << translate("metrics_reset_desc", "メトリクスをリセット") << "\n";
            oss << "  export     - " << translate("metrics_export_desc", "メトリクスをエクスポート") << "\n";
        } else if (command == "log") {
            oss << translate("subcommands", "サブコマンド") << ":\n";
            oss << "  show       - " << translate("log_show_desc", "ログを表示") << "\n";
            oss << "  level <lvl> - " << translate("log_level_desc", "ログレベルを設定") << "\n";
            oss << "  rotate     - " << translate("log_rotate_desc", "ログローテーションを実行") << "\n";
        } else if (command == "language") {
            oss << translate("subcommands", "サブコマンド") << ":\n";
            oss << "  set <lang> - " << translate("language_set_desc", "言語を設定 (例: en, ja)") << "\n";
            oss << "  list       - " << translate("language_list_desc", "利用可能な言語を一覧表示") << "\n";
        }
        
        return CliResult(true, "", oss.str());
    } else {
        return CliResult(false, translate("help_usage", "使用法: help [コマンド]"));
    }
}

CliResult CliManager::handleVersion(const std::vector<std::string>& args) {
    std::ostringstream oss;
    oss << "OCPP 2.0.1 " << translate("gateway_middleware", "ゲートウェイ・ミドルウェア") << "\n";
    oss << translate("version", "バージョン") << ": 1.0.0\n";
    oss << translate("build_date_time", "ビルド日時") << ": " << __DATE__ << " " << __TIME__ << "\n";
    oss << translate("cpp_standard", "C++標準") << ": C++17\n";
    
    return CliResult(true, "", oss.str());
}

CliResult CliManager::handleStatus(const std::vector<std::string>& args) {
    try {
        std::ostringstream oss;
        oss << translate("system_status", "システム状態") << ":\n";
        oss << "================\n";
        
        // システム設定情報
        const auto& system_config = config_manager_.getSystemConfig();
        oss << translate("log_level", "ログレベル") << ": " << system_config.getLogLevel() << "\n";
        oss << translate("max_charge_points", "最大充電ポイント数") << ": " << "N/A" << "\n"; // No getMaxChargePoints() method
        
        // デバイス情報
        const auto& device_configs = config_manager_.getDeviceConfigs();
        oss << translate("registered_devices_count", "登録デバイス数") << ": " << device_configs.getDevices().size() << "\n";
        
        // メトリクス情報（簡易版）
        oss << "\n" << translate("metrics", "メトリクス") << ":\n";
        oss << "----------\n";
        oss << translate("uptime", "稼働時間") << ": " << std::time(nullptr) << " " << translate("seconds", "秒") << "\n";
        oss << translate("config_last_update", "設定最終更新") << ": " << translate("unknown", "不明") << "\n";
        
        // 言語情報
        oss << "\n" << translate("language_info", "言語情報") << ":\n";
        oss << "----------\n";
        oss << translate("current_language", "現在の言語") << ": ";
        std::string current_lang = getCurrentLanguage();
        if (current_lang == "en") {
            oss << translate("english", "英語") << " (English)\n";
        } else if (current_lang == "ja") {
            oss << translate("japanese", "日本語") << " (Japanese)\n";
        } else {
            oss << current_lang << "\n";
        }
        
        return CliResult(true, "", oss.str());
    } catch (const std::exception& e) {
        return CliResult(false, translate("system_status_error", "システム状態の取得に失敗しました: ") + std::string(e.what()));
    }
}

CliResult CliManager::handleConfig(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return CliResult(false, translate("config_usage", "使用法: config <show|reload|validate|backup|restore> [オプション]"));
    }
    
    std::string subcommand = args[1];
    
    if (subcommand == "show") {
        return handleConfigShow(args);
    } else if (subcommand == "reload") {
        return handleConfigReload(args);
    } else if (subcommand == "validate") {
        return handleConfigValidate(args);
    } else if (subcommand == "backup") {
        return handleConfigBackup(args);
    } else if (subcommand == "restore") {
        return handleConfigRestore(args);
    } else {
        return CliResult(false, translate("unknown_subcommand", "不明なサブコマンドです: ") + subcommand);
    }
}

CliResult CliManager::handleConfigShow(const std::vector<std::string>& args) {
    try {
        std::ostringstream oss;
        oss << translate("current_config", "現在の設定") << ":\n";
        oss << "============\n\n";
        
        // システム設定
        const auto& system_config = config_manager_.getSystemConfig();
        oss << "[" << translate("system_config", "システム設定") << "]\n";
        oss << translate("log_level", "ログレベル") << ": " << system_config.getLogLevel() << "\n";
        oss << translate("max_charge_points", "最大充電ポイント数") << ": " << "N/A" << "\n"; // No getMaxChargePoints() method
        
        // CSMS設定
        const auto& csms_config = config_manager_.getCsmsConfig();
        oss << "\n[" << translate("csms_config", "CSMS設定") << "]\n";
        oss << translate("url", "URL") << ": " << csms_config.getUrl() << "\n";
        oss << translate("reconnect_interval", "再接続間隔") << ": " << csms_config.getReconnectInterval() << " " << translate("seconds", "秒") << "\n";
        oss << translate("max_reconnect_attempts", "最大再接続回数") << ": " << csms_config.getMaxReconnectAttempts() << "\n";
        
        // デバイス設定
        const auto& device_configs = config_manager_.getDeviceConfigs();
        oss << "\n[" << translate("device_config", "デバイス設定") << "]\n";
        oss << translate("device_count", "デバイス数") << ": " << device_configs.getDevices().size() << "\n";
        
        for (const auto& device : device_configs.getDevices()) {
            oss << "  - " << device.getId() << " (" << device.getId() << ")\n";
            oss << "    " << translate("protocol", "プロトコル") << ": " << config::protocolToString(device.getProtocol()) << "\n";
            oss << "    " << translate("template", "テンプレート") << ": " << device.getTemplateId() << "\n";
            oss << "    " << translate("enabled", "有効") << ": " << translate("yes", "はい") << "\n";
        }
        
        return CliResult(true, "", oss.str());
    } catch (const std::exception& e) {
        return CliResult(false, translate("config_get_error", "設定の取得に失敗しました: ") + std::string(e.what()));
    }
}

CliResult CliManager::handleConfigReload(const std::vector<std::string>& args) {
    try {
        if (config_manager_.reloadAllConfigs()) {
            return CliResult(true, translate("config_reload_success", "設定を再読み込みしました"));
        } else {
            return CliResult(false, translate("config_reload_error", "設定の再読み込みに失敗しました"));
        }
    } catch (const std::exception& e) {
        return CliResult(false, translate("config_reload_exception", "設定の再読み込み中にエラーが発生しました: ") + std::string(e.what()));
    }
}

CliResult CliManager::handleConfigValidate(const std::vector<std::string>& args) {
    try {
        config_manager_.validateAllConfigs();
        return CliResult(true, translate("config_validate_success", "設定の検証が成功しました"));
    } catch (const config::ConfigValidationError& e) {
        return CliResult(false, translate("config_validate_error", "設定の検証に失敗しました: ") + std::string(e.what()));
    } catch (const std::exception& e) {
        return CliResult(false, translate("config_validate_exception", "設定の検証中にエラーが発生しました: ") + std::string(e.what()));
    }
}

CliResult CliManager::handleConfigBackup(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "設定バックアップ機能は未実装です"));
}

CliResult CliManager::handleConfigRestore(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "設定復元機能は未実装です"));
}

CliResult CliManager::handleDevice(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return CliResult(false, translate("device_usage", "使用法: device <list|show|add|update|delete|test> [オプション]"));
    }
    
    std::string subcommand = args[1];
    
    if (subcommand == "list") {
        return handleDeviceList(args);
    } else if (subcommand == "show") {
        return handleDeviceShow(args);
    } else if (subcommand == "add") {
        return handleDeviceAdd(args);
    } else if (subcommand == "update") {
        return handleDeviceUpdate(args);
    } else if (subcommand == "delete") {
        return handleDeviceDelete(args);
    } else if (subcommand == "test") {
        return handleDeviceTest(args);
    } else {
        return CliResult(false, translate("unknown_subcommand", "不明なサブコマンドです: ") + subcommand);
    }
}

CliResult CliManager::handleDeviceList(const std::vector<std::string>& args) {
    try {
        const auto& device_configs = config_manager_.getDeviceConfigs();
        const auto& devices = device_configs.getDevices();
        
        if (devices.empty()) {
            return CliResult(true, "", translate("no_devices", "登録されているデバイスはありません。") + "\n");
        }
        
        std::vector<std::vector<std::string>> table_data;
        std::vector<std::string> headers = {
            translate("id", "ID"), 
            translate("name", "名前"), 
            translate("protocol", "プロトコル"), 
            translate("template", "テンプレート"), 
            translate("enabled", "有効")
        };
        
        for (const auto& device : devices) {
            std::vector<std::string> row;
            row.push_back(device.getId());
            row.push_back(device.getId()); // No getName() method
            row.push_back(config::protocolToString(device.getProtocol()));
            row.push_back(device.getTemplateId()); // No getTemplateName() method
            row.push_back(translate("yes", "はい")); // No isEnabled() method, assuming true
            table_data.push_back(row);
        }
        
        std::string output = translate("device_list", "デバイス一覧") + ":\n";
        output += formatTable(table_data, headers);
        output += "\n" + translate("total", "総数") + ": " + std::to_string(devices.size()) + " " + translate("devices", "デバイス") + "\n";
        
        return CliResult(true, "", output);
    } catch (const std::exception& e) {
        return CliResult(false, translate("device_list_error", "デバイス一覧の取得に失敗しました: ") + std::string(e.what()));
    }
}

CliResult CliManager::handleDeviceShow(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return CliResult(false, translate("device_show_usage", "使用法: device show <device_id>"));
    }
    
    std::string device_id = args[2];
    
    try {
        auto device_opt = config_manager_.getDeviceConfig(device_id);
        if (!device_opt) {
            return CliResult(false, translate("device_not_found", "デバイスが見つかりません: ") + device_id);
        }
        
        const auto& device = device_opt.value();
        std::string output = formatDeviceInfo(device);
        
        return CliResult(true, "", output);
    } catch (const std::exception& e) {
        return CliResult(false, translate("device_info_error", "デバイス情報の取得に失敗しました: ") + std::string(e.what()));
    }
}

// 残りのメソッドは簡易実装
CliResult CliManager::handleDeviceAdd(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "デバイス追加機能は未実装です"));
}

CliResult CliManager::handleDeviceUpdate(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "デバイス更新機能は未実装です"));
}

CliResult CliManager::handleDeviceDelete(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "デバイス削除機能は未実装です"));
}

CliResult CliManager::handleDeviceTest(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "デバイステスト機能は未実装です"));
}

CliResult CliManager::handleMapping(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "マッピング管理機能は未実装です"));
}

CliResult CliManager::handleMappingList(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "マッピング一覧機能は未実装です"));
}

CliResult CliManager::handleMappingShow(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "マッピング表示機能は未実装です"));
}

CliResult CliManager::handleMappingTest(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "マッピングテスト機能は未実装です"));
}

CliResult CliManager::handleMappingValidate(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "マッピング検証機能は未実装です"));
}

CliResult CliManager::handleMetrics(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return CliResult(false, translate("metrics_usage", "使用法: metrics <show|reset|export>"));
    }
    
    std::string subcommand = args[1];
    
    if (subcommand == "show") {
        return handleMetricsShow(args);
    } else if (subcommand == "reset") {
        return handleMetricsReset(args);
    } else if (subcommand == "export") {
        return handleMetricsExport(args);
    } else {
        return CliResult(false, translate("unknown_subcommand", "不明なサブコマンドです: ") + subcommand);
    }
}

CliResult CliManager::handleMetricsShow(const std::vector<std::string>& args) {
    try {
        auto& metrics_collector = common::MetricsCollector::getInstance();
        
        // フォーマット指定を確認
        std::string format = "table"; // デフォルトはテーブル形式
        if (args.size() > 2) {
            if (args[2] == "--json") {
                format = "json";
            } else if (args[2] == "--prometheus") {
                format = "prometheus";
            }
        }
        
        if (format == "json") {
            std::string json_data = metrics_collector.getMetricsAsJson();
            return CliResult(true, "", json_data);
        } else if (format == "prometheus") {
            std::string prometheus_data = metrics_collector.getMetricsAsPrometheus();
            return CliResult(true, "", prometheus_data);
        } else {
            // テーブル形式
            auto all_metrics = metrics_collector.getAllMetrics();
            
            std::ostringstream output;
            output << translate("metrics_list", "メトリクス一覧") << ":\n";
            output << "==============\n\n";
            
            for (const auto& metric_pair : all_metrics) {
                const auto& metric = metric_pair.second;
                std::lock_guard<std::mutex> lock(metric->mutex);
                
                output << translate("name", "名前") << ": " << metric->name << "\n";
                output << translate("description", "説明") << ": " << metric->description << "\n";
                output << translate("type", "タイプ") << ": ";
                
                switch (metric->type) {
                    case common::MetricType::COUNTER: output << translate("counter", "カウンター"); break;
                    case common::MetricType::GAUGE: output << translate("gauge", "ゲージ"); break;
                    case common::MetricType::HISTOGRAM: output << translate("histogram", "ヒストグラム"); break;
                    case common::MetricType::SUMMARY: output << translate("summary", "サマリー"); break;
                }
                output << "\n";
                
                output << translate("value", "値") << ":\n";
                for (const auto& value_pair : metric->values) {
                    output << "  ";
                    if (!value_pair.second.labels.empty()) {
                        output << "{";
                        bool first = true;
                        for (const auto& label : value_pair.second.labels) {
                            if (!first) output << ",";
                            output << label.first << "=" << label.second;
                            first = false;
                        }
                        output << "} ";
                    }
                    output << value_pair.second.value;
                    
                    // タイムスタンプ
                    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                        value_pair.second.timestamp.time_since_epoch()).count();
                    output << " @" << timestamp;
                    output << "\n";
                }
                output << "\n";
            }
            
            return CliResult(true, "", output.str());
        }
    } catch (const std::exception& e) {
        return CliResult(false, translate("metrics_show_error", "メトリクス表示中にエラーが発生しました: ") + std::string(e.what()));
    }
}

CliResult CliManager::handleMetricsReset(const std::vector<std::string>& args) {
    try {
        auto& metrics_collector = common::MetricsCollector::getInstance();
        
        if (args.size() > 2) {
            if (args[2] == "--all") {
                // 全メトリクスをリセット
                metrics_collector.resetMetrics();
                return CliResult(true, translate("metrics_reset_all_success", "すべてのメトリクスをリセットしました"));
            } else {
                // 特定のメトリクスをリセット
                std::string metric_name = args[2];
                metrics_collector.resetMetrics(metric_name);
                return CliResult(true, translate("metrics_reset_success", "メトリクス '") + metric_name + translate("metrics_reset_success_suffix", "' をリセットしました"));
            }
        } else {
            return CliResult(false, translate("metrics_reset_usage", "使用法: metrics reset [メトリクス名] または metrics reset --all"));
        }
    } catch (const std::exception& e) {
        return CliResult(false, translate("metrics_reset_error", "メトリクスリセット中にエラーが発生しました: ") + std::string(e.what()));
    }
}

CliResult CliManager::handleMetricsExport(const std::vector<std::string>& args) {
    try {
        auto& metrics_collector = common::MetricsCollector::getInstance();
        
        std::string format = "json"; // デフォルト
        std::string output_file = "";
        
        // 引数を解析
        for (size_t i = 2; i < args.size(); ++i) {
            if (args[i] == "--format" && i + 1 < args.size()) {
                format = args[i + 1];
                ++i;
            } else if (args[i] == "--output" && i + 1 < args.size()) {
                output_file = args[i + 1];
                ++i;
            } else if (args[i] == "--json") {
                format = "json";
            } else if (args[i] == "--prometheus") {
                format = "prometheus";
            }
        }
        
        std::string data;
        if (format == "prometheus") {
            data = metrics_collector.getMetricsAsPrometheus();
        } else {
            data = metrics_collector.getMetricsAsJson();
        }
        
        if (output_file.empty()) {
            // 標準出力に出力
            return CliResult(true, "", data);
        } else {
            // ファイルに出力
            std::ofstream file(output_file);
            if (!file.is_open()) {
                return CliResult(false, translate("file_open_error", "ファイルを開けませんでした: ") + output_file);
            }
            
            file << data;
            file.close();
            
            return CliResult(true, translate("metrics_export_success", "メトリクスを ") + output_file + translate("metrics_export_success_suffix", " にエクスポートしました"));
        }
    } catch (const std::exception& e) {
        return CliResult(false, translate("metrics_export_error", "メトリクスエクスポート中にエラーが発生しました: ") + std::string(e.what()));
    }
}

CliResult CliManager::handleHealth(const std::vector<std::string>& args) {
    std::ostringstream oss;
    oss << translate("health_check_result", "ヘルスチェック結果") << ":\n";
    oss << "==================\n";
    oss << translate("status", "状態") << ": " << translate("normal", "正常") << "\n";
    oss << translate("timestamp", "タイムスタンプ") << ": " << std::time(nullptr) << "\n";
    oss << translate("uptime", "稼働時間") << ": " << std::time(nullptr) << " " << translate("seconds", "秒") << "\n";
    
    return CliResult(true, "", oss.str());
}

CliResult CliManager::handleLog(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "ログ管理機能は未実装です"));
}

CliResult CliManager::handleLogShow(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "ログ表示機能は未実装です"));
}

CliResult CliManager::handleLogLevel(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "ログレベル設定機能は未実装です"));
}

CliResult CliManager::handleLogRotate(const std::vector<std::string>& args) {
    return CliResult(false, translate("not_implemented", "ログローテーション機能は未実装です"));
}

// ユーティリティ関数の実装
std::string CliManager::formatJson(const std::string& json) {
    // 簡易的なJSON整形
    return json;
}

std::string CliManager::formatTable(const std::vector<std::vector<std::string>>& data, 
                                   const std::vector<std::string>& headers) {
    if (data.empty() || headers.empty()) {
        return "";
    }
    
    // 各列の最大幅を計算
    std::vector<size_t> col_widths(headers.size(), 0);
    
    // ヘッダーの幅
    for (size_t i = 0; i < headers.size(); ++i) {
        col_widths[i] = std::max(col_widths[i], headers[i].length());
    }
    
    // データの幅
    for (const auto& row : data) {
        for (size_t i = 0; i < std::min(row.size(), col_widths.size()); ++i) {
            col_widths[i] = std::max(col_widths[i], row[i].length());
        }
    }
    
    std::ostringstream oss;
    
    // ヘッダーを出力
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) oss << " | ";
        oss << std::setw(static_cast<int>(col_widths[i])) << std::left << headers[i];
    }
    oss << "\n";
    
    // 区切り線
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) oss << "-+-";
        oss << std::string(col_widths[i], '-');
    }
    oss << "\n";
    
    // データを出力
    for (const auto& row : data) {
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) oss << " | ";
            std::string cell_value = (i < row.size()) ? row[i] : "";
            oss << std::setw(static_cast<int>(col_widths[i])) << std::left << cell_value;
        }
        oss << "\n";
    }
    
    return oss.str();
}

std::string CliManager::formatDeviceInfo(const config::DeviceConfig& device) {
    std::ostringstream oss;
    oss << translate("device_info", "デバイス情報") << ":\n";
    oss << "=============\n";
    oss << translate("id", "ID") << ": " << device.getId() << "\n";
    oss << translate("name", "名前") << ": " << device.getId() << "\n"; // No getName() method
    oss << translate("description", "説明") << ": " << "" << "\n"; // No getDescription() method
    oss << translate("protocol", "プロトコル") << ": " << config::protocolToString(device.getProtocol()) << "\n";
    oss << translate("template", "テンプレート") << ": " << device.getTemplateId() << "\n";
    oss << translate("enabled", "有効") << ": " << translate("yes", "はい") << "\n";
    
    oss << "\n" << translate("connection_settings", "接続設定") << ":\n";
    const auto& connection_config = device.getConnection();
    
    std::visit([&oss, this](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, config::ModbusTcpConnectionConfig>) {
            oss << "  " << translate("ip", "IP") << ": " << arg.ip << "\n";
            oss << "  " << translate("port", "ポート") << ": " << arg.port << "\n";
            oss << "  " << translate("unit_id", "ユニットID") << ": " << arg.unit_id << "\n";
        } else if constexpr (std::is_same_v<T, config::ModbusRtuConnectionConfig>) {
            oss << "  " << translate("port", "ポート") << ": " << arg.port << "\n";
            oss << "  " << translate("baud_rate", "ボーレート") << ": " << arg.baud_rate << "\n";
        } else if constexpr (std::is_same_v<T, config::EchonetLiteConnectionConfig>) {
            oss << "  " << translate("ip", "IP") << ": " << arg.ip << "\n";
        }
    }, connection_config);
    
    return oss.str();
}

std::string CliManager::formatMetrics(const std::map<std::string, double>& metrics) {
    std::ostringstream oss;
    oss << translate("metrics", "メトリクス") << ":\n";
    oss << "==========\n";
    
    for (const auto& metric : metrics) {
        oss << metric.first << ": " << metric.second << "\n";
    }
    
    return oss.str();
}

} // namespace api
} // namespace ocpp_gateway