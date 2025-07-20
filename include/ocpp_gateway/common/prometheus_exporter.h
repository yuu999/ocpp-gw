#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <functional>

namespace ocpp_gateway {
namespace common {

class MetricsCollector;

/**
 * @brief Prometheusメトリクスエクスポーター
 * 
 * Prometheus形式でメトリクスを公開するHTTPサーバーを提供します。
 * メトリクス収集システムと連携してリアルタイムでメトリクスを配信します。
 */
class PrometheusExporter {
public:
    /**
     * @brief コンストラクタ
     * @param port HTTPサーバーのポート番号
     * @param bind_address バインドアドレス
     * @param metrics_path メトリクスエンドポイントのパス
     */
    PrometheusExporter(int port = 9090, const std::string& bind_address = "0.0.0.0",
                      const std::string& metrics_path = "/metrics");

    /**
     * @brief デストラクタ
     */
    ~PrometheusExporter();

    /**
     * @brief エクスポーターを開始
     * @return 成功時true、失敗時false
     */
    bool start();

    /**
     * @brief エクスポーターを停止
     */
    void stop();

    /**
     * @brief エクスポーターが実行中かどうか
     * @return 実行中時true
     */
    bool isRunning() const;

    /**
     * @brief ヘルスチェックエンドポイントを有効化
     * @param enabled 有効フラグ
     */
    void enableHealthCheck(bool enabled = true);

    /**
     * @brief カスタムラベルを設定
     * @param labels グローバルラベル
     */
    void setGlobalLabels(const std::map<std::string, std::string>& labels);

    /**
     * @brief メトリクスフィルターを設定
     * @param filter_func フィルター関数（メトリクス名を受け取り、エクスポート対象かboolで返す）
     */
    void setMetricsFilter(std::function<bool(const std::string&)> filter_func);

private:
    /**
     * @brief HTTPサーバーのメインループ
     */
    void runServer();

    /**
     * @brief HTTPリクエストを処理
     * @param target リクエストターゲット
     * @param method HTTPメソッド
     * @return レスポンス文字列
     */
    std::string handleRequest(const std::string& target, const std::string& method);

    /**
     * @brief Prometheusメトリクスを生成
     * @return Prometheus形式のメトリクス文字列
     */
    std::string generatePrometheusMetrics();

    /**
     * @brief ヘルスチェックレスポンスを生成
     * @return ヘルスチェック結果
     */
    std::string generateHealthResponse();

    /**
     * @brief HTTPレスポンスを生成
     * @param status_code ステータスコード
     * @param content レスポンス内容
     * @param content_type Content-Type
     * @return HTTPレスポンス文字列
     */
    std::string createHttpResponse(int status_code, const std::string& content,
                                  const std::string& content_type = "text/plain");

    /**
     * @brief エラーレスポンスを生成
     * @param status_code ステータスコード
     * @param message エラーメッセージ
     * @return エラーレスポンス
     */
    std::string createErrorResponse(int status_code, const std::string& message);

    /**
     * @brief メトリクス名にプレフィックスを追加
     * @param metric_name 元のメトリクス名
     * @return プレフィックス付きメトリクス名
     */
    std::string addMetricPrefix(const std::string& metric_name);

    /**
     * @brief グローバルラベルを文字列として取得
     * @return ラベル文字列
     */
    std::string getGlobalLabelsString();

    // メンバー変数
    int port_;                                          ///< ポート番号
    std::string bind_address_;                          ///< バインドアドレス
    std::string metrics_path_;                          ///< メトリクスエンドポイントパス
    std::atomic<bool> running_;                         ///< 実行状態
    std::thread server_thread_;                         ///< サーバースレッド
    
    // 機能設定
    bool health_check_enabled_;                         ///< ヘルスチェック有効フラグ
    std::map<std::string, std::string> global_labels_;  ///< グローバルラベル
    std::function<bool(const std::string&)> metrics_filter_; ///< メトリクスフィルター
    
    // メトリクス収集システムへの参照
    MetricsCollector& metrics_collector_;               ///< メトリクス収集システム
    
    // 統計情報
    std::atomic<uint64_t> requests_total_;              ///< 総リクエスト数
    std::atomic<uint64_t> requests_errors_;             ///< エラーリクエスト数
};

} // namespace common
} // namespace ocpp_gateway 