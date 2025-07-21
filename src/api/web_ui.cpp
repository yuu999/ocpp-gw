#include "ocpp_gateway/api/web_ui.h"
#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/config_manager.h"
#include "ocpp_gateway/common/metrics_collector.h"
#include "ocpp_gateway/common/language_manager.h"
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
    
    // Initialize language manager
    common::LanguageManager::getInstance().initialize("en", "resources/lang");
}

WebUI::~WebUI() {
    stop();
}

bool WebUI::start() {
    if (running_.load()) {
        LOG_WARN(translate("webui_already_running", "WebUIサーバーは既に実行中です"));
        return true;
    }

    try {
        running_ = true;
        server_thread_ = std::thread(&WebUI::runServer, this);
        
        // サーバー開始まで少し待機
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        LOG_INFO(translate("webui_started", "WebUIサーバーを開始しました: {}:{}"), bind_address_, port_);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(translate("webui_start_failed", "WebUIサーバーの開始に失敗しました: {}"), e.what());
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
    
    LOG_INFO(translate("webui_stopped", "WebUIサーバーを停止しました"));
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
        LOG_INFO(translate("webui_auth_enabled", "WebUI認証を有効にしました（ユーザー: {}）"), username);
    } else {
        LOG_INFO(translate("webui_auth_disabled", "WebUI認証を無効にしました"));
    }
}

bool WebUI::setLanguage(const std::string& language) {
    return common::LanguageManager::getInstance().setLanguage(language);
}

std::string WebUI::getCurrentLanguage() const {
    return common::LanguageManager::getInstance().getCurrentLanguage();
}

std::vector<std::string> WebUI::getAvailableLanguages() const {
    return common::LanguageManager::getInstance().getAvailableLanguages();
}

std::string WebUI::translate(const std::string& key, const std::string& default_value) const {
    return common::LanguageManager::getInstance().translate(key, default_value);
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
                    LOG_ERROR(translate("webui_accept_error", "WebUI接続受け入れエラー: {}"), e.what());
                }
                continue;
            }

            // リクエストを読み取り
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            
            try {
                http::read(stream, buffer, req);
            } catch (const std::exception& e) {
                LOG_ERROR(translate("webui_request_error", "WebUIリクエスト読み取りエラー: {}"), e.what());
                continue;
            }

            // ヘッダーを抽出
            std::map<std::string, std::string> headers;
            for (auto const& field : req) {
                headers[std::string(field.name_string())] = std::string(field.value());
            }
            
            // クエリパラメータを抽出
            std::map<std::string, std::string> query_params;
            std::string target = std::string(req.target());
            size_t query_pos = target.find('?');
            if (query_pos != std::string::npos) {
                std::string query_string = target.substr(query_pos + 1);
                target = target.substr(0, query_pos);
                
                size_t start = 0;
                size_t end = 0;
                while ((end = query_string.find('&', start)) != std::string::npos) {
                    std::string param = query_string.substr(start, end - start);
                    size_t eq_pos = param.find('=');
                    if (eq_pos != std::string::npos) {
                        query_params[param.substr(0, eq_pos)] = param.substr(eq_pos + 1);
                    }
                    start = end + 1;
                }
                
                std::string param = query_string.substr(start);
                size_t eq_pos = param.find('=');
                if (eq_pos != std::string::npos) {
                    query_params[param.substr(0, eq_pos)] = param.substr(eq_pos + 1);
                }
            }

            // 言語切り替えの処理
            auto lang_it = query_params.find("lang");
            if (lang_it != query_params.end()) {
                setLanguage(lang_it->second);
            }

            // リクエストを処理
            std::string response_body = handleRequest(
                target,
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
                LOG_ERROR(translate("webui_response_error", "WebUIレスポンス送信エラー: {}"), e.what());
            }

            // 接続を閉じる
            beast::error_code ec;
            stream.socket().shutdown(beast::tcp_socket::shutdown_send, ec);
        }
    } catch (const std::exception& e) {
        LOG_ERROR(translate("webui_server_error", "WebUIサーバーエラー: {}"), e.what());
    }
}

