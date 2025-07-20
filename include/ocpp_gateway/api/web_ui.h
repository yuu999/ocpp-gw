#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <map>
#include <functional>

namespace ocpp_gateway {
namespace api {

/**
 * @brief WebUI管理クラス
 * 
 * HTMLベースの管理インターフェースを提供します。
 * 静的ファイルの配信とAPIとの連携を行います。
 */
class WebUI {
public:
    /**
     * @brief コンストラクタ
     * @param port HTTPサーバーのポート番号
     * @param bind_address バインドアドレス
     * @param document_root 静的ファイルのルートディレクトリ
     */
    WebUI(int port = 8081, const std::string& bind_address = "0.0.0.0", 
          const std::string& document_root = "web");

    /**
     * @brief デストラクタ
     */
    ~WebUI();

    /**
     * @brief WebUIサーバーを開始
     * @return 成功時true、失敗時false
     */
    bool start();

    /**
     * @brief WebUIサーバーを停止
     */
    void stop();

    /**
     * @brief WebUIサーバーが実行中かどうか
     * @return 実行中時true
     */
    bool isRunning() const;

    /**
     * @brief 静的ファイルのルートディレクトリを設定
     * @param root_path ルートディレクトリパス
     */
    void setDocumentRoot(const std::string& root_path);

    /**
     * @brief 認証を設定
     * @param enabled 認証有効フラグ
     * @param username ユーザー名
     * @param password パスワード
     */
    void setAuthentication(bool enabled, const std::string& username = "", 
                          const std::string& password = "");

private:
    /**
     * @brief HTTPサーバーのメインループ
     */
    void runServer();

    /**
     * @brief HTTPリクエストを処理
     * @param target リクエストターゲット
     * @param method HTTPメソッド
     * @param body リクエストボディ
     * @param headers HTTPヘッダー
     * @return レスポンス文字列
     */
    std::string handleRequest(const std::string& target, const std::string& method,
                             const std::string& body, 
                             const std::map<std::string, std::string>& headers);

    /**
     * @brief 静的ファイルを配信
     * @param file_path ファイルパス
     * @return ファイル内容（見つからない場合は404レスポンス）
     */
    std::string serveStaticFile(const std::string& file_path);

    /**
     * @brief MIMEタイプを取得
     * @param file_path ファイルパス
     * @return MIMEタイプ
     */
    std::string getMimeType(const std::string& file_path);

    /**
     * @brief 認証チェック
     * @param headers HTTPヘッダー
     * @return 認証成功時true
     */
    bool authenticate(const std::map<std::string, std::string>& headers);

    /**
     * @brief エラーレスポンスを生成
     * @param status_code ステータスコード
     * @param message エラーメッセージ
     * @return エラーレスポンス
     */
    std::string createErrorResponse(int status_code, const std::string& message);

    /**
     * @brief 成功レスポンスを生成
     * @param content レスポンス内容
     * @param content_type Content-Type
     * @return レスポンス
     */
    std::string createResponse(const std::string& content, 
                              const std::string& content_type = "text/html");

    /**
     * @brief デフォルトのHTMLページを生成
     * @return HTMLコンテンツ
     */
    std::string generateDefaultPage();

    /**
     * @brief ダッシュボードページを生成
     * @return HTMLコンテンツ
     */
    std::string generateDashboard();

    /**
     * @brief デバイス管理ページを生成
     * @return HTMLコンテンツ
     */
    std::string generateDevicePage();

    /**
     * @brief 設定管理ページを生成
     * @return HTMLコンテンツ
     */
    std::string generateConfigPage();

    /**
     * @brief ログ表示ページを生成
     * @return HTMLコンテンツ
     */
    std::string generateLogPage();

    /**
     * @brief ナビゲーションバーを生成
     * @return HTMLコンテンツ
     */
    std::string generateNavigation();

    /**
     * @brief CSSスタイルを生成
     * @return CSSコンテンツ
     */
    std::string generateCSS();

    /**
     * @brief JavaScriptコードを生成
     * @return JavaScriptコンテンツ
     */
    std::string generateJavaScript();

    // メンバー変数
    int port_;                          ///< ポート番号
    std::string bind_address_;          ///< バインドアドレス
    std::string document_root_;         ///< 静的ファイルルート
    std::atomic<bool> running_;         ///< 実行状態
    std::thread server_thread_;         ///< サーバースレッド
    
    // 認証設定
    bool auth_enabled_;                 ///< 認証有効フラグ
    std::string auth_username_;         ///< 認証ユーザー名
    std::string auth_password_;         ///< 認証パスワード
    
    // MIMEタイプマップ
    std::map<std::string, std::string> mime_types_;
};

} // namespace api
} // namespace ocpp_gateway 