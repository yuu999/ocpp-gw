#pragma once

#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/config_manager.h"
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <thread>
#include <atomic>

namespace ocpp_gateway {
namespace api {

/**
 * @brief HTTPメソッドの列挙型
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH
};

/**
 * @brief HTTPレスポンスの構造体
 */
struct HttpResponse {
    int status_code;
    std::string content_type;
    std::string body;
    std::map<std::string, std::string> headers;
    
    HttpResponse() : status_code(200), content_type("application/json") {}
};

/**
 * @brief HTTPリクエストの構造体
 */
struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    
    HttpRequest() : method(HttpMethod::GET) {}
};

/**
 * @brief ルートハンドラーの型定義
 */
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

/**
 * @brief 管理APIサーバークラス
 * 
 * RESTful APIサーバーを提供し、システムの管理機能を実装します。
 * 設定管理、デバイス管理、監視機能などのAPIエンドポイントを提供します。
 */
class AdminApi {
public:
    /**
     * @brief コンストラクタ
     * @param port サーバーのポート番号
     * @param bind_address バインドアドレス
     */
    explicit AdminApi(int port = 8080, const std::string& bind_address = "127.0.0.1");

    /**
     * @brief デストラクタ
     */
    ~AdminApi();

    /**
     * @brief サーバーを開始
     * @return 開始成功時true
     */
    bool start();

    /**
     * @brief サーバーを停止
     */
    void stop();

    /**
     * @brief サーバーが実行中かどうかを確認
     * @return 実行中の場合true
     */
    bool isRunning() const;

    /**
     * @brief ルートを登録
     * @param method HTTPメソッド
     * @param path パス
     * @param handler ハンドラー関数
     */
    void registerRoute(HttpMethod method, const std::string& path, RouteHandler handler);

    /**
     * @brief 認証を設定
     * @param enabled 認証を有効にするかどうか
     * @param username ユーザー名
     * @param password パスワード
     */
    void setAuthentication(bool enabled, const std::string& username = "", const std::string& password = "");

private:
    /**
     * @brief サーバーを実行するスレッド関数
     */
    void runServer();

    /**
     * @brief リクエストを処理
     * @param request HTTPリクエスト
     * @return HTTPレスポンス
     */
    HttpResponse handleRequest(const HttpRequest& request);

    /**
     * @brief 認証を検証
     * @param request HTTPリクエスト
     * @return 認証成功時true
     */
    bool authenticate(const HttpRequest& request);

    /**
     * @brief JSONレスポンスを生成
     * @param status_code ステータスコード
     * @param data JSONデータ
     * @return HTTPレスポンス
     */
    HttpResponse createJsonResponse(int status_code, const std::string& data);

    /**
     * @brief エラーレスポンスを生成
     * @param status_code ステータスコード
     * @param message エラーメッセージ
     * @return HTTPレスポンス
     */
    HttpResponse createErrorResponse(int status_code, const std::string& message);

    // APIエンドポイントハンドラー
    HttpResponse handleGetSystemInfo(const HttpRequest& request);
    HttpResponse handleGetDevices(const HttpRequest& request);
    HttpResponse handleGetDevice(const HttpRequest& request);
    HttpResponse handleAddDevice(const HttpRequest& request);
    HttpResponse handleUpdateDevice(const HttpRequest& request);
    HttpResponse handleDeleteDevice(const HttpRequest& request);
    HttpResponse handleGetConfig(const HttpRequest& request);
    HttpResponse handleUpdateConfig(const HttpRequest& request);
    HttpResponse handleReloadConfig(const HttpRequest& request);
    HttpResponse handleGetMetrics(const HttpRequest& request);
    HttpResponse handleGetHealth(const HttpRequest& request);

    int port_;
    std::string bind_address_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    std::map<std::string, RouteHandler> routes_;
    
    // 認証設定
    bool auth_enabled_;
    std::string auth_username_;
    std::string auth_password_;
    
    // 設定マネージャーの参照
    config::ConfigManager& config_manager_;
    
    // 開始時刻
    std::time_t start_time_ = std::time(nullptr);
};

} // namespace api
} // namespace ocpp_gateway 