std::string WebUI::handleRequest(const std::string& target, const std::string& method,
                                const std::string& body, 
                                const std::map<std::string, std::string>& headers) {
    try {
        // 認証チェック
        if (auth_enabled_ && !authenticate(headers)) {
            return createErrorResponse(401, translate("authentication_required", "認証が必要です"));
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
        } else if (target == "/language" || target == "/language.html") {
            return generateLanguagePage();
        }
        
        // 静的ファイルの配信
        std::string file_path = document_root_ + target;
        if (fs::exists(file_path) && fs::is_regular_file(file_path)) {
            return serveStaticFile(file_path);
        }
        
        // 404エラー
        return createErrorResponse(404, translate("page_not_found", "ページが見つかりません"));
    } catch (const std::exception& e) {
        LOG_ERROR(translate("webui_request_processing_error", "WebUIリクエスト処理エラー: {}"), e.what());
        return createErrorResponse(500, translate("internal_server_error", "内部サーバーエラー"));
    }
}

std::string WebUI::serveStaticFile(const std::string& file_path) {
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return createErrorResponse(404, translate("file_not_found", "ファイルが見つかりません"));
        }
        
        std::ostringstream content;
        content << file.rdbuf();
        
        std::string mime_type = getMimeType(file_path);
        return createResponse(content.str(), mime_type);
    } catch (const std::exception& e) {
        LOG_ERROR(translate("static_file_error", "静的ファイル配信エラー [{}]: {}"), file_path, e.what());
        return createErrorResponse(500, translate("file_read_error", "ファイル読み取りエラー"));
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
    html << "  <title>" << translate("error", "エラー") << " " << status_code << " - OCPP Gateway</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: Arial, sans-serif; margin: 40px; }\n";
    html << "    .error { color: #d32f2f; }\n";
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <h1 class=\"error\">" << translate("error", "エラー") << " " << status_code << "</h1>\n";
    html << "  <p>" << message << "</p>\n";
    html << "  <hr>\n";
    html << "  <p><a href=\"/\">" << translate("back_to_dashboard", "ダッシュボードに戻る") << "</a></p>\n";
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
        html << "  <title>OCPP Gateway - " << translate("dashboard", "ダッシュボード") << "</title>\n";
        html << "  <style>\n";
        html << generateCSS();
        html << "  </style>\n";
        html << "</head>\n";
        html << "<body>\n";
        html << generateNavigation();
        html << "  <div class=\"container\">\n";
        html << "    <h1>OCPP 2.0.1 " << translate("gateway_middleware", "ゲートウェイ・ミドルウェア") << "</h1>\n";
        
        // システム状態カード
        html << "    <div class=\"card\">\n";
        html << "      <h2>" << translate("system_status", "システム状態") << "</h2>\n";
        html << "      <div class=\"status-grid\">\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">" << translate("status", "状態") << ":</span>\n";
        html << "          <span class=\"status-value status-active\">" << translate("running", "稼働中") << "</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">" << translate("uptime", "稼働時間") << ":</span>\n";
        html << "          <span class=\"status-value\">" << translate("unknown", "不明") << "</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">" << translate("version", "バージョン") << ":</span>\n";
        html << "          <span class=\"status-value\">1.0.0</span>\n";
        html << "        </div>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        // デバイス状態カード
        const auto& device_configs = config_manager.getDeviceConfigs();
        html << "    <div class=\"card\">\n";
        html << "      <h2>" << translate("device_status", "デバイス状態") << "</h2>\n";
        html << "      <div class=\"status-grid\">\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">" << translate("total_devices", "総デバイス数") << ":</span>\n";
        html << "          <span class=\"status-value\">" << device_configs.getDevices().size() << "</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">" << translate("active", "アクティブ") << ":</span>\n";
        html << "          <span class=\"status-value status-active\">0</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"status-item\">\n";
        html << "          <span class=\"status-label\">" << translate("error", "エラー") << ":</span>\n";
        html << "          <span class=\"status-value status-error\">0</span>\n";
        html << "        </div>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        // 最近のイベント
        html << "    <div class=\"card\">\n";
        html << "      <h2>" << translate("recent_events", "最近のイベント") << "</h2>\n";
        html << "      <div class=\"event-list\">\n";
        html << "        <div class=\"event-item\">\n";
        html << "          <span class=\"event-time\">2024-12-20 17:00:00</span>\n";
        html << "          <span class=\"event-message\">" << translate("system_started", "システム開始") << "</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"event-item\">\n";
        html << "          <span class=\"event-time\">2024-12-20 17:00:01</span>\n";
        html << "          <span class=\"event-message\">" << translate("config_loaded", "設定読み込み完了") << "</span>\n";
        html << "        </div>\n";
        html << "        <div class=\"event-item\">\n";
        html << "          <span class=\"event-time\">2024-12-20 17:00:02</span>\n";
        html << "          <span class=\"event-message\">" << translate("admin_api_started", "管理APIサーバー開始") << "</span>\n";
        html << "        </div>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        html << "  </div>\n";
        html << generateJavaScript();
        html << "</body>\n";
        html << "</html>\n";
        
        return html.str();
    } catch (const std::exception& e) {
        LOG_ERROR(translate("dashboard_generation_error", "ダッシュボード生成エラー: {}"), e.what());
        return createErrorResponse(500, translate("dashboard_generation_failed", "ダッシュボードの生成に失敗しました"));
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
        html << "  <title>OCPP Gateway - " << translate("device_management", "デバイス管理") << "</title>\n";
        html << "  <style>\n";
        html << generateCSS();
        html << "  </style>\n";
        html << "</head>\n";
        html << "<body>\n";
        html << generateNavigation();
        html << "  <div class=\"container\">\n";
        html << "    <h1>" << translate("device_management", "デバイス管理") << "</h1>\n";
        
        // デバイス一覧テーブル
        html << "    <div class=\"card\">\n";
        html << "      <h2>" << translate("registered_devices", "登録デバイス一覧") << "</h2>\n";
        html << "      <table class=\"device-table\">\n";
        html << "        <thead>\n";
        html << "          <tr>\n";
        html << "            <th>" << translate("id", "ID") << "</th>\n";
        html << "            <th>" << translate("name", "名前") << "</th>\n";
        html << "            <th>" << translate("protocol", "プロトコル") << "</th>\n";
        html << "            <th>" << translate("template", "テンプレート") << "</th>\n";
        html << "            <th>" << translate("state", "状態") << "</th>\n";
        html << "            <th>" << translate("actions", "操作") << "</th>\n";
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
            html << (device.isEnabled() ? translate("enabled", "有効") : translate("disabled", "無効")) << "</span></td>\n";
            html << "            <td>\n";
            html << "              <button class=\"btn btn-info\" onclick=\"showDeviceDetail('" << device.getId() << "')\">" << translate("details", "詳細") << "</button>\n";
            html << "              <button class=\"btn btn-warning\" onclick=\"editDevice('" << device.getId() << "')\">" << translate("edit", "編集") << "</button>\n";
            html << "            </td>\n";
            html << "          </tr>\n";
        }
        
        if (device_configs.getDevices().empty()) {
            html << "          <tr>\n";
            html << "            <td colspan=\"6\" class=\"text-center\">" << translate("no_devices", "登録されているデバイスはありません") << "</td>\n";
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
        LOG_ERROR(translate("device_page_generation_error", "デバイスページ生成エラー: {}"), e.what());
        return createErrorResponse(500, translate("device_page_generation_failed", "デバイスページの生成に失敗しました"));
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
        html << "  <title>OCPP Gateway - " << translate("config", "設定管理") << "</title>\n";
        html << "  <style>\n";
        html << generateCSS();
        html << "  </style>\n";
        html << "</head>\n";
        html << "<body>\n";
        html << generateNavigation();
        html << "  <div class=\"container\">\n";
        html << "    <h1>" << translate("config", "設定管理") << "</h1>\n";
        
        // システム設定
        const auto& system_config = config_manager.getSystemConfig();
        html << "    <div class=\"card\">\n";
        html << "      <h2>" << translate("system_config", "システム設定") << "</h2>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>" << translate("log_level", "ログレベル") << ":</label>\n";
        html << "        <span>" << system_config.getLogLevel() << "</span>\n";
        html << "      </div>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>" << translate("max_charge_points", "最大充電ポイント数") << ":</label>\n";
        html << "        <span>" << system_config.getMaxChargePoints() << "</span>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        // CSMS設定
        const auto& csms_config = config_manager.getCsmsConfig();
        html << "    <div class=\"card\">\n";
        html << "      <h2>" << translate("csms_config", "CSMS設定") << "</h2>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>" << translate("url", "URL") << ":</label>\n";
        html << "        <span>" << csms_config.getUrl() << "</span>\n";
        html << "      </div>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>" << translate("reconnect_interval", "再接続間隔") << ":</label>\n";
        html << "        <span>" << csms_config.getReconnectInterval() << " " << translate("seconds", "秒") << "</span>\n";
        html << "      </div>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>" << translate("max_reconnect_attempts", "最大再接続回数") << ":</label>\n";
        html << "        <span>" << csms_config.getMaxReconnectAttempts() << "</span>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        // 言語設定
        html << "    <div class=\"card\">\n";
        html << "      <h2>" << translate("language_settings", "言語設定") << "</h2>\n";
        html << "      <div class=\"config-item\">\n";
        html << "        <label>" << translate("current_language", "現在の言語") << ":</label>\n";
        html << "        <span>" << (getCurrentLanguage() == "en" ? translate("english", "英語") : translate("japanese", "日本語")) << "</span>\n";
        html << "      </div>\n";
        html << "      <div class=\"button-group\">\n";
        html << "        <a href=\"/language\" class=\"btn btn-primary\">" << translate("change_language", "言語を変更") << "</a>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        // 操作ボタン
        html << "    <div class=\"card\">\n";
        html << "      <h2>" << translate("config_operations", "設定操作") << "</h2>\n";
        html << "      <div class=\"button-group\">\n";
        html << "        <button class=\"btn btn-primary\" onclick=\"reloadConfig()\">" << translate("reload_config", "設定再読み込み") << "</button>\n";
        html << "        <button class=\"btn btn-info\" onclick=\"validateConfig()\">" << translate("validate_config", "設定検証") << "</button>\n";
        html << "        <button class=\"btn btn-warning\" onclick=\"backupConfig()\">" << translate("backup_config", "設定バックアップ") << "</button>\n";
        html << "      </div>\n";
        html << "    </div>\n";
        
        html << "  </div>\n";
        html << generateJavaScript();
        html << "</body>\n";
        html << "</html>\n";
        
        return html.str();
    } catch (const std::exception& e) {
        LOG_ERROR(translate("config_page_generation_error", "設定ページ生成エラー: {}"), e.what());
        return createErrorResponse(500, translate("config_page_generation_failed", "設定ページの生成に失敗しました"));
    }
}

