#include "ocpp_gateway/common/metrics_collector.h"
#include "ocpp_gateway/common/logger.h"
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

#ifdef __linux__
#include <sys/resource.h>
#include <sys/times.h>
#include <unistd.h>
#endif

namespace ocpp_gateway {
namespace common {

MetricsCollector& MetricsCollector::getInstance() {
    static MetricsCollector instance;
    return instance;
}

MetricsCollector::MetricsCollector() 
    : running_(false),
      total_memory_bytes_(0),
      used_memory_bytes_(0),
      cpu_usage_percent_(0.0),
      uptime_seconds_(0),
      active_devices_(0),
      total_devices_(0),
      device_communication_errors_(0),
      ocpp_messages_sent_(0),
      ocpp_messages_received_(0),
      ocpp_errors_(0),
      average_response_time_ms_(0.0) {
}

bool MetricsCollector::initialize() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (running_.load()) {
        LOG_WARN("メトリクス収集は既に実行中です");
        return true;
    }
    
    try {
        // メトリクスの初期化
        initializeSystemMetrics();
        initializeDeviceMetrics();
        initializeOcppMetrics();
        
        running_ = true;
        
        // 更新スレッドを開始
        update_thread_ = std::thread([this]() {
            LOG_INFO("メトリクス更新スレッドを開始しました");
            
            while (running_.load()) {
                updateSystemMetrics();
                
                // 5秒間隔で更新
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
            
            LOG_INFO("メトリクス更新スレッドを終了しました");
        });
        
        LOG_INFO("メトリクス収集を初期化しました");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("メトリクス収集の初期化に失敗しました: {}", e.what());
        running_ = false;
        return false;
    }
}

void MetricsCollector::shutdown() {
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    
    if (update_thread_.joinable()) {
        update_thread_.join();
    }
    
    LOG_INFO("メトリクス収集を停止しました");
}

void MetricsCollector::incrementCounter(const std::string& name, double value, 
                                       const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it == metrics_.end()) {
        registerMetric(name, "Auto-registered counter", MetricType::COUNTER);
        it = metrics_.find(name);
    }
    
    if (it != metrics_.end() && it->second->type == MetricType::COUNTER) {
        std::lock_guard<std::mutex> metric_lock(it->second->mutex);
        std::string label_key = generateLabelKey(labels);
        it->second->values[label_key].value += value;
        it->second->values[label_key].timestamp = std::chrono::system_clock::now();
        it->second->values[label_key].labels = labels;
    }
}

void MetricsCollector::setGauge(const std::string& name, double value,
                               const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it == metrics_.end()) {
        registerMetric(name, "Auto-registered gauge", MetricType::GAUGE);
        it = metrics_.find(name);
    }
    
    if (it != metrics_.end() && it->second->type == MetricType::GAUGE) {
        std::lock_guard<std::mutex> metric_lock(it->second->mutex);
        std::string label_key = generateLabelKey(labels);
        it->second->values[label_key].value = value;
        it->second->values[label_key].timestamp = std::chrono::system_clock::now();
        it->second->values[label_key].labels = labels;
    }
}

void MetricsCollector::recordHistogram(const std::string& name, double value,
                                      const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it == metrics_.end()) {
        registerMetric(name, "Auto-registered histogram", MetricType::HISTOGRAM);
        it = metrics_.find(name);
    }
    
    if (it != metrics_.end() && it->second->type == MetricType::HISTOGRAM) {
        std::lock_guard<std::mutex> metric_lock(it->second->mutex);
        std::string label_key = generateLabelKey(labels);
        
        // ヒストグラムの場合、値の分布を記録（簡易実装）
        auto& metric_value = it->second->values[label_key];
        if (metric_value.value == 0.0) {
            metric_value.value = value;
        } else {
            // 移動平均を計算
            metric_value.value = (metric_value.value + value) / 2.0;
        }
        metric_value.timestamp = std::chrono::system_clock::now();
        metric_value.labels = labels;
    }
}

void MetricsCollector::recordSummary(const std::string& name, double value,
                                    const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it == metrics_.end()) {
        registerMetric(name, "Auto-registered summary", MetricType::SUMMARY);
        it = metrics_.find(name);
    }
    
    if (it != metrics_.end() && it->second->type == MetricType::SUMMARY) {
        std::lock_guard<std::mutex> metric_lock(it->second->mutex);
        std::string label_key = generateLabelKey(labels);
        
        // サマリーの場合、統計情報を計算（簡易実装）
        auto& metric_value = it->second->values[label_key];
        if (metric_value.value == 0.0) {
            metric_value.value = value;
        } else {
            // 移動平均を計算
            metric_value.value = (metric_value.value + value) / 2.0;
        }
        metric_value.timestamp = std::chrono::system_clock::now();
        metric_value.labels = labels;
    }
}

std::shared_ptr<MetricEntry> MetricsCollector::getMetric(const std::string& name) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::map<std::string, std::shared_ptr<MetricEntry>> MetricsCollector::getAllMetrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

