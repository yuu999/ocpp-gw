#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <functional>

namespace ocpp_gateway {

// Forward declarations
namespace api {
    class AdminApi;
    class CliManager;
    class WebUI;
}

namespace common {
    class PrometheusExporter;
    class ConfigManager;
    class MetricsCollector;
}

namespace ocpp {
    class OcppClientManager;
}

// namespace device {
//     class DeviceManager;
// }

namespace mapping {
    class MappingEngine;
}

namespace config {
    class ConfigManager;
}

/**
 * @brief アプリケーションのメインクラス
 * 
 * システム全体のライフサイクル管理、コンポーネントの初期化・終了、
 * 設定管理、シグナルハンドリングを担当します。
 */
class Application {
public:
    /**
     * @brief アプリケーション設定構造体
     */
    struct Config {
        std::string config_path;
        bool verbose = false;
        bool daemon_mode = false;
        bool cli_mode = false;
        std::vector<std::string> cli_args;
    };

    /**
     * @brief アプリケーション実行結果
     */
    enum class ExitCode {
        SUCCESS = 0,
        INITIALIZATION_FAILED = 1,
        CONFIG_ERROR = 2,
        RUNTIME_ERROR = 3
    };

    Application();
    ~Application();

    /**
     * @brief アプリケーションの初期化
     * @param config アプリケーション設定
     * @return 初期化成功時true
     */
    bool initialize(const Config& config);

    /**
     * @brief アプリケーションの実行
     * @return 終了コード
     */
    ExitCode run();

    /**
     * @brief アプリケーションの停止要求
     */
    void requestStop();

    /**
     * @brief 動作状態の確認
     * @return 動作中の場合true
     */
    bool isRunning() const;

    /**
     * @brief CLIコマンドの実行
     * @param args コマンド引数
     * @return 実行成功時true
     */
    bool executeCli(const std::vector<std::string>& args);

private:
    /**
     * @brief シグナルハンドラーの設定
     */
    void setupSignalHandlers();

    /**
     * @brief システムコンポーネントの初期化
     * @return 初期化成功時true
     */
    bool initializeComponents();

    /**
     * @brief デーモンモードでの実行
     * @return 終了コード
     */
    ExitCode runDaemon();

    /**
     * @brief CLIモードでの実行
     * @return 終了コード
     */
    ExitCode runCli();

    /**
     * @brief インタラクティブCLIの実行
     * @return 終了コード
     */
    ExitCode runInteractiveCli();

    /**
     * @brief 単発CLIコマンドの実行
     * @return 終了コード
     */
    ExitCode runSingleCli();

    /**
     * @brief メインループの実行
     */
    void runMainLoop();

    /**
     * @brief システム終了処理
     */
    void cleanup();

    /**
     * @brief 設定再読み込み
     */
    void reloadConfiguration();

    // Configuration
    Config config_;
    
    // Runtime state
    std::atomic<bool> running_;
    std::atomic<bool> reload_requested_;

    // Service components  
    std::unique_ptr<api::AdminApi> admin_api_;
    std::unique_ptr<api::CliManager> cli_manager_;
    std::unique_ptr<api::WebUI> web_ui_;
    std::unique_ptr<common::PrometheusExporter> prometheus_exporter_;
    
    // Business logic components (TODO: implement in Phase 2)
    // std::unique_ptr<mapping::MappingEngine> mapping_engine_;
    // std::unique_ptr<device::DeviceManager> device_manager_;
    // std::unique_ptr<ocpp::OcppClientManager> ocpp_client_manager_;

    // Signal handling
    static Application* instance_;
    static void signalHandler(int signal);
};

} // namespace ocpp_gateway 