std::string WebUI::generateLogPage() {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "  <title>OCPP Gateway - " << translate("logs", "ログ表示") << "</title>\n";
    html << "  <style>\n";
    html << generateCSS();
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << generateNavigation();
    html << "  <div class=\"container\">\n";
    html << "    <h1>" << translate("logs", "ログ表示") << "</h1>\n";
    
    html << "    <div class=\"card\">\n";
    html << "      <h2>" << translate("latest_logs", "最新ログ") << "</h2>\n";
    html << "      <div class=\"log-container\">\n";
    html << "        <div class=\"log-entry log-info\">\n";
    html << "          <span class=\"log-time\">2024-12-20 17:00:00</span>\n";
    html << "          <span class=\"log-level\">INFO</span>\n";
    html << "          <span class=\"log-message\">" << translate("webui_started", "WebUIサーバーを開始しました") << "</span>\n";
    html << "        </div>\n";
    html << "        <div class=\"log-entry log-info\">\n";
    html << "          <span class=\"log-time\">2024-12-20 17:00:01</span>\n";
    html << "          <span class=\"log-level\">INFO</span>\n";
    html << "          <span class=\"log-message\">" << translate("admin_api_started", "管理APIサーバーを開始しました") << "</span>\n";
    html << "        </div>\n";
    html << "        <div class=\"log-entry log-info\">\n";
    html << "          <span class=\"log-time\">2024-12-20 17:00:02</span>\n";
    html << "          <span class=\"log-level\">INFO</span>\n";
    html << "          <span class=\"log-message\">" << translate("metrics_initialized", "メトリクス収集を初期化しました") << "</span>\n";
    html << "        </div>\n";
    html << "      </div>\n";
    html << "    </div>\n";
    
    html << "  </div>\n";
    html << generateJavaScript();
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}