std::string MetricsCollector::getMetricsAsJson() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    Json::Value root;
    root["timestamp"] = static_cast<Json::Int64>(std::time(nullptr));
    root["metrics"] = Json::Value(Json::arrayValue);
    
    for (const auto& metric_pair : metrics_) {
        const auto& metric = metric_pair.second;
        std::lock_guard<std::mutex> metric_lock(metric->mutex);
        
        Json::Value metric_json;
        metric_json["name"] = metric->name;
        metric_json["description"] = metric->description;
        metric_json["type"] = static_cast<int>(metric->type);
        
        Json::Value values(Json::arrayValue);
        for (const auto& value_pair : metric->values) {
            Json::Value value_json;
            value_json["value"] = value_pair.second.value;
            value_json["timestamp"] = static_cast<Json::Int64>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    value_pair.second.timestamp.time_since_epoch()).count());
            
            Json::Value labels_json;
            for (const auto& label : value_pair.second.labels) {
                labels_json[label.first] = label.second;
            }
            value_json["labels"] = labels_json;
            
            values.append(value_json);
        }
        metric_json["values"] = values;
        
        root["metrics"].append(metric_json);
    }
    
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    return Json::writeString(builder, root);
}

std::string MetricsCollector::getMetricsAsPrometheus() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    std::ostringstream oss;
    
    for (const auto& metric_pair : metrics_) {
        const auto& metric = metric_pair.second;
        std::lock_guard<std::mutex> metric_lock(metric->mutex);
        
        // メトリクスのヘルプとタイプを出力
        oss << "# HELP " << metric->name << " " << metric->description << "\n";
        
        std::string type_str;
        switch (metric->type) {
            case MetricType::COUNTER: type_str = "counter"; break;
            case MetricType::GAUGE: type_str = "gauge"; break;
            case MetricType::HISTOGRAM: type_str = "histogram"; break;
            case MetricType::SUMMARY: type_str = "summary"; break;
        }
        oss << "# TYPE " << metric->name << " " << type_str << "\n";
        
        // 値を出力
        for (const auto& value_pair : metric->values) {
            oss << metric->name;
            
            // ラベルを出力
            if (!value_pair.second.labels.empty()) {
                oss << "{";
                bool first = true;
                for (const auto& label : value_pair.second.labels) {
                    if (!first) oss << ",";
                    oss << label.first << "=\"" << label.second << "\"";
                    first = false;
                }
                oss << "}";
            }
            
            oss << " " << std::fixed << std::setprecision(6) << value_pair.second.value;
            oss << " " << std::chrono::duration_cast<std::chrono::milliseconds>(
                value_pair.second.timestamp.time_since_epoch()).count();
            oss << "\n";
        }
        oss << "\n";
    }
    
    return oss.str();
}

void MetricsCollector::resetMetrics(const std::string& name) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (name.empty()) {
        // 全メトリクスをリセット
        for (auto& metric_pair : metrics_) {
            std::lock_guard<std::mutex> metric_lock(metric_pair.second->mutex);
            metric_pair.second->values.clear();
        }
        LOG_INFO("すべてのメトリクスをリセットしました");
    } else {
        // 指定されたメトリクスのみリセット
        auto it = metrics_.find(name);
        if (it != metrics_.end()) {
            std::lock_guard<std::mutex> metric_lock(it->second->mutex);
            it->second->values.clear();
            LOG_INFO("メトリクス '{}' をリセットしました", name);
        }
    }
}

void MetricsCollector::updateSystemMetrics() {
    static auto start_time = std::chrono::steady_clock::now();
    
    // 稼働時間を更新
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
    uptime_seconds_ = uptime.count();
    setGauge("system_uptime_seconds", static_cast<double>(uptime_seconds_.load()));
    
#ifdef __linux__
    // Linuxでのシステムメトリクス更新
    updateLinuxSystemMetrics();
#else
    // 他のOSでの簡易実装
    setGauge("system_memory_total_bytes", 1024.0 * 1024 * 1024); // 1GB（ダミー）
    setGauge("system_memory_used_bytes", 512.0 * 1024 * 1024);   // 512MB（ダミー）
    setGauge("system_cpu_usage_percent", 25.0);                  // 25%（ダミー）
#endif
}

void MetricsCollector::updateDeviceMetrics(const std::string& device_id, 
                                          const std::string& status,
                                          double response_time) {
    std::map<std::string, std::string> labels = {{"device_id", device_id}};
    
    // デバイス状態メトリクス
    setGauge("device_status", status == "active" ? 1.0 : 0.0, labels);
    
    // 応答時間メトリクス
    recordHistogram("device_response_time_ms", response_time, labels);
    
    // デバイス通信エラー
    if (status == "error") {
        incrementCounter("device_communication_errors_total", 1.0, labels);
        device_communication_errors_++;
    }
    
    // 統計情報を更新
    setGauge("device_communication_errors_total", static_cast<double>(device_communication_errors_.load()));
}

