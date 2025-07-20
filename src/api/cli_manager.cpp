#include "ocpp_gateway/api/cli_manager.h"
#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/metrics_collector.h"
#include <json/json.h>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace ocpp_gateway {
namespace api {

CliManager::CliManager() : config_manager_(config::ConfigManager::getInstance()) {
    // 基本コマンドの登録
    registerCommand("help", "ヘルプメッセージを表示", 
                   [this](const std::vector<std::string>& args) { return handleHelp(args); });
    
    registerCommand("version", "バージョン情報を表示",
                   [this](const std::vector<std::string>& args) { return handleVersion(args); });
    
    registerCommand("status", "システム状態を表示",
                   [this](const std::vector<std::string>& args) { return handleStatus(args); });
    
    registerCommand("config", "設定管理コマンド",
                   [this](const std::vector<std::string>& args) { return handleConfig(args); });
    
    registerCommand("device", "デバイス管理コマンド",
                   [this](const std::vector<std::string>& args) { return handleDevice(args); });
    
    registerCommand("mapping", "マッピング管理コマンド",
                   [this](const std::vector<std::string>& args) { return handleMapping(args); });
    
    registerCommand("metrics", "メトリクス管理コマンド",
                   [this](const std::vector<std::string>& args) { return handleMetrics(args); });
    
    registerCommand("health", "ヘルスチェック",
                   [this](const std::vector<std::string>& args) { return handleHealth(args); });
    
    registerCommand("log", "ログ管理コマンド",
                   [this](const std::vector<std::string>& args) { return handleLog(args); });
}

CliManager::~CliManager() {
}

CliResult CliManager::executeCommand(const std::vector<std::string>& args) {
    if (args.empty()) {
        return CliResult(false, "コマンドが指定されていません。'help'でヘルプを確認してください。");
    }

    std::string command = args[0];
    auto it = commands_.find(command);
    
    if (it == commands_.end()) {
        return CliResult(false, "不明なコマンドです: " + command + "。'help'でヘルプを確認してください。");
    }

    try {
        return it->second.handler(args);
    } catch (const std::exception& e) {
        LOG_ERROR("CLIコマンド実行エラー [{}]: {}", command, e.what());
        return CliResult(false, "コマンド実行中にエラーが発生しました: " + std::string(e.what()));
    }
}

void CliManager::registerCommand(const std::string& command, 
                                const std::string& description,
                                CliCommandHandler handler) {
    commands_[command] = {description, handler};
}

std::string CliManager::getHelpMessage() const {
    std::ostringstream oss;
    oss << "OCPP 2.0.1 ゲートウェイ・ミドルウェア CLI\n\n";
    oss << "利用可能なコマンド:\n";
    
    // 最大コマンド長を計算
    size_t max_cmd_length = 0;
    for (const auto& cmd : commands_) {
        max_cmd_length = std::max(max_cmd_length, cmd.first.length());
    }
    
    for (const auto& cmd : commands_) {
        oss << "  " << std::setw(static_cast<int>(max_cmd_length)) << std::left 
            << cmd.first << " - " << cmd.second.description << "\n";
    }
    
    oss << "\n詳細なヘルプは 'help <コマンド>' で確認できます。\n";
    return oss.str();
}

CliResult CliManager::handleHelp(const std::vector<std::string>& args) {
    if (args.size() == 1) {
        return CliResult(true, "", getHelpMessage());
    } else if (args.size() == 2) {
        std::string command = args[1];
        auto it = commands_.find(command);
        
        if (it == commands_.end()) {
            return CliResult(false, "不明なコマンドです: " + command);
        }
        
        std::ostringstream oss;
        oss << "コマンド: " << command << "\n";
        oss << "説明: " << it->second.description << "\n\n";
        
        // コマンド固有のヘルプ
        if (command == "config") {
            oss << "サブコマンド:\n";
            oss << "  show     - 現在の設定を表示\n";
            oss << "  reload   - 設定を再読み込み\n";
            oss << "  validate - 設定を検証\n";
            oss << "  backup   - 設定をバックアップ\n";
            oss << "  restore  - 設定を復元\n";
        } else if (command == "device") {
            oss << "サブコマンド:\n";
            oss << "  list              - デバイス一覧を表示\n";
            oss << "  show <id>         - デバイス詳細を表示\n";
            oss << "  add <file>        - デバイスを追加\n";
            oss << "  update <id> <file> - デバイス設定を更新\n";
            oss << "  delete <id>       - デバイスを削除\n";
            oss << "  test <id>         - デバイス通信をテスト\n";
        } else if (command == "mapping") {
            oss << "サブコマンド:\n";
            oss << "  list               - マッピング一覧を表示\n";
            oss << "  show <template>    - マッピング詳細を表示\n";
            oss << "  test <device_id>   - マッピングをテスト\n";
            oss << "  validate <file>    - マッピングファイルを検証\n";
        } else if (command == "metrics") {
            oss << "サブコマンド:\n";
            oss << "  show       - メトリクスを表示\n";
            oss << "  reset      - メトリクスをリセット\n";
            oss << "  export     - メトリクスをエクスポート\n";
        } else if (command == "log") {
            oss << "サブコマンド:\n";
            oss << "  show       - ログを表示\n";
            oss << "  level <lvl> - ログレベルを設定\n";
            oss << "  rotate     - ログローテーションを実行\n";
        }
        
        return CliResult(true, "", oss.str());
    } else {
        return CliResult(false, "使用法: help [コマンド]");
    }
}

CliResult CliManager::handleVersion(const std::vector<std::string>& args) {
    std::ostringstream oss;
    oss << "OCPP 2.0.1 ゲートウェイ・ミドルウェア\n";
    oss << "バージョン: 1.0.0\n";
    oss << "ビルド日時: " << __DATE__ << " " << __TIME__ << "\n";
    oss << "C++標準: C++17\n";
    
    return CliResult(true, "", oss.str());
}

CliResult CliManager::handleStatus(const std::vector<std::string>& args) {
    try {
        std::ostringstream oss;
        oss << "システム状態:\n";
        oss << "================\n";
        
        // システム設定情報
        const auto& system_config = config_manager_.getSystemConfig();
        oss << "ログレベル: " << system_config.getLogLevel() << "\n";
        oss << "最大充電ポイント数: " << system_config.getMaxChargePoints() << "\n";
        
        // デバイス情報
        const auto& device_configs = config_manager_.getDeviceConfigs();
        oss << "登録デバイス数: " << device_configs.getDevices().size() << "\n";
        
        // メトリクス情報（簡易版）
        oss << "\nメトリクス:\n";
        oss << "----------\n";
        oss << "稼働時間: " << std::time(nullptr) << " 秒\n";
        oss << "設定最終更新: " << "不明" << "\n";
        
        return CliResult(true, "", oss.str());
    } catch (const std::exception& e) {
        return CliResult(false, "システム状態の取得に失敗しました: " + std::string(e.what()));
    }
}

CliResult CliManager::handleConfig(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return CliResult(false, "使用法: config <show|reload|validate|backup|restore> [オプション]");
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
        return CliResult(false, "不明なサブコマンドです: " + subcommand);
    }
}

