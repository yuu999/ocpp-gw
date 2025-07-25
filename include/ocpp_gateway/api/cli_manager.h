#pragma once

#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/config_manager.h"
#include <string>
#include <vector>
#include <functional>
#include <map>

namespace ocpp_gateway {
namespace api {

/**
 * @brief CLIコマンドの結果構造体
 */
struct CliResult {
    bool success;
    std::string message;
    std::string output;
    
    CliResult() : success(true) {}
    CliResult(bool s, const std::string& m) : success(s), message(m) {}
    CliResult(bool s, const std::string& m, const std::string& o) : success(s), message(m), output(o) {}
};

/**
 * @brief CLIコマンドハンドラーの型定義
 */
using CliCommandHandler = std::function<CliResult(const std::vector<std::string>&)>;

/**
 * @brief CLI管理クラス
 * 
 * コマンドラインインターフェースを提供し、システムの管理機能を実装します。
 * 設定管理、デバイス管理、監視・診断機能などのコマンドを提供します。
 */
class CliManager {
public:
    /**
     * @brief コンストラクタ
     */
    CliManager();

    /**
     * @brief デストラクタ
     */
    ~CliManager();

    /**
     * @brief CLIコマンドを実行
     * @param args コマンドライン引数
     * @return 実行結果
     */
    CliResult executeCommand(const std::vector<std::string>& args);

    /**
     * @brief コマンドを登録
     * @param command コマンド名
     * @param description コマンドの説明
     * @param handler ハンドラー関数
     */
    void registerCommand(const std::string& command, 
                        const std::string& description,
                        CliCommandHandler handler);

    /**
     * @brief ヘルプメッセージを表示
     * @return ヘルプメッセージ
     */
    std::string getHelpMessage() const;

private:
    // コマンドハンドラー
    CliResult handleHelp(const std::vector<std::string>& args);
    CliResult handleVersion(const std::vector<std::string>& args);
    CliResult handleStatus(const std::vector<std::string>& args);
    CliResult handleConfig(const std::vector<std::string>& args);
    CliResult handleDevice(const std::vector<std::string>& args);
    CliResult handleMapping(const std::vector<std::string>& args);
    CliResult handleMetrics(const std::vector<std::string>& args);
    CliResult handleHealth(const std::vector<std::string>& args);
    CliResult handleLog(const std::vector<std::string>& args);

    // 設定管理コマンド
    CliResult handleConfigShow(const std::vector<std::string>& args);
    CliResult handleConfigReload(const std::vector<std::string>& args);
    CliResult handleConfigValidate(const std::vector<std::string>& args);
    CliResult handleConfigBackup(const std::vector<std::string>& args);
    CliResult handleConfigRestore(const std::vector<std::string>& args);

    // デバイス管理コマンド
    CliResult handleDeviceList(const std::vector<std::string>& args);
    CliResult handleDeviceShow(const std::vector<std::string>& args);
    CliResult handleDeviceAdd(const std::vector<std::string>& args);
    CliResult handleDeviceUpdate(const std::vector<std::string>& args);
    CliResult handleDeviceDelete(const std::vector<std::string>& args);
    CliResult handleDeviceTest(const std::vector<std::string>& args);

    // マッピング管理コマンド
    CliResult handleMappingList(const std::vector<std::string>& args) __attribute__((unused));
    CliResult handleMappingShow(const std::vector<std::string>& args) __attribute__((unused));
    CliResult handleMappingTest(const std::vector<std::string>& args) __attribute__((unused));
    CliResult handleMappingValidate(const std::vector<std::string>& args) __attribute__((unused));

    // メトリクスコマンド
    CliResult handleMetricsShow(const std::vector<std::string>& args);
    CliResult handleMetricsReset(const std::vector<std::string>& args);
    CliResult handleMetricsExport(const std::vector<std::string>& args);

    // ログコマンド
    CliResult handleLogShow(const std::vector<std::string>& args) __attribute__((unused));
    CliResult handleLogLevel(const std::vector<std::string>& args) __attribute__((unused));
    CliResult handleLogRotate(const std::vector<std::string>& args) __attribute__((unused));

    // ユーティリティ関数
    std::string formatJson(const std::string& json) __attribute__((unused));
    std::string formatTable(const std::vector<std::vector<std::string>>& data, 
                           const std::vector<std::string>& headers);
    std::string formatDeviceInfo(const config::DeviceConfig& device);
    std::string formatMetrics(const std::map<std::string, double>& metrics) __attribute__((unused));
    
    // 国際化サポート
    std::string translate(const std::string& key, const std::string& default_value = "") const;
    bool setLanguage(const std::string& language);
    std::string getCurrentLanguage() const;

    struct CommandInfo {
        std::string description;
        CliCommandHandler handler;
    };

    std::map<std::string, CommandInfo> commands_;
    config::ConfigManager& config_manager_;
};

} // namespace api
} // namespace ocpp_gateway 