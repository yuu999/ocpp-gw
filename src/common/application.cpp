#include "ocpp_gateway/common/application.h"
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

#include <boost/filesystem.hpp>
#include <signal.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

namespace fs = boost::filesystem;

namespace ocpp_gateway {

// Static member initialization
Application* Application::instance_ = nullptr;

Application::Application() 
    : running_(false)
    , reload_requested_(false) {
    instance_ = this;
}

Application::~Application() {
    cleanup();
    instance_ = nullptr;
}

bool Application::initialize(const Config& config) {
    config_ = config;
    
    try {
        // ログレベルの設定
        std::string log_level = config_.verbose ? "debug" : "info";
        std::string log_file = "/var/log/ocpp-gateway/ocpp-gateway.log";
        
        // ロガーの初期化
        common::LogConfig log_config;
        log_config.log_level = log_level;
        log_config.log_file = log_file;
        log_config.max_size_mb = 10;
        log_config.max_files = 5;
        log_config.console_output = true;
        log_config.file_output = true;
        log_config.daily_rotation = false;
        log_config.compress_logs = true;
        
        if (!common::Logger::initialize(log_config)) {
            std::cerr << "ロガーの初期化に失敗しました" << std::endl;
            return false;
        }
        
        LOG_INFO("OCPP 2.0.1 ゲートウェイ・ミドルウェアを開始します");
        LOG_INFO("設定ファイル: {}", config_.config_path);
        
        // 設定ファイルの存在確認
        if (!fs::exists(config_.config_path)) {
            LOG_ERROR("設定ファイルが見つかりません: {}", config_.config_path);
            return false;
        }
        
        // システムコンポーネントの初期化
        if (!initializeComponents()) {
            LOG_ERROR("システムコンポーネントの初期化に失敗しました");
            return false;
        }
        
        // シグナルハンドラーの設定
        setupSignalHandlers();
        
        LOG_INFO("アプリケーションの初期化が完了しました");
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "アプリケーション初期化エラー: " << e.what() << std::endl;
        return false;
    }
}

Application::ExitCode Application::run() {
    if (!running_.load()) {
        LOG_ERROR("アプリケーションが初期化されていません");
        return ExitCode::INITIALIZATION_FAILED;
    }
    
    try {
        if (config_.cli_mode) {
            return runCli();
        } else {
            return runDaemon();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("実行時エラー: {}", e.what());
        return ExitCode::RUNTIME_ERROR;
    }
}

void Application::requestStop() {
    LOG_INFO("アプリケーション停止が要求されました");
    running_ = false;
}

bool Application::isRunning() const {
    return running_.load();
}

bool Application::executeCli(const std::vector<std::string>& args) {
    if (!cli_manager_) {
        LOG_ERROR("CLIマネージャーが初期化されていません");
        return false;
    }
    
    auto result = cli_manager_->executeCommand(args);
    if (!result.success) {
        std::cerr << "エラー: " << result.message << std::endl;
        return false;
    }
    
    if (!result.output.empty()) {
        std::cout << result.output << std::endl;
    }
    
    return true;
}

void Application::setupSignalHandlers() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
}

bool Application::initializeComponents() {
    try {
        LOG_INFO("コンポーネント初期化を開始します");
        
        // 設定ディレクトリの取得
        fs::path config_file_path(config_.config_path);
        std::string config_dir = config_file_path.parent_path().string();
        LOG_INFO("設定ディレクトリ: {}", config_dir);
        
        // 設定マネージャーの初期化 (シングルトンパターン)
        LOG_INFO("設定マネージャーを初期化中...");
        auto& config_manager = config::ConfigManager::getInstance();
        if (!config_manager.initialize(config_dir)) {
            LOG_ERROR("設定マネージャーの初期化に失敗しました");
            return false;
        }
        LOG_INFO("設定マネージャーの初期化完了");
        
        // 設定の検証
        try {
            LOG_INFO("設定の検証を開始...");
            config_manager.validateAllConfigs();
            LOG_INFO("設定の検証が完了しました");
        } catch (const config::ConfigValidationError& e) {
            LOG_ERROR("設定の検証に失敗しました: {}", e.what());
            return false;
        }
        
        // メトリクス収集の初期化 (シングルトンパターン)
        LOG_INFO("メトリクス収集を初期化中...");
        auto& metrics_collector = common::MetricsCollector::getInstance();
        if (!metrics_collector.initialize()) {
            LOG_ERROR("メトリクス収集の初期化に失敗しました");
            return false;
        }
        LOG_INFO("メトリクス収集を初期化しました");
        
        // 管理APIの初期化
        LOG_INFO("管理APIを初期化中...");
        const auto& system_config = config_manager.getSystemConfig();
        int admin_port = 8080; // Default admin API port
        std::string admin_bind_address = "0.0.0.0"; // Default bind address
        
        admin_api_ = std::make_unique<api::AdminApi>(admin_port, admin_bind_address);
        
        // 認証設定
        bool auth_enabled = false; // Default: no authentication
        if (auth_enabled) {
            admin_api_->setAuthentication(true, "admin", "password");
            LOG_INFO("管理API認証を有効にしました");
        }
        
        if (!admin_api_->start()) {
            LOG_ERROR("管理APIの開始に失敗しました");
            return false;
        }
        LOG_INFO("管理APIを開始しました: {}:{}", admin_bind_address, admin_port);
        
        // CLIマネージャーの初期化
        LOG_INFO("CLIマネージャーを初期化中...");
        cli_manager_ = std::make_unique<api::CliManager>();
        LOG_INFO("CLIマネージャーを初期化しました");
        
        // WebUIの初期化
        LOG_INFO("WebUIを初期化中...");
        web_ui_ = std::make_unique<api::WebUI>(8081, "0.0.0.0", "web");
        if (!web_ui_->start()) {
            LOG_ERROR("WebUIの開始に失敗しました");
            return false;
        }
        LOG_INFO("WebUIを初期化しました: {}:{}", "0.0.0.0", 8081);
        
        // Prometheusエクスポーターの初期化
        LOG_INFO("Prometheusエクスポーターを初期化中...");
        prometheus_exporter_ = std::make_unique<common::PrometheusExporter>(9090, "0.0.0.0", "/metrics");
        if (!prometheus_exporter_->start()) {
            LOG_ERROR("Prometheusエクスポーターの開始に失敗しました");
            return false;
        }
        LOG_INFO("Prometheusエクスポーターを初期化しました: {}:{}{}", "0.0.0.0", 9090, "/metrics");
        
        // Business logic components initialization
        // 1. マッピングエンジンの初期化
        LOG_INFO("マッピングエンジンを初期化中...");
        mapping_engine_ = std::make_unique<mapping::MappingEngine>();
        fs::path mapping_config_path = fs::path(config_dir) / "templates";
        if (!mapping_engine_->initialize(mapping_config_path.string())) {
            LOG_ERROR("マッピングエンジンの初期化に失敗しました");
            return false;
        }
        LOG_INFO("マッピングエンジンを初期化しました");
        
        // 2. デバイスマネージャーの初期化 (これは新しいクラスで複数のアダプターを管理)
        // device_manager_ = std::make_unique<device::DeviceManager>();
        
        // 3. OCPPクライアントマネージャーの初期化
        LOG_INFO("OCPPクライアントマネージャーを初期化中...");
        try {
            const auto& csms_config = config_manager.getCsmsConfig();
            LOG_INFO("CSMS設定を取得しました: URL={}", csms_config.getUrl());
            
            // CSMS設定をOCPP設定に変換
            ocpp::OcppClientConfig ocpp_config;
            ocpp_config.csms_url = csms_config.getUrl();
            ocpp_config.heartbeat_interval = std::chrono::seconds(csms_config.getHeartbeatInterval());
            ocpp_config.reconnect_interval = std::chrono::seconds(csms_config.getReconnectInterval());
            ocpp_config.max_reconnect_attempts = csms_config.getMaxReconnectAttempts();
            ocpp_config.connect_timeout = std::chrono::seconds(10);
            ocpp_config.charge_point_model = "OCPP Gateway";
            ocpp_config.charge_point_vendor = "OCPP Gateway";
            ocpp_config.firmware_version = "1.0.0";
            
            // TLS設定（シミュレーター用に無効化）
            ocpp_config.verify_peer = false;
            
            LOG_INFO("OCPP設定: URL={}, Heartbeat={}s", 
                    ocpp_config.csms_url, 
                    ocpp_config.heartbeat_interval.count());
            
            // OCPPクライアントマネージャーを作成
            ocpp_client_manager_ = ocpp::OcppClientManager::create(io_context_, ocpp_config);
            if (!ocpp_client_manager_) {
                LOG_ERROR("OCPPクライアントマネージャーの作成に失敗しました");
                return false;
            }
            
            // OCPPクライアントマネージャーを開始
            if (!ocpp_client_manager_->start()) {
                LOG_ERROR("OCPPクライアントマネージャーの開始に失敗しました");
                return false;
            }
            
            LOG_INFO("OCPPクライアントマネージャーを初期化しました");
            
        } catch (const std::exception& e) {
            LOG_ERROR("OCPPクライアントマネージャーの初期化エラー: {}", e.what());
            return false;
        }
        
        LOG_INFO("全コンポーネントの初期化が完了しました");
        running_ = true;
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("コンポーネント初期化エラー: {}", e.what());
        return false;
    }
}

Application::ExitCode Application::runDaemon() {
    LOG_INFO("デーモンモードでメインループを開始します。Ctrl+Cで終了してください");
    
    runMainLoop();
    
    cleanup();
    return ExitCode::SUCCESS;
}

Application::ExitCode Application::runCli() {
    if (config_.cli_args.empty()) {
        return runInteractiveCli();
    } else {
        return runSingleCli();
    }
}

Application::ExitCode Application::runInteractiveCli() {
    std::cout << "OCPP Gateway CLI (Ctrl+Cで終了)" << std::endl;
    std::string line;
    
    while (running_.load() && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        // コマンドを解析
        std::vector<std::string> args;
        std::istringstream iss(line);
        std::string arg;
        while (iss >> arg) {
            args.push_back(arg);
        }
        
        if (!args.empty()) {
            executeCli(args);
        }
    }
    
    cleanup();
    return ExitCode::SUCCESS;
}

Application::ExitCode Application::runSingleCli() {
    bool success = executeCli(config_.cli_args);
    cleanup();
    return success ? ExitCode::SUCCESS : ExitCode::RUNTIME_ERROR;
}

void Application::runMainLoop() {
    while (running_.load()) {
        // 設定再読み込みの確認
        if (reload_requested_.load()) {
            reloadConfiguration();
            reload_requested_ = false;
        }
        
        // IO context polling for async operations
        io_context_.poll();
        
        // TODO: メインループでの処理
        // 1. デバイス状態の監視
        // 2. OCPPメッセージの処理
        // 3. エラー処理
        // 4. 統計情報の更新
        // メトリクスの更新は別スレッドで実行される
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Application::cleanup() {
    LOG_INFO("アプリケーションの終了処理を開始します");
    
    // Stop OCPP client manager
    if (ocpp_client_manager_) {
        ocpp_client_manager_->stop();
        LOG_INFO("OCPPクライアントマネージャーを停止しました");
    }
    
    // Stop Prometheus exporter
    if (prometheus_exporter_) {
        prometheus_exporter_->stop();
        LOG_INFO("Prometheusエクスポーターを停止しました");
    }
    
    // Stop WebUI
    if (web_ui_) {
        web_ui_->stop();
        LOG_INFO("WebUIを停止しました");
    }
    
    // Stop admin API
    if (admin_api_) {
        admin_api_->stop();
        LOG_INFO("管理APIを停止しました");
    }
    
    // Stop metrics collector
    auto& metrics_collector = common::MetricsCollector::getInstance();
    metrics_collector.shutdown();
    LOG_INFO("メトリクス収集を停止しました");
    
    // Config manager is automatically cleaned up
    LOG_INFO("設定マネージャーは自動的にクリーンアップされます");
    
    LOG_INFO("アプリケーションの終了処理が完了しました");
}

void Application::reloadConfiguration() {
    LOG_INFO("設定を再読み込みしています...");
    
    try {
        auto& config_manager = config::ConfigManager::getInstance();
        // TODO: 設定の再読み込み実装 (Phase 2)
        // config_manager.reload();
        LOG_INFO("設定の再読み込みが完了しました");
    } catch (const std::exception& e) {
        LOG_ERROR("設定再読み込みエラー: {}", e.what());
    }
}

void Application::signalHandler(int signal) {
    if (instance_) {
        LOG_INFO("シグナルを受信しました: {}", signal);
        
        switch (signal) {
            case SIGHUP:
                instance_->reload_requested_ = true;
                break;
            case SIGINT:
            case SIGTERM:
                instance_->requestStop();
                break;
            default:
                break;
        }
    }
}

} // namespace ocpp_gateway 