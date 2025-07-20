#include <gtest/gtest.h>
#include "ocpp_gateway/common/metrics_collector.h"
#include <thread>
#include <chrono>

using namespace ocpp_gateway::common;

class MetricsCollectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // シングルトンインスタンスを取得
        metrics_collector_ = &MetricsCollector::getInstance();
    }

    void TearDown() override {
        // テスト後にメトリクスをリセット
        if (metrics_collector_) {
            metrics_collector_->resetMetrics();
        }
    }

    MetricsCollector* metrics_collector_;
};

TEST_F(MetricsCollectorTest, SingletonTest) {
    // シングルトンパターンのテスト
    auto& instance1 = MetricsCollector::getInstance();
    auto& instance2 = MetricsCollector::getInstance();
    
    EXPECT_EQ(&instance1, &instance2);
    EXPECT_EQ(metrics_collector_, &instance1);
}

TEST_F(MetricsCollectorTest, CounterIncrementTest) {
    // カウンターメトリクスのテスト
    std::string metric_name = "test_counter";
    
    // 初回のカウンター作成
    metrics_collector_->incrementCounter(metric_name, 1.0);
    
    auto metric = metrics_collector_->getMetric(metric_name);
    ASSERT_NE(metric, nullptr);
    EXPECT_EQ(metric->type, MetricType::COUNTER);
    
    // 値の追加
    metrics_collector_->incrementCounter(metric_name, 2.0);
    
    // 値の確認（具体的な値はlockが必要だが、メトリクスが存在することを確認）
    auto all_metrics = metrics_collector_->getAllMetrics();
    EXPECT_TRUE(all_metrics.find(metric_name) != all_metrics.end());
}

TEST_F(MetricsCollectorTest, GaugeSetTest) {
    // ゲージメトリクスのテスト
    std::string metric_name = "test_gauge";
    double test_value = 42.5;
    
    metrics_collector_->setGauge(metric_name, test_value);
    
    auto metric = metrics_collector_->getMetric(metric_name);
    ASSERT_NE(metric, nullptr);
    EXPECT_EQ(metric->type, MetricType::GAUGE);
}

TEST_F(MetricsCollectorTest, HistogramRecordTest) {
    // ヒストグラムメトリクスのテスト
    std::string metric_name = "test_histogram";
    
    metrics_collector_->recordHistogram(metric_name, 10.0);
    metrics_collector_->recordHistogram(metric_name, 20.0);
    
    auto metric = metrics_collector_->getMetric(metric_name);
    ASSERT_NE(metric, nullptr);
    EXPECT_EQ(metric->type, MetricType::HISTOGRAM);
}

TEST_F(MetricsCollectorTest, SummaryRecordTest) {
    // サマリーメトリクスのテスト
    std::string metric_name = "test_summary";
    
    metrics_collector_->recordSummary(metric_name, 15.0);
    
    auto metric = metrics_collector_->getMetric(metric_name);
    ASSERT_NE(metric, nullptr);
    EXPECT_EQ(metric->type, MetricType::SUMMARY);
}

TEST_F(MetricsCollectorTest, LabelsTest) {
    // ラベル付きメトリクスのテスト
    std::string metric_name = "test_labeled_counter";
    std::map<std::string, std::string> labels = {
        {"device_id", "test_device"},
        {"status", "active"}
    };
    
    metrics_collector_->incrementCounter(metric_name, 1.0, labels);
    
    auto metric = metrics_collector_->getMetric(metric_name);
    ASSERT_NE(metric, nullptr);
    EXPECT_EQ(metric->type, MetricType::COUNTER);
}

TEST_F(MetricsCollectorTest, JsonExportTest) {
    // JSON形式でのエクスポートテスト
    metrics_collector_->setGauge("test_json_gauge", 123.45);
    
    std::string json_output = metrics_collector_->getMetricsAsJson();
    
    // 基本的なJSON構造の確認
    EXPECT_FALSE(json_output.empty());
    EXPECT_TRUE(json_output.find("test_json_gauge") != std::string::npos);
    EXPECT_TRUE(json_output.find("123.45") != std::string::npos);
}

TEST_F(MetricsCollectorTest, PrometheusExportTest) {
    // Prometheus形式でのエクスポートテスト
    metrics_collector_->incrementCounter("test_prometheus_counter", 5.0);
    
    std::string prometheus_output = metrics_collector_->getMetricsAsPrometheus();
    
    // 基本的なPrometheus形式の確認
    EXPECT_FALSE(prometheus_output.empty());
    EXPECT_TRUE(prometheus_output.find("test_prometheus_counter") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("# HELP") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("# TYPE") != std::string::npos);
}

TEST_F(MetricsCollectorTest, ResetMetricsTest) {
    // メトリクスリセットのテスト
    metrics_collector_->setGauge("test_reset_gauge", 100.0);
    
    auto metric_before = metrics_collector_->getMetric("test_reset_gauge");
    EXPECT_NE(metric_before, nullptr);
    
    // 特定のメトリクスをリセット
    metrics_collector_->resetMetrics("test_reset_gauge");
    
    // リセット後でもメトリクス自体は存在するが、値がクリアされる
    auto metric_after = metrics_collector_->getMetric("test_reset_gauge");
    EXPECT_NE(metric_after, nullptr);
}

TEST_F(MetricsCollectorTest, AllMetricsResetTest) {
    // 全メトリクスリセットのテスト
    metrics_collector_->setGauge("test_gauge_1", 1.0);
    metrics_collector_->setGauge("test_gauge_2", 2.0);
    
    auto all_metrics_before = metrics_collector_->getAllMetrics();
    EXPECT_FALSE(all_metrics_before.empty());
    
    // 全メトリクスをリセット
    metrics_collector_->resetMetrics();
    
    // メトリクス定義は残るが、値がクリアされる
    auto all_metrics_after = metrics_collector_->getAllMetrics();
    EXPECT_FALSE(all_metrics_after.empty());
}

TEST_F(MetricsCollectorTest, DeviceMetricsUpdateTest) {
    // デバイスメトリクス更新のテスト
    std::string device_id = "test_device_001";
    std::string status = "active";
    double response_time = 50.5;
    
    EXPECT_NO_THROW(
        metrics_collector_->updateDeviceMetrics(device_id, status, response_time)
    );
    
    // デバイス関連メトリクスが作成されることを確認
    auto device_status_metric = metrics_collector_->getMetric("device_status");
    EXPECT_NE(device_status_metric, nullptr);
    
    auto response_time_metric = metrics_collector_->getMetric("device_response_time_ms");
    EXPECT_NE(response_time_metric, nullptr);
}

TEST_F(MetricsCollectorTest, OcppMetricsUpdateTest) {
    // OCPPメトリクス更新のテスト
    std::string message_type = "BootNotification";
    bool success = true;
    double response_time = 25.3;
    
    EXPECT_NO_THROW(
        metrics_collector_->updateOcppMetrics(message_type, success, response_time)
    );
    
    // OCPP関連メトリクスが作成されることを確認
    auto sent_metric = metrics_collector_->getMetric("ocpp_messages_sent_total");
    EXPECT_NE(sent_metric, nullptr);
    
    auto response_time_metric = metrics_collector_->getMetric("ocpp_response_time_ms");
    EXPECT_NE(response_time_metric, nullptr);
} 