void MetricsCollector::updateOcppMetrics(const std::string& message_type,
                                        bool success,
                                        double response_time) {
    std::map<std::string, std::string> labels = {
        {"message_type", message_type},
        {"status", success ? "success" : "error"}
    };
    
    // メッセージ数をカウント
    if (success) {
        incrementCounter("ocpp_messages_sent_total", 1.0, {{"message_type", message_type}});
        ocpp_messages_sent_++;
    } else {
        incrementCounter("ocpp_errors_total", 1.0, {{"message_type", message_type}});
        ocpp_errors_++;
    }
    
    // 応答時間を記録
    recordHistogram("ocpp_response_time_ms", response_time, labels);
    
    // 平均応答時間を更新（簡易計算）
    double current_avg = average_response_time_ms_.load();
    if (current_avg == 0.0) {
        average_response_time_ms_ = response_time;
    } else {
        average_response_time_ms_ = (current_avg + response_time) / 2.0;
    }
    
    // 統計情報を更新
    setGauge("ocpp_messages_sent_total", static_cast<double>(ocpp_messages_sent_.load()));
    setGauge("ocpp_errors_total", static_cast<double>(ocpp_errors_.load()));
    setGauge("ocpp_average_response_time_ms", average_response_time_ms_.load());
}

void MetricsCollector::registerMetric(const std::string& name, 
                                     const std::string& description,
                                     MetricType type) {
    auto metric = std::make_shared<MetricEntry>(name, description, type);
    metrics_[name] = metric;
}

std::string MetricsCollector::generateLabelKey(const std::map<std::string, std::string>& labels) {
    if (labels.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    bool first = true;
    for (const auto& label : labels) {
        if (!first) oss << ",";
        oss << label.first << "=" << label.second;
        first = false;
    }
    return oss.str();
}

void MetricsCollector::initializeSystemMetrics() {
    registerMetric("system_uptime_seconds", "システム稼働時間（秒）", MetricType::GAUGE);
    registerMetric("system_memory_total_bytes", "総メモリ量（バイト）", MetricType::GAUGE);
    registerMetric("system_memory_used_bytes", "使用メモリ量（バイト）", MetricType::GAUGE);
    registerMetric("system_cpu_usage_percent", "CPU使用率（%）", MetricType::GAUGE);
}

void MetricsCollector::initializeDeviceMetrics() {
    registerMetric("device_status", "デバイス状態（1=アクティブ, 0=非アクティブ）", MetricType::GAUGE);
    registerMetric("device_response_time_ms", "デバイス応答時間（ミリ秒）", MetricType::HISTOGRAM);
    registerMetric("device_communication_errors_total", "デバイス通信エラー総数", MetricType::COUNTER);
    registerMetric("devices_active_total", "アクティブデバイス数", MetricType::GAUGE);
    registerMetric("devices_total", "総デバイス数", MetricType::GAUGE);
}

void MetricsCollector::initializeOcppMetrics() {
    registerMetric("ocpp_messages_sent_total", "OCPP送信メッセージ総数", MetricType::COUNTER);
    registerMetric("ocpp_messages_received_total", "OCPP受信メッセージ総数", MetricType::COUNTER);
    registerMetric("ocpp_errors_total", "OCPPエラー総数", MetricType::COUNTER);
    registerMetric("ocpp_response_time_ms", "OCPP応答時間（ミリ秒）", MetricType::HISTOGRAM);
    registerMetric("ocpp_average_response_time_ms", "OCPP平均応答時間（ミリ秒）", MetricType::GAUGE);
}

#ifdef __linux__
void MetricsCollector::updateLinuxSystemMetrics() {
    // メモリ情報を読み取り
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                std::istringstream iss(line);
                std::string key, unit;
                uint64_t value;
                iss >> key >> value >> unit;
                total_memory_bytes_ = value * 1024; // kBからBytesに変換
                setGauge("system_memory_total_bytes", static_cast<double>(total_memory_bytes_.load()));
            } else if (line.find("MemAvailable:") == 0) {
                std::istringstream iss(line);
                std::string key, unit;
                uint64_t available;
                iss >> key >> available >> unit;
                used_memory_bytes_ = total_memory_bytes_.load() - (available * 1024);
                setGauge("system_memory_used_bytes", static_cast<double>(used_memory_bytes_.load()));
            }
        }
    }
    
    // CPU使用率を計算（簡易実装）
    static std::chrono::steady_clock::time_point last_cpu_time = std::chrono::steady_clock::now();
    static struct tms last_tms;
    
    auto now = std::chrono::steady_clock::now();
    struct tms current_tms;
    times(&current_tms);
    
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cpu_time).count() >= 1) {
        clock_t total_time = (current_tms.tms_utime + current_tms.tms_stime) - 
                            (last_tms.tms_utime + last_tms.tms_stime);
        clock_t elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_cpu_time).count() * CLOCKS_PER_SEC / 1000;
        
        if (elapsed_time > 0) {
            double cpu_usage = (static_cast<double>(total_time) / elapsed_time) * 100.0;
            cpu_usage_percent_ = cpu_usage;
            setGauge("system_cpu_usage_percent", cpu_usage);
        }
        
        last_cpu_time = now;
        last_tms = current_tms;
    }
}
#endif

} // namespace common
} // namespace ocpp_gateway 