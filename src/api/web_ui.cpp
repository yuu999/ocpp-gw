#include "ocpp_gateway/api/web_ui.h"
#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/config_manager.h"
#include "ocpp_gateway/common/metrics_collector.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>
#include <json/json.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace fs = boost::filesystem;

namespace ocpp_gateway {
namespace api {

WebUI::WebUI(int port, const std::string& bind_address, const std::string& document_root)
    : port_(port), bind_address_(bind_address), document_root_(document_root),
      running_(false), auth_enabled_(false) {
    
    // MIMEタイプマップの初期化
    mime_types_[".html"] = "text/html";
    mime_types_[".htm"] = "text/html";
    mime_types_[".css"] = "text/css";
    mime_types_[".js"] = "application/javascript";
    mime_types_[".json"] = "application/json";
    mime_types_[".png"] = "image/png";
    mime_types_[".jpg"] = "image/jpeg";
    mime_types_[".jpeg"] = "image/jpeg";
    mime_types_[".gif"] = "image/gif";
    mime_types_[".svg"] = "image/svg+xml";
    mime_types_[".ico"] = "image/x-icon";
    mime_types_[".woff"] = "font/woff";
    mime_types_[".woff2"] = "font/woff2";
    mime_types_[".ttf"] = "font/ttf";
}

WebUI::~WebUI() {
    stop();
}

bool WebUI::start() {
    if (running_.load()) {
        LOG_WARN("WebUIサーバーは既に実行中です");
        return true;
    }

    try {
        running_ = true;
        server_thread_ = std::thread(&WebUI::runServer, this);
        
        // サーバー開始まで少し待機
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        LOG_INFO("WebUIサーバーを開始しました: {}:{}", bind_address_, port_);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("WebUIサーバーの開始に失敗しました: {}", e.what());
        running_ = false;
        return false;
    }
}

void WebUI::stop() {
    if (!running_.load()) {
        return;
    }

    running_ = false;
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    LOG_INFO("WebUIサーバーを停止しました");
}

bool WebUI::isRunning() const {
    return running_.load();
}

void WebUI::setDocumentRoot(const std::string& root_path) {
    document_root_ = root_path;
}

void WebUI::setAuthentication(bool enabled, const std::string& username, const std::string& password) {
    auth_enabled_ = enabled;
    auth_username_ = username;
    auth_password_ = password;
    
    if (enabled) {
        LOG_INFO("WebUI認証を有効にしました（ユーザー: {}）", username);
    } else {
        LOG_INFO("WebUI認証を無効にしました");
    }
}

void WebUI::runServer() {
    try {
        auto const address = net::ip::make_address(bind_address_);
        auto const port = static_cast<unsigned short>(port_);

        net::io_context ioc{1};
        beast::tcp_acceptor acceptor{ioc, {address, port}};

        while (running_.load()) {
            beast::tcp_stream stream{ioc};
            
            // 接続を受け入れ
            try {
                acceptor.accept(stream.socket());
            } catch (const std::exception& e) {
                if (running_.load()) {
                    LOG_ERROR("WebUI接続受け入れエラー: {}", e.what());
                }
                continue;
            }

            // リクエストを読み取り
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            
            try {
                http::read(stream, buffer, req);
            } catch (const std::exception& e) {
                LOG_ERROR("WebUIリクエスト読み取りエラー: {}", e.what());
                continue;
            }

            // ヘッダーを抽出
            std::map<std::string, std::string> headers;
            for (auto const& field : req) {
                headers[std::string(field.name_string())] = std::string(field.value());
            }

            // リクエストを処理
            std::string response_body = handleRequest(
                std::string(req.target()),
                std::string(req.method_string()),
                req.body(),
                headers
            );

            // レスポンスを作成
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "OCPP Gateway WebUI Server");
            res.set(http::field::content_type, "text/html; charset=utf-8");
            res.set(http::field::access_control_allow_origin, "*");
            res.body() = response_body;
            res.prepare_payload();

            // レスポンスを送信
            try {
                http::write(stream, res);
            } catch (const std::exception& e) {
                LOG_ERROR("WebUIレスポンス送信エラー: {}", e.what());
            }

            // 接続を閉じる
            beast::error_code ec;
            stream.socket().shutdown(beast::tcp_socket::shutdown_send, ec);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("WebUIサーバーエラー: {}", e.what());
    }
}

std::string WebUI::handleRequest(const std::string& target, const std::string& method,
                                const std::string& body, 
                                const std::map<std::string, std::string>& headers) {
    try {
        // 認証チェック
        if (auth_enabled_ && !authenticate(headers)) {
            return createErrorResponse(401, "認証が必要です");
        }

        // ルートパスの場合はダッシュボードを表示
        if (target == "/" || target == "/index.html") {
            return generateDashboard();
        }
        
        // 各ページのルーティング
        if (target == "/dashboard" || target == "/dashboard.html") {
            return generateDashboard();
        } else if (target == "/devices" || target == "/devices.html") {
            return generateDevicePage();
        } else if (target == "/config" || target == "/config.html") {
            return generateConfigPage();
        } else if (target == "/logs" || target == "/logs.html") {
            return generateLogPage();
        }
        
        // 静的ファイルの配信
        std::string file_path = document_root_ + target;
        if (fs::exists(file_path) && fs::is_regular_file(file_path)) {
            return serveStaticFile(file_path);
        }
        
        // 404エラー
        return createErrorResponse(404, "ページが見つかりません");
    } catch (const std::exception& e) {
        LOG_ERROR("WebUIリクエスト処理エラー: {}", e.what());
        return createErrorResponse(500, "内部サーバーエラー");
    }
}

std::string WebUI::serveStaticFile(const std::string& file_path) {
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return createErrorResponse(404, "ファイルが見つかりません");
        }
        