CliResult CliManager::handleConfigShow(const std::vector<std::string>& args) {
    try {
        std::ostringstream oss;
        oss << "現在の設定:\n";
        oss << "============\n\n";
        
        // システム設定
        const auto& system_config = config_manager_.getSystemConfig();
        oss << "[システム設定]\n";
        oss << "ログレベル: " << system_config.getLogLevel() << "\n";
        oss << "最大充電ポイント数: " << system_config.getMaxChargePoints() << "\n";
        
        // CSMS設定
        const auto& csms_config = config_manager_.getCsmsConfig();
        oss << "\n[CSMS設定]\n";
        oss << "URL: " << csms_config.getUrl() << "\n";
        oss << "再接続間隔: " << csms_config.getReconnectInterval() << " 秒\n";
        oss << "最大再接続回数: " << csms_config.getMaxReconnectAttempts() << "\n";
        
        // デバイス設定
        const auto& device_configs = config_manager_.getDeviceConfigs();
        oss << "\n[デバイス設定]\n";
        oss << "デバイス数: " << device_configs.getDevices().size() << "\n";
        
        for (const auto& device : device_configs.getDevices()) {
            oss << "  - " << device.getId() << " (" << device.getName() << ")\n";
            oss << "    プロトコル: " << device.getProtocol() << "\n";
            oss << "    テンプレート: " << device.getTemplateName() << "\n";
            oss << "    有効: " << (device.isEnabled() ? "はい" : "いいえ") << "\n";
        }
        
        return CliResult(true, "", oss.str());
    } catch (const std::exception& e) {
        return CliResult(false, "設定の取得に失敗しました: " + std::string(e.what()));
    }
}

