#include "ocpp_gateway/api/admin_api.h"
#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/metrics_collector.h"
#include <json/json.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <thread>
#include <sstream>
#include <regex>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

namespace ocpp_gateway {
namespace api {

AdminApi::AdminApi(int port, const std::string& bind_address)
    : port_(port), bind_address_(bind_address), running_(false),
      auth_enabled_(false), config_manager_(config::ConfigManager::getInstance()) {
    
    // デフォルトルートの登録
    registerRoute(HttpMethod::GET, "/api/v1/system/info", 
                 [this](const HttpRequest& req) { return handleGetSystemInfo(req); });
    
    registerRoute(HttpMethod::GET, "/api/v1/devices", 
                 [this](const HttpRequest& req) { return handleGetDevices(req); });
    
    registerRoute(HttpMethod::GET, "/api/v1/devices/{id}", 
                 [this](const HttpRequest& req) { return handleGetDevice(req); });
    
    registerRoute(HttpMethod::POST, "/api/v1/devices", 
                 [this](const HttpRequest& req) { return handleAddDevice(req); });
    
    registerRoute(HttpMethod::PUT, "/api/v1/devices/{id}", 
                 [this](const HttpRequest& req) { return handleUpdateDevice(req); });
    
    registerRoute(HttpMethod::DELETE, "/api/v1/devices/{id}", 
                 [this](const HttpRequest& req) { return handleDeleteDevice(req); });
    
    registerRoute(HttpMethod::GET, "/api/v1/config", 
                 [this](const HttpRequest& req) { return handleGetConfig(req); });
    
    registerRoute(HttpMethod::PUT, "/api/v1/config", 
                 [this](const HttpRequest& req) { return handleUpdateConfig(req); });
    
    registerRoute(HttpMethod::POST, "/api/v1/config/reload", 
                 [this](const HttpRequest& req) { return handleReloadConfig(req); });
    
    registerRoute(HttpMethod::GET, "/api/v1/metrics", 
                 [this](const HttpRequest& req) { return handleGetMetrics(req); });
    
    registerRoute(HttpMethod::GET, "/api/v1/health", 
                 [this](const HttpRequest& req) { return handleGetHealth(req); });
}

AdminApi::~AdminApi() {
    stop();
}

bool AdminApi::start() {
    if (running_.load()) {
        LOG_WARN("管理APIサーバーは既に実行中です");
        return true;
    }

    try {
        running_ = true;
        server_thread_ = std::thread(&AdminApi::runServer, this);
        
        // サーバー開始まで少し待機
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        LOG_INFO("管理APIサーバーを開始しました: {}:{}", bind_address_, port_);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("管理APIサーバーの開始に失敗しました: {}", e.what());
        running_ = false;
        return false;
    }
}

void AdminApi::stop() {
    if (!running_.load()) {
        return;
    }

    running_ = false;
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    LOG_INFO("管理APIサーバーを停止しました");
}

bool AdminApi::isRunning() const {
    return running_.load();
}

void AdminApi::registerRoute(HttpMethod method, const std::string& path, RouteHandler handler) {
    std::string key = std::to_string(static_cast<int>(method)) + ":" + path;
    routes_[key] = handler;
}

void AdminApi::setAuthentication(bool enabled, const std::string& username, const std::string& password) {
    auth_enabled_ = enabled;
    auth_username_ = username;
    auth_password_ = password;
    
    if (enabled) {
        LOG_INFO("管理API認証を有効にしました（ユーザー: {}）", username);
    } else {
        LOG_INFO("管理API認証を無効にしました");
    }
}

void AdminApi::runServer() {
    try {
        auto const address = net::ip::make_address(bind_address_);
        [[maybe_unused]] auto const port = static_cast<unsigned short>(port_);

        net::io_context ioc{1};
        net::ip::tcp::acceptor acceptor{ioc, {address, port_}};

        while (running_.load()) {
            beast::tcp_stream stream{ioc};
            
            // 接続を受け入れ
            try {
                acceptor.accept(stream.socket());
            } catch (const std::exception& e) {
                if (running_.load()) {
                    LOG_ERROR("接続受け入れエラー: {}", e.what());
                }
                continue;
            }

            // リクエストを読み取り
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            
            try {
                http::read(stream, buffer, req);
            } catch (const std::exception& e) {
                LOG_ERROR("リクエスト読み取りエラー: {}", e.what());
                continue;
            }

            // リクエストを処理
            HttpRequest request;
            request.method = static_cast<HttpMethod>(req.method());
            request.path = std::string(req.target());
            request.body = req.body();
            
            // ヘッダーをコピー
            for (auto const& field : req) {
                request.headers[std::string(field.name_string())] = std::string(field.value());
            }

            // クエリパラメータを解析
            auto query_pos = request.path.find('?');
            if (query_pos != std::string::npos) {
                std::string query = request.path.substr(query_pos + 1);
                request.path = request.path.substr(0, query_pos);
                
                // 簡単なクエリパラメータ解析
                std::istringstream iss(query);
                std::string pair;
                while (std::getline(iss, pair, '&')) {
                    auto eq_pos = pair.find('=');
                    if (eq_pos != std::string::npos) {
                        std::string key = pair.substr(0, eq_pos);
                        std::string value = pair.substr(eq_pos + 1);
                        request.query_params[key] = value;
                    }
                }
            }

            HttpResponse response = handleRequest(request);

            // レスポンスを作成
            http::response<http::string_body> res{
                static_cast<http::status>(response.status_code), req.version()};
            res.set(http::field::server, "OCPP Gateway API Server");
            res.set(http::field::content_type, response.content_type);
            
            for (const auto& header : response.headers) {
                res.set(header.first, header.second);
            }
            
            res.body() = response.body;
            res.prepare_payload();

            // レスポンスを送信
            try {
                http::write(stream, res);
            } catch (const std::exception& e) {
                LOG_ERROR("レスポンス送信エラー: {}", e.what());
            }

            // 接続を閉じる
            beast::error_code ec;
            stream.socket().shutdown(net::ip::tcp::socket::shutdown_send, ec);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("管理APIサーバーエラー: {}", e.what());
    }
}

HttpResponse AdminApi::handleRequest(const HttpRequest& request) {
    try {
        // 認証チェック
        if (auth_enabled_ && !authenticate(request)) {
            return createErrorResponse(401, "認証が必要です");
        }

        // ルートマッチング
        std::string method_str = std::to_string(static_cast<int>(request.method));
        
        // 完全一致を最初に試す
        std::string exact_key = method_str + ":" + request.path;
        auto exact_it = routes_.find(exact_key);
        if (exact_it != routes_.end()) {
            return exact_it->second(request);
        }

        // パターンマッチング（{id}などのパラメータ）
        for (const auto& route : routes_) {
            std::string route_method = route.first.substr(0, route.first.find(':'));
            std::string route_path = route.first.substr(route.first.find(':') + 1);
            
            if (route_method != method_str) {
                continue;
            }

            // パスパラメータのマッチング
            std::regex pattern(std::regex_replace(route_path, std::regex("\\{[^}]+\\}"), "([^/]+)"));
            if (std::regex_match(request.path, pattern)) {
                return route.second(request);
            }
        }

        return createErrorResponse(404, "エンドポイントが見つかりません");
    } catch (const std::exception& e) {
        LOG_ERROR("リクエスト処理エラー: {}", e.what());
        return createErrorResponse(500, "内部サーバーエラー");
    }
}

bool AdminApi::authenticate(const HttpRequest& request) {
    auto auth_it = request.headers.find("Authorization");
    if (auth_it == request.headers.end()) {
        return false;
    }

    std::string auth_header = auth_it->second;
    if (auth_header.substr(0, 6) != "Basic ") {
        return false;
    }

    // Base64デコードは簡易実装（実際にはBoost.Beastのユーティリティを使用）
    std::string credentials = auth_header.substr(6);
    
    // 簡易的な認証チェック（実際にはBase64デコードを実装）
    std::string expected = auth_username_ + ":" + auth_password_;
    
    return credentials == expected; // 実際にはBase64エンコード済みの値と比較
}

HttpResponse AdminApi::createJsonResponse(int status_code, const std::string& data) {
    HttpResponse response;
    response.status_code = status_code;
    response.content_type = "application/json";
    response.body = data;
    response.headers["Access-Control-Allow-Origin"] = "*";
    response.headers["Cache-Control"] = "no-cache";
    return response;
}

HttpResponse AdminApi::createErrorResponse(int status_code, const std::string& message) {
    Json::Value error;
    error["status"] = "error";
    error["message"] = message;
    error["timestamp"] = static_cast<Json::Value::Int64>(std::time(nullptr));

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::string json_string = Json::writeString(builder, error);

    return createJsonResponse(status_code, json_string);
}

HttpResponse AdminApi::handleGetSystemInfo([[maybe_unused]] const HttpRequest& request) {
    try {
        Json::Value info;
        info["name"] = "OCPP 2.0.1 Gateway Middleware";
        info["version"] = "1.0.0";
        info["build_date"] = __DATE__;
        info["build_time"] = __TIME__;
        // Calculate uptime
        if (start_time_ > 0) {
            info["uptime_seconds"] = static_cast<Json::Value::Int64>(std::time(nullptr) - start_time_);
        } else {
            info["uptime_seconds"] = 0;
        }
        
        const auto& system_config = config_manager_.getSystemConfig();
        info["log_level"] = config::logLevelToString(system_config.getLogLevel());
        // 'charge_points' is not in SystemConfig, commenting out for now.
        // It might belong in CSMSConfig.
        // info["max_charge_points"] = static_cast<int>(system_config.charge_points);
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::string json_string = Json::writeString(builder, info);

        return createJsonResponse(200, json_string);
    } catch (const std::exception& e) {
        LOG_ERROR("システム情報取得エラー: {}", e.what());
        return createErrorResponse(500, "システム情報の取得に失敗しました");
    }
}

HttpResponse AdminApi::handleGetDevices([[maybe_unused]] const HttpRequest& request) {
    try {
        const auto& device_configs = config_manager_.getDeviceConfigs();
        
        Json::Value devices(Json::arrayValue);
        for (const auto& device : device_configs.getDevices()) {
            Json::Value device_json;
            device_json["id"] = device.getId();
            device_json["name"] = device.getId(); // No 'name' getter, using ID.
            device_json["protocol"] = config::protocolToString(device.getProtocol());
            device_json["template"] = device.getTemplateId();
            device_json["enabled"] = true; // No 'enabled' getter, assuming true.
            devices.append(device_json);
        }

        Json::Value response;
        response["devices"] = devices;
        response["total"] = static_cast<int>(devices.size());

        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::string json_string = Json::writeString(builder, response);

        return createJsonResponse(200, json_string);
    } catch (const std::exception& e) {
        LOG_ERROR("デバイス一覧取得エラー: {}", e.what());
        return createErrorResponse(500, "デバイス一覧の取得に失敗しました");
    }
}

HttpResponse AdminApi::handleGetDevice(const HttpRequest& request) {
    // パスからデバイスIDを抽出
    std::regex pattern("/api/v1/devices/([^/]+)");
    std::smatch matches;
    
    if (!std::regex_match(request.path, matches, pattern)) {
        return createErrorResponse(400, "無効なデバイスIDです");
    }
    
    std::string device_id = matches[1];
    
    try {
        auto device_opt = config_manager_.getDeviceConfig(device_id);
        if (!device_opt) {
            return createErrorResponse(404, "デバイスが見つかりません");
        }

        const auto& device = device_opt.value();
        Json::Value device_json;
        device_json["id"] = device.getId();
        device_json["name"] = device.getId(); // No 'name' getter
        device_json["description"] = ""; // No 'description' getter
        device_json["protocol"] = config::protocolToString(device.getProtocol());
        device_json["template"] = device.getTemplateId();
        device_json["enabled"] = true; // No 'enabled' getter
        
        // 接続設定
        Json::Value connection_json;
        const auto& connection_config = device.getConnection();
        
        std::visit([&connection_json](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, config::ModbusTcpConnectionConfig>) {
                connection_json["ip"] = arg.ip;
                connection_json["port"] = arg.port;
                connection_json["unit_id"] = arg.unit_id;
            } else if constexpr (std::is_same_v<T, config::ModbusRtuConnectionConfig>) {
                connection_json["port"] = arg.port;
                connection_json["baud_rate"] = arg.baud_rate;
            } else if constexpr (std::is_same_v<T, config::EchonetLiteConnectionConfig>) {
                connection_json["ip"] = arg.ip;
            }
        }, connection_config);
        
        device_json["connection"] = connection_json;

        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::string json_string = Json::writeString(builder, device_json);

        return createJsonResponse(200, json_string);
    } catch (const std::exception& e) {
        LOG_ERROR("デバイス取得エラー: {}", e.what());
        return createErrorResponse(500, "デバイス情報の取得に失敗しました");
    }
}

// その他のハンドラーメソッドは簡易実装
HttpResponse AdminApi::handleAddDevice([[maybe_unused]] const HttpRequest& request) {
    return createErrorResponse(501, "デバイス追加機能は未実装です");
}

HttpResponse AdminApi::handleUpdateDevice([[maybe_unused]] const HttpRequest& request) {
    return createErrorResponse(501, "デバイス更新機能は未実装です");
}

HttpResponse AdminApi::handleDeleteDevice([[maybe_unused]] const HttpRequest& request) {
    return createErrorResponse(501, "デバイス削除機能は未実装です");
}

HttpResponse AdminApi::handleGetConfig([[maybe_unused]] const HttpRequest& request) {
    return createErrorResponse(501, "設定取得機能は未実装です");
}

HttpResponse AdminApi::handleUpdateConfig([[maybe_unused]] const HttpRequest& request) {
    return createErrorResponse(501, "設定更新機能は未実装です");
}

HttpResponse AdminApi::handleReloadConfig([[maybe_unused]] const HttpRequest& request) {
    try {
        if (config_manager_.reloadAllConfigs()) {
            Json::Value response;
            response["status"] = "success";
            response["message"] = "設定を再読み込みしました";
            response["timestamp"] = static_cast<Json::Value::Int64>(std::time(nullptr));

            Json::StreamWriterBuilder builder;
            builder["indentation"] = "  ";
            std::string json_string = Json::writeString(builder, response);

            return createJsonResponse(200, json_string);
        } else {
            return createErrorResponse(500, "設定の再読み込みに失敗しました");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("設定再読み込みエラー: {}", e.what());
        return createErrorResponse(500, "設定の再読み込みに失敗しました");
    }
}

HttpResponse AdminApi::handleGetMetrics(const HttpRequest& request) {
    try {
        auto& metrics_collector = ocpp_gateway::common::MetricsCollector::getInstance();
        
        // フォーマットパラメータをチェック
        std::string format = "json"; // デフォルトはJSON
        auto format_it = request.query_params.find("format");
        if (format_it != request.query_params.end()) {
            format = format_it->second;
        }
        
        if (format == "prometheus") {
            // Prometheus形式
            std::string prometheus_data = metrics_collector.getMetricsAsPrometheus();
            HttpResponse response;
            response.status_code = 200;
            response.content_type = "text/plain; version=0.0.4; charset=utf-8";
            response.body = prometheus_data;
            response.headers["Access-Control-Allow-Origin"] = "*";
            response.headers["Cache-Control"] = "no-cache";
            return response;
        } else {
            // JSON形式（デフォルト）
            std::string json_data = metrics_collector.getMetricsAsJson();
            return createJsonResponse(200, json_data);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("メトリクス取得エラー: {}", e.what());
        return createErrorResponse(500, "メトリクスの取得に失敗しました");
    }
}

HttpResponse AdminApi::handleGetHealth([[maybe_unused]] const HttpRequest& request) {
    Json::Value health;
    health["status"] = "ok";
    health["timestamp"] = static_cast<Json::Value::Int64>(std::time(nullptr));
    health["uptime_seconds"] = static_cast<Json::Value::Int64>(std::time(nullptr) - start_time_);

    // Check OCPP server status (placeholder)
    Json::Value ocpp_server;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::string json_string = Json::writeString(builder, health);

    return createJsonResponse(200, json_string);
}

} // namespace api
} // namespace ocpp_gateway 