        std::ostringstream content;
        content << file.rdbuf();
        
        std::string mime_type = getMimeType(file_path);
        return createResponse(content.str(), mime_type);
    } catch (const std::exception& e) {
        LOG_ERROR("静的ファイル配信エラー [{}]: {}", file_path, e.what());
        return createErrorResponse(500, "ファイル読み取りエラー");
    }
}

std::string WebUI::getMimeType(const std::string& file_path) {
    fs::path path(file_path);
    std::string extension = path.extension().string();
    
    auto it = mime_types_.find(extension);
    if (it != mime_types_.end()) {
        return it->second;
    }
    
    return "application/octet-stream";
}

bool WebUI::authenticate(const std::map<std::string, std::string>& headers) {
    auto auth_it = headers.find("Authorization");
    if (auth_it == headers.end()) {
        return false;
    }

    std::string auth_header = auth_it->second;
    if (auth_header.substr(0, 6) != "Basic ") {
        return false;
    }

    // 簡易的な認証チェック（実際にはBase64デコードを実装）
    std::string credentials = auth_header.substr(6);
    std::string expected = auth_username_ + ":" + auth_password_;
    
    return credentials == expected;
}

std::string WebUI::createErrorResponse(int status_code, const std::string& message) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <title>エラー " << status_code << " - OCPP Gateway</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: Arial, sans-serif; margin: 40px; }\n";
    html << "    .error { color: #d32f2f; }\n";
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <h1 class=\"error\">エラー " << status_code << "</h1>\n";
    html << "  <p>" << message << "</p>\n";
    html << "  <hr>\n";
    html << "  <p><a href=\"/\">ダッシュボードに戻る</a></p>\n";
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}

std::string WebUI::createResponse(const std::string& content, const std::string& content_type) {
    // 簡易実装：HTTPヘッダーは runServer() で設定される
    return content;
}

std::string WebUI::generateDefaultPage() {
    return generateDashboard();
}