CliResult CliManager::handleConfigReload(const std::vector<std::string>& args) {
    try {
        if (config_manager_.reloadAllConfigs()) {
            return CliResult(true, "設定を再読み込みしました");
        } else {
            return CliResult(false, "設定の再読み込みに失敗しました");
        }
    } catch (const std::exception& e) {
        return CliResult(false, "設定の再読み込み中にエラーが発生しました: " + std::string(e.what()));
    }
}

CliResult CliManager::handleConfigValidate(const std::vector<std::string>& args) {
    try {
        config_manager_.validateAllConfigs();
        return CliResult(true, "設定の検証が成功しました");
    } catch (const config::ConfigValidationError& e) {
        return CliResult(false, "設定の検証に失敗しました: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return CliResult(false, "設定の検証中にエラーが発生しました: " + std::string(e.what()));
    }
}

CliResult CliManager::handleConfigBackup(const std::vector<std::string>& args) {
    return CliResult(false, "設定バックアップ機能は未実装です");
}

CliResult CliManager::handleConfigRestore(const std::vector<std::string>& args) {
    return CliResult(false, "設定復元機能は未実装です");
}

CliResult CliManager::handleDevice(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return CliResult(false, "使用法: device <list|show|add|update|delete|test> [オプション]");
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
        return CliResult(false, "不明なサブコマンドです: " + subcommand);
    }
}

CliResult CliManager::handleDeviceList(const std::vector<std::string>& args) {
    try {
        const auto& device_configs = config_manager_.getDeviceConfigs();
        const auto& devices = device_configs.getDevices();
        
        if (devices.empty()) {
            return CliResult(true, "", "登録されているデバイスはありません。\n");
        }
        
        std::vector<std::vector<std::string>> table_data;
        std::vector<std::string> headers = {"ID", "名前", "プロトコル", "テンプレート", "有効"};
        
        for (const auto& device : devices) {
            std::vector<std::string> row;
            row.push_back(device.getId());
            row.push_back(device.getName());
            row.push_back(device.getProtocol());
            row.push_back(device.getTemplateName());
            row.push_back(device.isEnabled() ? "はい" : "いいえ");
            table_data.push_back(row);
        }
        
        std::string output = "デバイス一覧:\n";
        output += formatTable(table_data, headers);
        output += "\n総数: " + std::to_string(devices.size()) + " デバイス\n";
        
        return CliResult(true, "", output);
    } catch (const std::exception& e) {
        return CliResult(false, "デバイス一覧の取得に失敗しました: " + std::string(e.what()));
    }
}

CliResult CliManager::handleDeviceShow(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return CliResult(false, "使用法: device show <device_id>");
    }
    
    std::string device_id = args[2];
    
    try {
        auto device_opt = config_manager_.getDeviceConfig(device_id);
        if (!device_opt) {
            return CliResult(false, "デバイスが見つかりません: " + device_id);
        }
        
        const auto& device = device_opt.value();
        std::string output = formatDeviceInfo(device);
        
        return CliResult(true, "", output);
    } catch (const std::exception& e) {
        return CliResult(false, "デバイス情報の取得に失敗しました: " + std::string(e.what()));
    }
}

// 残りのメソッドは簡易実装
CliResult CliManager::handleDeviceAdd(const std::vector<std::string>& args) {
    return CliResult(false, "デバイス追加機能は未実装です");
}

CliResult CliManager::handleDeviceUpdate(const std::vector<std::string>& args) {
    return CliResult(false, "デバイス更新機能は未実装です");
}

CliResult CliManager::handleDeviceDelete(const std::vector<std::string>& args) {
    return CliResult(false, "デバイス削除機能は未実装です");
}

CliResult CliManager::handleDeviceTest(const std::vector<std::string>& args) {
    return CliResult(false, "デバイステスト機能は未実装です");
}

CliResult CliManager::handleMapping(const std::vector<std::string>& args) {
    return CliResult(false, "マッピング管理機能は未実装です");
}

CliResult CliManager::handleMappingList(const std::vector<std::string>& args) {
    return CliResult(false, "マッピング一覧機能は未実装です");
}

CliResult CliManager::handleMappingShow(const std::vector<std::string>& args) {
    return CliResult(false, "マッピング表示機能は未実装です");
}

CliResult CliManager::handleMappingTest(const std::vector<std::string>& args) {
    return CliResult(false, "マッピングテスト機能は未実装です");
}

CliResult CliManager::handleMappingValidate(const std::vector<std::string>& args) {
    return CliResult(false, "マッピング検証機能は未実装です");
}

