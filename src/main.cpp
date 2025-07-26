#include <iostream>
#include <string>
#include <signal.h>
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>

#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/config_manager.h"
#include "ocpp_gateway/common/metrics_collector.h"
#include "ocpp_gateway/common/prometheus_exporter.h"
#include "ocpp_gateway/api/admin_api.h"
#include "ocpp_gateway/api/cli_manager.h"
#include "ocpp_gateway/api/web_ui.h"
#include "ocpp_gateway/ocpp/ocpp_client_manager.h"
#include "ocpp_gateway/device/device_adapter.h"
#include "ocpp_gateway/mapping/mapping_engine.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

// グローバル変数（シグナルハンドラー用）
std::atomic<bool> g_running(true);

// グローバルコンポーネント
std::unique_ptr<ocpp_gateway::api::AdminApi> g_admin_api;
std::unique_ptr<ocpp_gateway::api::CliManager> g_cli_manager;
std::unique_ptr<ocpp_gateway::api::WebUI> g_web_ui;
std::unique_ptr<ocpp_gateway::common::PrometheusExporter> g_prometheus_exporter;

/**
 * @brief シグナルハンドラー
 * @param signal シグナル番号
 */
void signalHandler(int signal) {
    LOG_INFO("シグナルを受信しました: {}", signal);
    g_running = false;
}

/**
 * @brief シグナルハンドラーの設定
 */
void setupSignalHandlers() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
}

/**
 * @brief アプリケーションの初期化
 * @param config_path 設定ファイルパス
 * @param verbose 詳細ログ出力フラグ
 * @return 初期化成功時true
 */