std::string WebUI::generateDashboard() {
    try {
        auto& config_manager = config::ConfigManager::getInstance();
        auto& metrics_collector = common::MetricsCollector::getInstance();
        
        std::ostringstream html;
        html << "<!DOCTYPE html>\n";
        html << "<html>\n";
        html << "<head>\n";
        html << "  <meta charset=\"UTF-8\">\n";
        html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        html << "  <title>OCPP Gateway - ダッシュボード</title>\n";
        html << "  <style>\n";
        html << generateCSS();
        html << "  </style>\n";
        html << "</head>\n";
        html << "<body>\n";
        html << generateNavigation();
        html << "  <div class=\"container\">\n";
        html << "    <h1>OCPP 2.0.1 ゲートウェイ・ミドルウェア</h1>\n";
        
        // システム状態カード
        html << "    <div class=\"card\">\n";
        html << "      <h2>システム状態</h2>\n";
        html << "      <div class=\"status-grid\">\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">状態:</span>\n";
        html << "          <span class=\"status-value status-active\">稼働中</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">稼働時間:</span>\n";
        html << "          <span class=\"status-value\">不明</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">バージョン:</span>\n";
        html << "          <span class=\"status-value\">1.0.0</span>\n";
        html << "        </div>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        // デバイス状態カード
        const auto& device_configs = config_manager.getDeviceConfigs();
        html << "    <div class=\"card\">\n";
        html << "      <h2>デバイス状態</h2>\n";
        html << "      <div class=\"status-grid\">\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">総デバイス数:</span>\n";
        html << "          <span class=\"status-value\">" << device_configs.getDevices().size() << "</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">アクティブ:</span>\n";
        html << "          <span class=\"status-value status-active\">0</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">エラー:</span>\n";
        html << "          <span class=\"status-value status-error\">0</span>\n";
        html << "        </div>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        // 最近のイベント
        html << "    <div class=\"card\">\n";
        html << "      <h2>最近のイベント</h2>\n";
        html << "      <div class=\"event-list\">\n";
        html << "        <div class=\"event-item\">\n";
        html << "          <span class=\"event-time\">2024-12-20 17:00:00</span>\n";
        html << "          <span class=\"event-message\">システム開始</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"event-item\">\n";
        html << "          <span class=\"event-time\">2024-12-20 17:00:01</span>\n";
        html << "          <span class=\"event-message\">設定読み込み完了</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"event-item\">\n";
        html << "          <span class=\"event-time\">2024-12-20 17:00:02</span>\n";
        html << "          <span class=\"event-message\">管理APIサーバー開始</span>\n";
        html << "        </div>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        html << "  </div>\n";
        html << generateJavaScript();
        html << "</body>\n";
        html << "</html>\n";
        
        return html.str();
    } catch (const std::exception& e) {
        LOG_ERROR("ダッシュボード生成エラー: {}", e.what());
        return createErrorResponse(500, "ダッシュボードの生成に失敗しました");
    }
}