CliResult CliManager::handleMetrics(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return CliResult(false, "使用法: metrics <show|reset|export>");
    }
    
    std::string subcommand = args[1];
    
    if (subcommand == "show") {
        return handleMetricsShow(args);
    } else if (subcommand == "reset") {
        return handleMetricsReset(args);
    } else if (subcommand == "export") {
        return handleMetricsExport(args);
    } else {
        return CliResult(false, "不明なサブコマンドです: " + subcommand);
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
            output << "メトリクス一覧:\n";
            output << "==============\n\n";
            
            for (const auto& metric_pair : all_metrics) {
                const auto& metric = metric_pair.second;
                std::lock_guard<std::mutex> lock(metric->mutex);
                
                output << "名前: " << metric->name << "\n";
                output << "説明: " << metric->description << "\n";
                output << "タイプ: ";
                
                switch (metric->type) {
                    case common::MetricType::COUNTER: output << "カウンター"; break;
                    case common::MetricType::GAUGE: output << "ゲージ"; break;
                    case common::MetricType::HISTOGRAM: output << "ヒストグラム"; break;
                    case common::MetricType::SUMMARY: output << "サマリー"; break;
                }
                output << "\n";
                
                output << "値:\n";
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
        return CliResult(false, "メトリクス表示中にエラーが発生しました: " + std::string(e.what()));
    }
}

CliResult CliManager::handleMetricsReset(const std::vector<std::string>& args) {
    try {
        auto& metrics_collector = common::MetricsCollector::getInstance();
        
        if (args.size() > 2) {
            // 特定のメトリクスをリセット
            std::string metric_name = args[2];
            metrics_collector.resetMetrics(metric_name);
            return CliResult(true, "メトリクス '" + metric_name + "' をリセットしました");
        } else {
            // 全メトリクスをリセット
            if (args.size() > 2 && args[2] == "--all") {
                metrics_collector.resetMetrics();
                return CliResult(true, "すべてのメトリクスをリセットしました");
            } else {
                return CliResult(false, "使用法: metrics reset [メトリクス名] または metrics reset --all");
            }
        }
    } catch (const std::exception& e) {
        return CliResult(false, "メトリクスリセット中にエラーが発生しました: " + std::string(e.what()));
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
                return CliResult(false, "ファイルを開けませんでした: " + output_file);
            }
            
            file << data;
            file.close();
            
            return CliResult(true, "メトリクスを " + output_file + " にエクスポートしました");
        }
    } catch (const std::exception& e) {
        return CliResult(false, "メトリクスエクスポート中にエラーが発生しました: " + std::string(e.what()));
    }
}

CliResult CliManager::handleHealth(const std::vector<std::string>& args) {
    std::ostringstream oss;
    oss << "ヘルスチェック結果:\n";
    oss << "==================\n";
    oss << "状態: 正常\n";
    oss << "タイムスタンプ: " << std::time(nullptr) << "\n";
    oss << "稼働時間: " << std::time(nullptr) << " 秒\n";
    
    return CliResult(true, "", oss.str());
}

CliResult CliManager::handleLog(const std::vector<std::string>& args) {
    return CliResult(false, "ログ管理機能は未実装です");
}

CliResult CliManager::handleLogShow(const std::vector<std::string>& args) {
    return CliResult(false, "ログ表示機能は未実装です");
}

CliResult CliManager::handleLogLevel(const std::vector<std::string>& args) {
    return CliResult(false, "ログレベル設定機能は未実装です");
}

CliResult CliManager::handleLogRotate(const std::vector<std::string>& args) {
    return CliResult(false, "ログローテーション機能は未実装です");
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
    oss << "デバイス情報:\n";
    oss << "=============\n";
    oss << "ID: " << device.getId() << "\n";
    oss << "名前: " << device.getName() << "\n";
    oss << "説明: " << device.getDescription() << "\n";
    oss << "プロトコル: " << device.getProtocol() << "\n";
    oss << "テンプレート: " << device.getTemplateName() << "\n";
    oss << "有効: " << (device.isEnabled() ? "はい" : "いいえ") << "\n";
    
    oss << "\n接続設定:\n";
    const auto& params = device.getConnectionParameters();
    for (const auto& param : params) {
        oss << "  " << param.first << ": " << param.second << "\n";
    }
    
    return oss.str();
}

std::string CliManager::formatMetrics(const std::map<std::string, double>& metrics) {
    std::ostringstream oss;
    oss << "メトリクス:\n";
    oss << "==========\n";
    
    for (const auto& metric : metrics) {
        oss << metric.first << ": " << metric.second << "\n";
    }
    
    return oss.str();
}

} // namespace api
} // namespace ocpp_gateway 