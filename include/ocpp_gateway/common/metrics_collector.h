#pragma once

#include "ocpp_gateway/common/logger.h"
#include <string>
#include <map>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace ocpp_gateway {
namespace common {

/**
 * @brief メトリクスタイプの列挙型
 */
enum class MetricType {
    COUNTER,    // カウンター（増加のみ）
    GAUGE,      // ゲージ（増減可能）
    HISTOGRAM,  // ヒストグラム
    SUMMARY     // サマリー
};

/**
 * @brief メトリクス値の構造体
 */
struct MetricValue {
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> labels;
    
    MetricValue() : value(0.0) {}
    explicit MetricValue(double v) : value(v), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief メトリクスエントリの構造体
 */
struct MetricEntry {
    std::string name;
    std::string description;
    MetricType type;
    std::map<std::string, MetricValue> values;
    std::mutex mutex;
    
    MetricEntry() : type(MetricType::GAUGE) {}
    MetricEntry(const std::string& n, const std::string& desc, MetricType t)
        : name(n), description(desc), type(t) {}
};

/**
 * @brief メトリクス収集クラス
 * 
 * システムのパフォーマンス、デバイス通信、OCPPメッセージなどの
 * メトリクスを収集・管理します。
 */
class MetricsCollector {
public:
    /**
     * @brief シングルトンインスタンスを取得
     * @return MetricsCollectorの参照
     */
    static MetricsCollector& getInstance();

    /**
     * @brief メトリクスを初期化
     * @return 初期化成功時true
     */
    bool initialize();

    /**
     * @brief メトリクスを終了
     */
    void shutdown();

    /**
     * @brief カウンターメトリクスを増加
     * @param name メトリクス名
     * @param value 増加値
     * @param labels ラベル
     */
    void incrementCounter(const std::string& name, double value = 1.0, 
                         const std::map<std::string, std::string>& labels = {});

    /**
     * @brief ゲージメトリクスを設定
     * @param name メトリクス名
     * @param value 値
     * @param labels ラベル
     */
    void setGauge(const std::string& name, double value,
                  const std::map<std::string, std::string>& labels = {});

    /**
     * @brief ヒストグラムメトリクスを記録
     * @param name メトリクス名
     * @param value 値
     * @param labels ラベル
     */
    void recordHistogram(const std::string& name, double value,
                        const std::map<std::string, std::string>& labels = {});

    /**
     * @brief サマリーメトリクスを記録
     * @param name メトリクス名
     * @param value 値
     * @param labels ラベル
     */
    void recordSummary(const std::string& name, double value,
                      const std::map<std::string, std::string>& labels = {});

    /**
     * @brief メトリクスを取得
     * @param name メトリクス名
     * @return メトリクスエントリ（存在しない場合はnullptr）
     */
    std::shared_ptr<MetricEntry> getMetric(const std::string& name);

    /**
     * @brief すべてのメトリクスを取得
     * @return メトリクスエントリのマップ
     */
    std::map<std::string, std::shared_ptr<MetricEntry>> getAllMetrics();

    /**
     * @brief メトリクスをJSON形式で取得
     * @return JSON文字列
     */
    std::string getMetricsAsJson();

    /**
     * @brief メトリクスをPrometheus形式で取得
     * @return Prometheus形式の文字列
     */
    std::string getMetricsAsPrometheus();

    /**
     * @brief メトリクスをリセット
     * @param name メトリクス名（空の場合はすべて）
     */
    void resetMetrics(const std::string& name = "");

    /**
     * @brief システムメトリクスを更新
     */
    void updateSystemMetrics();

    /**
     * @brief デバイスメトリクスを更新
     * @param device_id デバイスID
     * @param status デバイス状態
     * @param response_time 応答時間
     */
    void updateDeviceMetrics(const std::string& device_id, 
                           const std::string& status,
                           double response_time);

    /**
     * @brief OCPPメトリクスを更新
     * @param message_type メッセージタイプ
     * @param success 成功フラグ
     * @param response_time 応答時間
     */
    void updateOcppMetrics(const std::string& message_type,
                          bool success,
                          double response_time);

private:
    /**
     * @brief コンストラクタ
     */
    MetricsCollector();

    /**
     * @brief メトリクスを登録
     * @param name メトリクス名
     * @param description 説明
     * @param type メトリクスタイプ
     */
    void registerMetric(const std::string& name, 
                       const std::string& description,
                       MetricType type);

    /**
     * @brief ラベルキーを生成
     * @param labels ラベル
     * @return ラベルキー
     */
    std::string generateLabelKey(const std::map<std::string, std::string>& labels);

    /**
     * @brief システムメトリクスを初期化
     */
    void initializeSystemMetrics();

    /**
     * @brief デバイスメトリクスを初期化
     */
    void initializeDeviceMetrics();

    /**
     * @brief OCPPメトリクスを初期化
     */
    void initializeOcppMetrics();

#ifdef __linux__
    /**
     * @brief Linuxシステムメトリクスを更新
     */
    void updateLinuxSystemMetrics();
#endif

    std::map<std::string, std::shared_ptr<MetricEntry>> metrics_;
    std::mutex metrics_mutex_;
    std::atomic<bool> running_;
    std::thread update_thread_;
    
    // システムメトリクス
    std::atomic<uint64_t> total_memory_bytes_;
    std::atomic<uint64_t> used_memory_bytes_;
    std::atomic<double> cpu_usage_percent_;
    std::atomic<uint64_t> uptime_seconds_;
    
    // デバイスメトリクス
    std::atomic<uint32_t> active_devices_;
    std::atomic<uint32_t> total_devices_;
    std::atomic<uint64_t> device_communication_errors_;
    
    // OCPPメトリクス
    std::atomic<uint64_t> ocpp_messages_sent_;
    std::atomic<uint64_t> ocpp_messages_received_;
    std::atomic<uint64_t> ocpp_errors_;
    std::atomic<double> average_response_time_ms_;
};

} // namespace common
} // namespace ocpp_gateway 