std::string WebUI::generateDevicePage() {
    try {
        auto& config_manager = config::ConfigManager::getInstance();
        const auto& device_configs = config_manager.getDeviceConfigs();
        
        std::ostringstream html;
        html << "<!DOCTYPE html>\n";
        html << "<html>\n";
        html << "<head>\n";
        html << "  <meta charset=\"UTF-8\">\n";
        html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        html << "  <title>OCPP Gateway - デバイス管理</title>\n";
        html << "  <style>\n";
        html << generateCSS();
        html << "  </style>\n";
        html << "</head>\n";
        html << "<body>\n";
        html << generateNavigation();
        html << "  <div class=\"container\">\n";
        html << "    <h1>デバイス管理</h1>\n";
        
        // デバイス一覧テーブル
        html << "    <div class=\"card\">\n";
        html << "      <h2>登録デバイス一覧</h2>\n";
        html << "      <table class=\"device-table\">\n";
        html << "        <thead>\n";
        html << "          <tr>\n";
        html << "            <th>ID</th>\n";
        html << "            <th>名前</th>\n";
        html << "            <th>プロトコル</th>\n";
        html << "            <th>テンプレート</th>\n";
        html << "            <th>状態</th>\n";
        html << "            <th>操作</th>\n";
        html << "          </tr>\n";
        html << "        </thead>\n";
        html << "        <tbody>\n";
        
        for (const auto& device : device_configs.getDevices()) {
            html << "          <tr>\n";
            html << "            <td>" << device.getId() << "</td>\n";
            html << "            <td>" << device.getName() << "</td>\n";
            html << "            <td>" << device.getProtocol() << "</td>\n";
            html << "            <td>" << device.getTemplateName() << "</td>\n";
            html << "            <td><span class=\"status-" << (device.isEnabled() ? "active" : "inactive") << "\">";
            html << (device.isEnabled() ? "有効" : "無効") << "</span></td>\n";
            html << "            <td>\n";
            html << "              <button class=\"btn btn-info\" onclick=\"showDeviceDetail('" << device.getId() << "')\">詳細</button>\n";
            html << "              <button class=\"btn btn-warning\" onclick=\"editDevice('" << device.getId() << "')\">編集</button>\n";
            html << "            </td>\n";
            html << "          </tr>\n";
        }
        
        if (device_configs.getDevices().empty()) {
            html << "          <tr>\n";
            html << "            <td colspan=\"6\" class=\"text-center\">登録されているデバイスはありません</td>\n";
            html << "          </tr>\n";
        }
        
        html << "        </tbody>\n";
        html << "      </table>\n";
        html << "    </div>\n";
        
        html << "  </div>\n";
        html << generateJavaScript();
        html << "</body>\n";
        html << "</html>\n";
        
        return html.str();
    } catch (const std::exception& e) {
        LOG_ERROR("デバイスページ生成エラー: {}", e.what());
        return createErrorResponse(500, "デバイスページの生成に失敗しました");
    }
}

std::string WebUI::generateConfigPage() {
    try {
        auto& config_manager = config::ConfigManager::getInstance();
        
        std::ostringstream html;
        html << "<!DOCTYPE html>\n";
        html << "<html>\n";
        html << "<head>\n";
        html << "  <meta charset=\"UTF-8\">\n";
        html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        html << "  <title>OCPP Gateway - 設定管理</title>\n";
        html << "  <style>\n";
        html << generateCSS();
        html << "  </style>\n";
        html << "</head>\n";
        html << "<body>\n";
        html << generateNavigation();
        html << "  <div class=\"container\">\n";
        html << "    <h1>設定管理</h1>\n";
        
        // システム設定
        const auto& system_config = config_manager.getSystemConfig();
        html << "    <div class=\"card\">\n";
        html << "      <h2>システム設定</h2>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>ログレベル:</label>\n";
        html << "        <span>" << system_config.getLogLevel() << "</span>\n";
        html << "      </div>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>最大充電ポイント数:</label>\n";
        html << "        <span>" << system_config.getMaxChargePoints() << "</span>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        // CSMS設定
        const auto& csms_config = config_manager.getCsmsConfig();
        html << "    <div class=\"card\">\n";
        html << "      <h2>CSMS設定</h2>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>URL:</label>\n";
        html << "        <span>" << csms_config.getUrl() << "</span>\n";
        html << "      </div>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>再接続間隔:</label>\n";
        html << "        <span>" << csms_config.getReconnectInterval() << " 秒</span>\n";
        html << "      </div>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>最大再接続回数:</label>\n";
        html << "        <span>" << csms_config.getMaxReconnectAttempts() << "</span>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        // 操作ボタン
        html << "    <div class=\"card\">\n";
        html << "      <h2>設定操作</h2>\n";
        html << "      <div class=\"button-group\">\n";
        html << "        <button class=\"btn btn-primary\" onclick=\"reloadConfig()\">設定再読み込み</button>\n";
        html << "        <button class=\"btn btn-info\" onclick=\"validateConfig()\">設定検証</button>\n";
        html << "        <button class=\"btn btn-warning\" onclick=\"backupConfig()\">設定バックアップ</button>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        html << "  </div>\n";
        html << generateJavaScript();
        html << "</body>\n";
        html << "</html>\n";
        
        return html.str();
    } catch (const std::exception& e) {
        LOG_ERROR("設定ページ生成エラー: {}", e.what());
        return createErrorResponse(500, "設定ページの生成に失敗しました");
    }
}

