#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include "ocpp_gateway/common/application.h"

namespace po = boost::program_options;

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

        // アプリケーション設定の構築
        ocpp_gateway::Application::Config app_config;
        app_config.config_path = vm["config"].as<std::string>();
        app_config.verbose = vm.count("verbose") > 0;
        app_config.daemon_mode = vm.count("daemon") > 0;
        app_config.cli_mode = vm.count("cli") > 0;
        
        if (vm.count("cli-command")) {
            app_config.cli_args = vm["cli-command"].as<std::vector<std::string>>();
        }

        // アプリケーションの作成と初期化
        ocpp_gateway::Application app;
        if (!app.initialize(app_config)) {
            std::cerr << "アプリケーションの初期化に失敗しました" << std::endl;
            return static_cast<int>(ocpp_gateway::Application::ExitCode::INITIALIZATION_FAILED);
        }

        // アプリケーションの実行
        auto exit_code = app.run();
        return static_cast<int>(exit_code);
        
    } catch (const std::exception& e) {
        std::cerr << "エラー: " << e.what() << std::endl;
        return static_cast<int>(ocpp_gateway::Application::ExitCode::RUNTIME_ERROR);
    }
}