bool initializeApplication(const std::string& config_path, bool verbose) {
    try {
        // ログレベルの設定
        std::string log_level = verbose ? "debug" : "info";
        std::string log_file = "/var/log/ocpp-gateway/ocpp-gateway.log";
        
        // ロガーの初期化
        ocpp_gateway::common::LogConfig log_config;
        log_config.log_level = log_level;
        log_config.log_file = log_file;
        log_config.max_size_mb = 10;
        log_config.max_files = 5;
        log_config.console_output = true;
        log_config.file_output = true;
        log_config.daily_rotation = false;
        log_config.compress_logs = true;
        
        if (!ocpp_gateway::common::Logger::initialize(log_config)) {
            std::cerr << "ロガーの初期化に失敗しました" << std::endl;
            return false;
        }
        
        LOG_INFO("OCPP 2.0.1 ゲートウェイ・ミドルウェアを開始します");
        LOG_INFO("設定ファイル: {}", config_path);
        
        // 設定ファイルの存在確認
        if (!fs::exists(config_path)) {
            LOG_ERROR("設定ファイルが見つかりません: {}", config_path);
            return false;
        }
        
        // 設定ディレクトリの取得
        fs::path config_file_path(config_path);
        std::string config_dir = config_file_path.parent_path().string();
        
        // 設定マネージャーの初期化
        auto& config_manager = ocpp_gateway::config::ConfigManager::getInstance();
        if (!config_manager.initialize(config_dir)) {
            LOG_ERROR("設定マネージャーの初期化に失敗しました");
            return false;
        }
        
        // 設定の検証
        try {
            config_manager.validateAllConfigs();
            LOG_INFO("設定の検証が完了しました");
        } catch (const ocpp_gateway::config::ConfigValidationError& e) {
            LOG_ERROR("設定の検証に失敗しました: {}", e.what());
            return false;
        }
        
        // メトリクス収集の初期化
        auto& metrics_collector = ocpp_gateway::common::MetricsCollector::getInstance();
        if (!metrics_collector.initialize()) {
            LOG_ERROR("メトリクス収集の初期化に失敗しました");
            return false;
        }
        LOG_INFO("メトリクス収集を初期化しました");
        
        // 管理APIの初期化
        const auto& system_config = config_manager.getSystemConfig();
        // Note: Admin API configuration methods don't exist in SystemConfig yet
        // Using default values for now
        int admin_port = 8080; // Default admin API port
        std::string admin_bind_address = "0.0.0.0"; // Default bind address
        
        g_admin_api = std::make_unique<ocpp_gateway::api::AdminApi>(admin_port, admin_bind_address);
        
        // 認証設定
        // Note: Authentication configuration method doesn't exist in SystemConfig yet
        // Using default values for now
        bool auth_enabled = false; // Default: no authentication
        if (auth_enabled) {
            g_admin_api->setAuthentication(true, "admin", "password");
            LOG_INFO("管理API認証を有効にしました");
        }
        
        if (!g_admin_api->start()) {
            LOG_ERROR("管理APIの開始に失敗しました");
            return false;
        }
        LOG_INFO("管理APIを開始しました: {}:{}", admin_bind_address, admin_port);
        
        // CLIマネージャーの初期化
        g_cli_manager = std::make_unique<ocpp_gateway::api::CliManager>();
        LOG_INFO("CLIマネージャーを初期化しました");
        
        // WebUIの初期化
        g_web_ui = std::make_unique<ocpp_gateway::api::WebUI>(8081, "0.0.0.0", "web");
        if (!g_web_ui->start()) {
            LOG_ERROR("WebUIの開始に失敗しました");
            return false;
        }
        LOG_INFO("WebUIを初期化しました: {}:{}", "0.0.0.0", 8081);
        
        // Prometheusエクスポーターの初期化
        g_prometheus_exporter = std::make_unique<ocpp_gateway::common::PrometheusExporter>(9090, "0.0.0.0", "/metrics");
        if (!g_prometheus_exporter->start()) {
            LOG_ERROR("Prometheusエクスポーターの開始に失敗しました");
            return false;
        }
        LOG_INFO("Prometheusエクスポーターを初期化しました: {}:{}{}", "0.0.0.0", 9090, "/metrics");
        
        // TODO: 他のコンポーネントの初期化
        // 1. マッピングエンジンの初期化
        // 2. デバイスアダプターの初期化
        // 3. OCPPクライアントマネージャーの初期化
        
        LOG_INFO("アプリケーションの初期化が完了しました");
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "アプリケーション初期化エラー: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief アプリケーションの終了処理
 */
void cleanupApplication() {
    LOG_INFO("アプリケーションを終了しています...");
    
    // 管理APIの終了
    if (g_admin_api) {
        g_admin_api->stop();
        LOG_INFO("管理APIを停止しました");
    }
    
    // Prometheusエクスポーターの終了
    if (g_prometheus_exporter) {
        g_prometheus_exporter->stop();
        g_prometheus_exporter.reset();
        LOG_INFO("Prometheusエクスポーターを停止しました");
    }
    
    // WebUIの終了
    if (g_web_ui) {
        g_web_ui->stop();
        g_web_ui.reset();
        LOG_INFO("WebUIを停止しました");
    }
    
    // メトリクス収集の終了
    auto& metrics_collector = ocpp_gateway::common::MetricsCollector::getInstance();
    metrics_collector.shutdown();
    LOG_INFO("メトリクス収集を停止しました");
    
    // TODO: 各コンポーネントの終了処理
    // 1. OCPPクライアントマネージャーの終了
    // 2. デバイスアダプターの終了
    // 3. マッピングエンジンの終了
    
    ocpp_gateway::common::Logger::flush();
    LOG_INFO("アプリケーションの終了処理が完了しました");
}

int main(int argc, char* argv[]) {
    try {
        // コマンドラインオプションの解析
        po::options_description desc("OCPP 2.0.1 ゲートウェイ・ミドルウェア");
        desc.add_options()
            ("help,h", "ヘルプメッセージを表示")
            ("config,c", po::value<std::string>()->default_value("/etc/ocpp-gw/system.yaml"), "設定ファイルのパス")
            ("verbose,v", "詳細ログ出力を有効化")
            ("daemon,d", "デーモンモードで実行")
            ("cli", "CLIモードで実行")
            ("cli-command", po::value<std::vector<std::string>>(), "CLIコマンド");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // ヘルプの表示
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        // 設定ファイルパスと詳細ログフラグの取得
        std::string config_path = vm["config"].as<std::string>();
        bool verbose = vm.count("verbose") > 0;
        bool cli_mode = vm.count("cli") > 0;

        // CLIモードの処理
        if (cli_mode) {
            if (!initializeApplication(config_path, verbose)) {
                std::cerr << "アプリケーションの初期化に失敗しました" << std::endl;
                return 1;
            }
            
            std::vector<std::string> cli_args;
            if (vm.count("cli-command")) {
                cli_args = vm["cli-command"].as<std::vector<std::string>>();
            }
            
            if (cli_args.empty()) {
                // インタラクティブモード
                std::cout << "OCPP Gateway CLI (Ctrl+Cで終了)" << std::endl;
                std::string line;
                while (g_running && std::getline(std::cin, line)) {
                    if (line.empty()) continue;
                    
                    // コマンドを解析
                    std::vector<std::string> args;
                    std::istringstream iss(line);
                    std::string arg;
                    while (iss >> arg) {
                        args.push_back(arg);
                    }
                    
                    if (!args.empty()) {
                        auto result = g_cli_manager->executeCommand(args);
                        if (!result.success) {
                            std::cerr << "エラー: " << result.message << std::endl;
                        } else if (!result.output.empty()) {
                            std::cout << result.output << std::endl;
                        }
                    }
                }
            } else {
                // 単発コマンド実行
                auto result = g_cli_manager->executeCommand(cli_args);
                if (!result.success) {
                    std::cerr << "エラー: " << result.message << std::endl;
                    return 1;
                } else if (!result.output.empty()) {
                    std::cout << result.output << std::endl;
                }
            }
            
            cleanupApplication();
            return 0;
        }

        // シグナルハンドラーの設定
        setupSignalHandlers();

        // アプリケーションの初期化
        if (!initializeApplication(config_path, verbose)) {
            std::cerr << "アプリケーションの初期化に失敗しました" << std::endl;
            return 1;
        }

        // メインループ
        LOG_INFO("メインループを開始します。Ctrl+Cで終了してください");
        
        while (g_running) {
            // TODO: メインループでの処理
            // 1. デバイス状態の監視
            // 2. OCPPメッセージの処理
            // 3. エラー処理
            // 4. 統計情報の更新
            // メトリクスの更新は別スレッドで実行される
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 終了処理
        cleanupApplication();
        
    } catch (const std::exception& e) {
        std::cerr << "エラー: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}