std::string WebUI::generateLogPage() {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "  <title>OCPP Gateway - ログ表示</title>\n";
    html << "  <style>\n";
    html << generateCSS();
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << generateNavigation();
    html << "  <div class=\"container\">\n";
    html << "    <h1>ログ表示</h1>\n";
    
    html << "    <div class=\"card\">\n";
    html << "      <h2>最新ログ</h2>\n";
    html << "      <div class=\"log-container\">\n";
    html << "        <div class=\"log-entry log-info\">\n";
    html << "          <span class=\"log-time\">2024-12-20 17:00:00</span>\n";
    html << "          <span class=\"log-level\">INFO</span>\n";
    html << "          <span class=\"log-message\">WebUIサーバーを開始しました</span>\n";
    html << "        </div>\n";
    html << "        <div class=\"log-entry log-info\">\n";
    html << "          <span class=\"log-time\">2024-12-20 17:00:01</span>\n";
    html << "          <span class=\"log-level\">INFO</span>\n";
    html << "          <span class=\"log-message\">管理APIサーバーを開始しました</span>\n";
    html << "        </div>\n";
    html << "        <div class=\"log-entry log-info\">\n";
    html << "          <span class=\"log-time\">2024-12-20 17:00:02</span>\n";
    html << "          <span class=\"log-level\">INFO</span>\n";
    html << "          <span class=\"log-message\">メトリクス収集を初期化しました</span>\n";
    html << "        </div>\n";
    html << "      </div>\n";
    html << "    </div>\n";
    
    html << "  </div>\n";
    html << generateJavaScript();
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}

std::string WebUI::generateNavigation() {
    std::ostringstream nav;
    nav << "  <nav class=\"navbar\">\n";
    nav << "    <div class=\"nav-brand\">\n";
    nav << "      <h2>OCPP Gateway</h2>\n";
    nav << "    </div>\n";
    nav << "    <div class=\"nav-links\">\n";
    nav << "      <a href=\"/dashboard\">ダッシュボード</a>\n";
    nav << "      <a href=\"/devices\">デバイス</a>\n";
    nav << "      <a href=\"/config\">設定</a>\n";
    nav << "      <a href=\"/logs\">ログ</a>\n";
    nav << "    </div>\n";
    nav << "  </nav>\n";
    return nav.str();
}