std::string WebUI::generateLanguagePage() {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "  <title>OCPP Gateway - " << translate("language_settings", "言語設定") << "</title>\n";
    html << "  <style>\n";
    html << generateCSS();
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << generateNavigation();
    html << "  <div class=\"container\">\n";
    html << "    <h1>" << translate("language_settings", "言語設定") << "</h1>\n";
    
    html << "    <div class=\"card\">\n";
    html << "      <h2>" << translate("select_language", "言語を選択") << "</h2>\n";
    html << "      <div class=\"language-selection\">\n";
    
    // 言語選択リスト
    std::vector<std::string> available_languages = getAvailableLanguages();
    std::string current_language = getCurrentLanguage();
    
    for (const auto& lang : available_languages) {
        std::string lang_name = (lang == "en") ? translate("english", "英語") : translate("japanese", "日本語");
        std::string lang_class = (lang == current_language) ? "language-item selected" : "language-item";
        
        html << "        <div class=\"" << lang_class << "\">\n";
        html << "          <a href=\"?lang=" << lang << "\" class=\"language-link\">\n";
        html << "            <span class=\"language-name\">" << lang_name << "</span>\n";
        if (lang == current_language) {
            html << "            <span class=\"language-current\">(" << translate("current", "現在") << ")</span>\n";
        }
        html << "          </a>\n";
        html << "        </div>\n";
    }
    
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
    nav << "      <a href=\"/dashboard\">" << translate("dashboard", "ダッシュボード") << "</a>\n";
    nav << "      <a href=\"/devices\">" << translate("devices", "デバイス") << "</a>\n";
    nav << "      <a href=\"/config\">" << translate("config", "設定") << "</a>\n";
    nav << "      <a href=\"/logs\">" << translate("logs", "ログ") << "</a>\n";
    nav << "      <a href=\"/language\">" << translate("language", "言語") << "</a>\n";
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
    css << "    .btn { padding: 0.5rem 1rem; border: none; border-radius: 4px; cursor: pointer; margin-right: 0.5rem; transition: opacity 0.3s; display: inline-block; text-decoration: none; }\n";
    css << "    .btn:hover { opacity: 0.8; }\n";
    css << "    .btn-primary { background-color: #2196F3; color: white; }\n";
    css << "    .btn-info { background-color: #00BCD4; color: white; }\n";
    css << "    .btn-warning { background-color: #FF9800; color: white; }\n";
    css << "    .config-item { display: flex; justify-content: space-between; padding: 0.5rem 0; border-bottom: 1px solid #eee; }\n";
    css << "    .config-item label { font-weight: bold; }\n";
    css << "    .button-group { display: flex; gap: 1rem; flex-wrap: wrap; margin-top: 1rem; }\n";
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
    css << "    .language-selection { display: flex; flex-direction: column; gap: 1rem; }\n";
    css << "    .language-item { padding: 1rem; border: 1px solid #ddd; border-radius: 4px; transition: all 0.3s; }\n";
    css << "    .language-item:hover { background-color: #f5f5f5; }\n";
    css << "    .language-item.selected { border-color: #2196F3; background-color: #e3f2fd; }\n";
    css << "    .language-link { text-decoration: none; color: #333; display: flex; justify-content: space-between; align-items: center; }\n";
    css << "    .language-name { font-weight: bold; }\n";
    css << "    .language-current { color: #2196F3; }\n";
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
    js << "      alert('" << translate("device_detail", "デバイス詳細") << ": ' + deviceId + ' (" << translate("not_implemented", "未実装") << ")');\n";
    js << "    }\n";
    js << "    function editDevice(deviceId) {\n";
    js << "      alert('" << translate("device_edit", "デバイス編集") << ": ' + deviceId + ' (" << translate("not_implemented", "未実装") << ")');\n";
    js << "    }\n";
    js << "    function reloadConfig() {\n";
    js << "      if (confirm('" << translate("reload_config_confirm", "設定を再読み込みしますか？") << "')) {\n";
    js << "        alert('" << translate("reload_config", "設定再読み込み") << " (" << translate("not_implemented", "未実装") << ")');\n";
    js << "      }\n";
    js << "    }\n";
    js << "    function validateConfig() {\n";
    js << "      alert('" << translate("validate_config", "設定検証") << " (" << translate("not_implemented", "未実装") << ")');\n";
    js << "    }\n";
    js << "    function backupConfig() {\n";
    js << "      alert('" << translate("backup_config", "設定バックアップ") << " (" << translate("not_implemented", "未実装") << ")');\n";
    js << "    }\n";
    js << "    // 自動更新（30秒間隔）\n";
    js << "    setInterval(function() {\n";
    js << "      if (window.location.pathname === '/dashboard' || window.location.pathname === '/') {\n";
    js << "        // ダッシュボードのみ自動更新\n";
    js << "        console.log('" << translate("dashboard_auto_refresh", "ダッシュボード自動更新") << " (" << translate("not_implemented", "未実装") << ")');\n";
    js << "      }\n";
    js << "    }, 30000);\n";
    js << "  </script>\n";
    return js.str();
}

} // namespace api
} // namespace ocpp_gateway