std::string WebUI::generateCSS() {
    std::ostringstream css;
    css << "    * { margin: 0; padding: 0; box-sizing: border-box; }\n";
    css << "    body { font-family: 'Arial', sans-serif; background-color: #f5f5f5; }\n";
    css << "    .navbar { background-color: #2196F3; color: white; padding: 1rem; display: flex; justify-content: space-between; align-items: center; }\n";
    css << "    .nav-brand h2 { margin: 0; }\n";
    css << "    .nav-links a { color: white; text-decoration: none; margin-left: 2rem; padding: 0.5rem 1rem; border-radius: 4px; transition: background-color 0.3s; }\n";
    css << "    .nav-links a:hover { background-color: rgba(255,255,255,0.1); }\n";
    css << "    .container { max-width: 1200px; margin: 2rem auto; padding: 0 1rem; }\n";
    css << "    .card { background: white; border-radius: 8px; padding: 1.5rem; margin-bottom: 2rem; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    css << "    .card h2 { margin-bottom: 1rem; color: #333; }\n";
    css << "    .status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 1rem; }\n";
    css << "    .status-item { display: flex; justify-content: space-between; padding: 0.5rem; border-bottom: 1px solid #eee; }\n";
    css << "    .status-label { font-weight: bold; }\n";
    css << "    .status-active { color: #4CAF50; }\n";
    css << "    .status-inactive { color: #757575; }\n";
    css << "    .status-error { color: #F44336; }\n";
    css << "    .device-table { width: 100%; border-collapse: collapse; }\n";
    css << "    .device-table th, .device-table td { padding: 0.75rem; text-align: left; border-bottom: 1px solid #ddd; }\n";
    css << "    .device-table th { background-color: #f8f9fa; font-weight: bold; }\n";
    css << "    .btn { padding: 0.5rem 1rem; border: none; border-radius: 4px; cursor: pointer; margin-right: 0.5rem; transition: opacity 0.3s; }\n";
    css << "    .btn:hover { opacity: 0.8; }\n";
    css << "    .btn-primary { background-color: #2196F3; color: white; }\n";
    css << "    .btn-info { background-color: #00BCD4; color: white; }\n";
    css << "    .btn-warning { background-color: #FF9800; color: white; }\n";
    css << "    .config-item { display: flex; justify-content: space-between; padding: 0.5rem 0; border-bottom: 1px solid #eee; }\n";
    css << "    .config-item label { font-weight: bold; }\n";
    css << "    .button-group { display: flex; gap: 1rem; flex-wrap: wrap; }\n";
    css << "    .event-list { max-height: 300px; overflow-y: auto; }\n";
    css << "    .event-item { display: flex; justify-content: space-between; padding: 0.5rem 0; border-bottom: 1px solid #eee; }\n";
    css << "    .event-time { color: #666; font-size: 0.9rem; }\n";
    css << "    .log-container { max-height: 500px; overflow-y: auto; background-color: #1e1e1e; color: #fff; padding: 1rem; border-radius: 4px; }\n";
    css << "    .log-entry { display: flex; gap: 1rem; padding: 0.25rem 0; font-family: 'Courier New', monospace; font-size: 0.9rem; }\n";
    css << "    .log-time { color: #888; }\n";
    css << "    .log-level { font-weight: bold; min-width: 60px; }\n";
    css << "    .log-info .log-level { color: #4CAF50; }\n";
    css << "    .log-warning .log-level { color: #FF9800; }\n";
    css << "    .log-error .log-level { color: #F44336; }\n";
    css << "    .text-center { text-align: center; }\n";
    css << "    @media (max-width: 768px) {\n";
    css << "      .navbar { flex-direction: column; gap: 1rem; }\n";
    css << "      .nav-links { display: flex; flex-wrap: wrap; gap: 0.5rem; }\n";
    css << "      .nav-links a { margin-left: 0; }\n";
    css << "      .status-grid { grid-template-columns: 1fr; }\n";
    css << "      .device-table { font-size: 0.9rem; }\n";
    css << "      .button-group { flex-direction: column; }\n";
    css << "    }\n";
    return css.str();
}

std::string WebUI::generateJavaScript() {
    std::ostringstream js;
    js << "  <script>\n";
    js << "    function showDeviceDetail(deviceId) {\n";
    js << "      alert('デバイス詳細: ' + deviceId + ' (未実装)');\n";
    js << "    }\n";
    js << "    function editDevice(deviceId) {\n";
    js << "      alert('デバイス編集: ' + deviceId + ' (未実装)');\n";
    js << "    }\n";
    js << "    function reloadConfig() {\n";
    js << "      if (confirm('設定を再読み込みしますか？')) {\n";
    js << "        alert('設定再読み込み (未実装)');\n";
    js << "      }\n";
    js << "    }\n";
    js << "    function validateConfig() {\n";
    js << "      alert('設定検証 (未実装)');\n";
    js << "    }\n";
    js << "    function backupConfig() {\n";
    js << "      alert('設定バックアップ (未実装)');\n";
    js << "    }\n";
    js << "    // 自動更新（30秒間隔）\n";
    js << "    setInterval(function() {\n";
    js << "      if (window.location.pathname === '/dashboard' || window.location.pathname === '/') {\n";
    js << "        // ダッシュボードのみ自動更新\n";
    js << "        console.log('ダッシュボード自動更新 (未実装)');\n";
    js << "      }\n";
    js << "    }, 30000);\n";
    js << "  </script>\n";
    return js.str();
}

} // namespace api
} // namespace ocpp_gateway 