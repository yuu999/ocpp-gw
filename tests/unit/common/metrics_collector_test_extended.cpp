#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ocpp_gateway/common/metrics_collector.h"
#include "ocpp_gateway/common/language_manager.h"
#include <thread>
#include <chrono>
#include <json/json.h>

using namespace ocpp_gateway::common;
using namespace testing;

class MetricsCollectorExtendedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize language manager with test resources
        LanguageManager::getInstance().initialize("en", "resources/lang");
        
        // Get singleton instance of metrics collector
        metrics_collector_ = &MetricsCollector::getInstance();
        
        // Reset all metrics before each test
        metrics_collector_->resetMetrics();
    }

    void TearDown() override {
        // Reset all metrics after each test
        if (metrics_collector_) {
            metrics_collector_->resetMetrics();
        }
    }

    MetricsCollector* metrics_collector_;
};

// Test initialization and shutdown
TEST_F(MetricsCollectorExtendedTest, InitializeShutdownTest) {
    EXPECT_TRUE(metrics_collector_->initialize());
    
    // Check that system metrics are created
    auto system_uptime_metric = metrics_collector_->getMetric("system_uptime_seconds");
    EXPECT_NE(system_uptime_metric, nullptr);
    
    auto system_memory_total_metric = metrics_collector_->getMetric("system_memory_total_bytes");
    EXPECT_NE(system_memory_total_metric, nullptr);
    
    auto system_memory_used_metric = metrics_collector_->getMetric("system_memory_used_bytes");
    EXPECT_NE(system_memory_used_metric, nullptr);
    
    auto system_cpu_usage_metric = metrics_collector_->getMetric("system_cpu_usage_percent");
    EXPECT_NE(system_cpu_usage_metric, nullptr);
    
    // Check that device metrics are created
    auto device_status_metric = metrics_collector_->getMetric("device_status");
    EXPECT_NE(device_status_metric, nullptr);
    
    auto device_response_time_metric = metrics_collector_->getMetric("device_response_time_ms");
    EXPECT_NE(device_response_time_metric, nullptr);
    
    auto device_communication_errors_metric = metrics_collector_->getMetric("device_communication_errors_total");
    EXPECT_NE(device_communication_errors_metric, nullptr);
    
    // Check that OCPP metrics are created
    auto ocpp_messages_sent_metric = metrics_collector_->getMetric("ocpp_messages_sent_total");
    EXPECT_NE(ocpp_messages_sent_metric, nullptr);
    
    auto ocpp_messages_received_metric = metrics_collector_->getMetric("ocpp_messages_received_total");
    EXPECT_NE(ocpp_messages_received_metric, nullptr);
    
    auto ocpp_errors_metric = metrics_collector_->getMetric("ocpp_errors_total");
    EXPECT_NE(ocpp_errors_metric, nullptr);
    
    auto ocpp_response_time_metric = metrics_collector_->getMetric("ocpp_response_time_ms");
    EXPECT_NE(ocpp_response_time_metric, nullptr);
    
    // Shutdown
    metrics_collector_->shutdown();
}

// Test system metrics update
TEST_F(MetricsCollectorExtendedTest, SystemMetricsUpdateTest) {
    EXPECT_TRUE(metrics_collector_->initialize());
    
    // Update system metrics
    metrics_collector_->updateSystemMetrics();
    
    // Check that system metrics are updated
    auto system_uptime_metric = metrics_collector_->getMetric("system_uptime_seconds");
    ASSERT_NE(system_uptime_metric, nullptr);
    EXPECT_EQ(system_uptime_metric->type, MetricType::GAUGE);
    
    auto system_memory_total_metric = metrics_collector_->getMetric("system_memory_total_bytes");
    ASSERT_NE(system_memory_total_metric, nullptr);
    EXPECT_EQ(system_memory_total_metric->type, MetricType::GAUGE);
    
    auto system_memory_used_metric = metrics_collector_->getMetric("system_memory_used_bytes");
    ASSERT_NE(system_memory_used_metric, nullptr);
    EXPECT_EQ(system_memory_used_metric->type, MetricType::GAUGE);
    
    auto system_cpu_usage_metric = metrics_collector_->getMetric("system_cpu_usage_percent");
    ASSERT_NE(system_cpu_usage_metric, nullptr);
    EXPECT_EQ(system_cpu_usage_metric->type, MetricType::GAUGE);
    
    // Shutdown
    metrics_collector_->shutdown();
}

// Test device metrics update
TEST_F(MetricsCollectorExtendedTest, DeviceMetricsUpdateTest) {
    EXPECT_TRUE(metrics_collector_->initialize());
    
    // Update device metrics for active device
    std::string device_id = "test_device_001";
    std::string status = "active";
    double response_time = 50.5;
    
    metrics_collector_->updateDeviceMetrics(device_id, status, response_time);
    
    // Check device status metric
    auto device_status_metric = metrics_collector_->getMetric("device_status");
    ASSERT_NE(device_status_metric, nullptr);
    EXPECT_EQ(device_status_metric->type, MetricType::GAUGE);
    
    // Check device response time metric
    auto device_response_time_metric = metrics_collector_->getMetric("device_response_time_ms");
    ASSERT_NE(device_response_time_metric, nullptr);
    EXPECT_EQ(device_response_time_metric->type, MetricType::HISTOGRAM);
    
    // Update device metrics for error device
    std::string error_device_id = "test_device_002";
    std::string error_status = "error";
    double error_response_time = 500.0;
    
    metrics_collector_->updateDeviceMetrics(error_device_id, error_status, error_response_time);
    
    // Check device communication errors metric
    auto device_communication_errors_metric = metrics_collector_->getMetric("device_communication_errors_total");
    ASSERT_NE(device_communication_errors_metric, nullptr);
    EXPECT_EQ(device_communication_errors_metric->type, MetricType::COUNTER);
    
    // Shutdown
    metrics_collector_->shutdown();
}

// Test OCPP metrics update
TEST_F(MetricsCollectorExtendedTest, OcppMetricsUpdateTest) {
    EXPECT_TRUE(metrics_collector_->initialize());
    
    // Update OCPP metrics for successful message
    std::string message_type = "BootNotification";
    bool success = true;
    double response_time = 25.3;
    
    metrics_collector_->updateOcppMetrics(message_type, success, response_time);
    
    // Check OCPP messages sent metric
    auto ocpp_messages_sent_metric = metrics_collector_->getMetric("ocpp_messages_sent_total");
    ASSERT_NE(ocpp_messages_sent_metric, nullptr);
    EXPECT_EQ(ocpp_messages_sent_metric->type, MetricType::COUNTER);
    
    // Check OCPP response time metric
    auto ocpp_response_time_metric = metrics_collector_->getMetric("ocpp_response_time_ms");
    ASSERT_NE(ocpp_response_time_metric, nullptr);
    EXPECT_EQ(ocpp_response_time_metric->type, MetricType::HISTOGRAM);
    
    // Update OCPP metrics for error message
    std::string error_message_type = "Heartbeat";
    bool error_success = false;
    double error_response_time = 100.0;
    
    metrics_collector_->updateOcppMetrics(error_message_type, error_success, error_response_time);
    
    // Check OCPP errors metric
    auto ocpp_errors_metric = metrics_collector_->getMetric("ocpp_errors_total");
    ASSERT_NE(ocpp_errors_metric, nullptr);
    EXPECT_EQ(ocpp_errors_metric->type, MetricType::COUNTER);
    
    // Check OCPP average response time metric
    auto ocpp_average_response_time_metric = metrics_collector_->getMetric("ocpp_average_response_time_ms");
    ASSERT_NE(ocpp_average_response_time_metric, nullptr);
    EXPECT_EQ(ocpp_average_response_time_metric->type, MetricType::GAUGE);
    
    // Shutdown
    metrics_collector_->shutdown();
}

// Test metrics with labels
TEST_F(MetricsCollectorExtendedTest, MetricsWithLabelsTest) {
    EXPECT_TRUE(metrics_collector_->initialize());
    
    // Create metrics with labels
    std::string metric_name = "test_labeled_metric";
    std::map<std::string, std::string> labels1 = {
        {"device_id", "device1"},
        {"status", "active"}
    };
    std::map<std::string, std::string> labels2 = {
        {"device_id", "device2"},
        {"status", "error"}
    };
    
    // Set gauge values with different labels
    metrics_collector_->setGauge(metric_name, 100.0, labels1);
    metrics_collector_->setGauge(metric_name, 200.0, labels2);
    
    // Get the metric
    auto metric = metrics_collector_->getMetric(metric_name);
    ASSERT_NE(metric, nullptr);
    EXPECT_EQ(metric->type, MetricType::GAUGE);
    
    // Check that there are two values with different labels
    EXPECT_EQ(metric->values.size(), 2);
    
    // Check JSON export
    std::string json_output = metrics_collector_->getMetricsAsJson();
    
    // Parse JSON
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(json_output, root));
    
    // Find our labeled metric in the JSON
    bool found_metric = false;
    for (const auto& metric_json : root["metrics"]) {
        if (metric_json["name"].asString() == metric_name) {
            found_metric = true;
            EXPECT_EQ(metric_json["values"].size(), 2);
            break;
        }
    }
    EXPECT_TRUE(found_metric);
    
    // Check Prometheus export
    std::string prometheus_output = metrics_collector_->getMetricsAsPrometheus();
    
    // Check that the metric name and labels appear in the Prometheus output
    EXPECT_TRUE(prometheus_output.find(metric_name) != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("device_id=\"device1\"") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("device_id=\"device2\"") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("status=\"active\"") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("status=\"error\"") != std::string::npos);
    
    // Shutdown
    metrics_collector_->shutdown();
}

// Test metrics reset
TEST_F(MetricsCollectorExtendedTest, MetricsResetTest) {
    EXPECT_TRUE(metrics_collector_->initialize());
    
    // Create some test metrics
    metrics_collector_->setGauge("test_gauge_1", 100.0);
    metrics_collector_->setGauge("test_gauge_2", 200.0);
    metrics_collector_->incrementCounter("test_counter", 50.0);
    
    // Check that metrics exist
    auto gauge1 = metrics_collector_->getMetric("test_gauge_1");
    ASSERT_NE(gauge1, nullptr);
    EXPECT_FALSE(gauge1->values.empty());
    
    auto gauge2 = metrics_collector_->getMetric("test_gauge_2");
    ASSERT_NE(gauge2, nullptr);
    EXPECT_FALSE(gauge2->values.empty());
    
    auto counter = metrics_collector_->getMetric("test_counter");
    ASSERT_NE(counter, nullptr);
    EXPECT_FALSE(counter->values.empty());
    
    // Reset a specific metric
    metrics_collector_->resetMetrics("test_gauge_1");
    
    // Check that only the specified metric was reset
    gauge1 = metrics_collector_->getMetric("test_gauge_1");
    ASSERT_NE(gauge1, nullptr);
    EXPECT_TRUE(gauge1->values.empty());
    
    gauge2 = metrics_collector_->getMetric("test_gauge_2");
    ASSERT_NE(gauge2, nullptr);
    EXPECT_FALSE(gauge2->values.empty());
    
    counter = metrics_collector_->getMetric("test_counter");
    ASSERT_NE(counter, nullptr);
    EXPECT_FALSE(counter->values.empty());
    
    // Reset all metrics
    metrics_collector_->resetMetrics();
    
    // Check that all metrics were reset
    gauge1 = metrics_collector_->getMetric("test_gauge_1");
    ASSERT_NE(gauge1, nullptr);
    EXPECT_TRUE(gauge1->values.empty());
    
    gauge2 = metrics_collector_->getMetric("test_gauge_2");
    ASSERT_NE(gauge2, nullptr);
    EXPECT_TRUE(gauge2->values.empty());
    
    counter = metrics_collector_->getMetric("test_counter");
    ASSERT_NE(counter, nullptr);
    EXPECT_TRUE(counter->values.empty());
    
    // Shutdown
    metrics_collector_->shutdown();
}

// Test JSON export format
TEST_F(MetricsCollectorExtendedTest, JsonExportFormatTest) {
    EXPECT_TRUE(metrics_collector_->initialize());
    
    // Create some test metrics
    metrics_collector_->setGauge("test_gauge", 123.45);
    metrics_collector_->incrementCounter("test_counter", 50.0);
    metrics_collector_->recordHistogram("test_histogram", 75.0);
    
    // Get JSON export
    std::string json_output = metrics_collector_->getMetricsAsJson();
    
    // Parse JSON
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(json_output, root));
    
    // Check JSON structure
    EXPECT_TRUE(root.isMember("timestamp"));
    EXPECT_TRUE(root.isMember("metrics"));
    EXPECT_TRUE(root["metrics"].isArray());
    
    // Check that our test metrics are in the JSON
    bool found_gauge = false;
    bool found_counter = false;
    bool found_histogram = false;
    
    for (const auto& metric_json : root["metrics"]) {
        EXPECT_TRUE(metric_json.isMember("name"));
        EXPECT_TRUE(metric_json.isMember("description"));
        EXPECT_TRUE(metric_json.isMember("type"));
        EXPECT_TRUE(metric_json.isMember("values"));
        EXPECT_TRUE(metric_json["values"].isArray());
        
        std::string name = metric_json["name"].asString();
        if (name == "test_gauge") {
            found_gauge = true;
            EXPECT_EQ(metric_json["type"].asInt(), static_cast<int>(MetricType::GAUGE));
            EXPECT_FALSE(metric_json["values"].empty());
            EXPECT_DOUBLE_EQ(metric_json["values"][0]["value"].asDouble(), 123.45);
        } else if (name == "test_counter") {
            found_counter = true;
            EXPECT_EQ(metric_json["type"].asInt(), static_cast<int>(MetricType::COUNTER));
            EXPECT_FALSE(metric_json["values"].empty());
            EXPECT_DOUBLE_EQ(metric_json["values"][0]["value"].asDouble(), 50.0);
        } else if (name == "test_histogram") {
            found_histogram = true;
            EXPECT_EQ(metric_json["type"].asInt(), static_cast<int>(MetricType::HISTOGRAM));
            EXPECT_FALSE(metric_json["values"].empty());
            EXPECT_DOUBLE_EQ(metric_json["values"][0]["value"].asDouble(), 75.0);
        }
    }
    
    EXPECT_TRUE(found_gauge);
    EXPECT_TRUE(found_counter);
    EXPECT_TRUE(found_histogram);
    
    // Shutdown
    metrics_collector_->shutdown();
}

// Test Prometheus export format
TEST_F(MetricsCollectorExtendedTest, PrometheusExportFormatTest) {
    EXPECT_TRUE(metrics_collector_->initialize());
    
    // Create some test metrics
    metrics_collector_->setGauge("test_gauge", 123.45);
    metrics_collector_->incrementCounter("test_counter", 50.0);
    metrics_collector_->recordHistogram("test_histogram", 75.0);
    
    // Add a metric with labels
    std::map<std::string, std::string> labels = {
        {"device_id", "test_device"},
        {"status", "active"}
    };
    metrics_collector_->setGauge("test_labeled_gauge", 99.9, labels);
    
    // Get Prometheus export
    std::string prometheus_output = metrics_collector_->getMetricsAsPrometheus();
    
    // Check Prometheus format
    EXPECT_TRUE(prometheus_output.find("# HELP test_gauge") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("# TYPE test_gauge gauge") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("test_gauge ") != std::string::npos);
    
    EXPECT_TRUE(prometheus_output.find("# HELP test_counter") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("# TYPE test_counter counter") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("test_counter ") != std::string::npos);
    
    EXPECT_TRUE(prometheus_output.find("# HELP test_histogram") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("# TYPE test_histogram histogram") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("test_histogram ") != std::string::npos);
    
    // Check labeled metric
    EXPECT_TRUE(prometheus_output.find("# HELP test_labeled_gauge") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("# TYPE test_labeled_gauge gauge") != std::string::npos);
    EXPECT_TRUE(prometheus_output.find("test_labeled_gauge{device_id=\"test_device\",status=\"active\"}") != std::string::npos);
    
    // Shutdown
    metrics_collector_->shutdown();
}

// Test internationalization support
TEST_F(MetricsCollectorExtendedTest, InternationalizationTest) {
    // Set language to English
    LanguageManager::getInstance().setLanguage("en");
    
    EXPECT_TRUE(metrics_collector_->initialize());
    
    // Check that metrics descriptions are in English
    auto system_uptime_metric = metrics_collector_->getMetric("system_uptime_seconds");
    ASSERT_NE(system_uptime_metric, nullptr);
    EXPECT_TRUE(system_uptime_metric->description.find("System uptime") != std::string::npos ||
                system_uptime_metric->description.find("seconds") != std::string::npos);
    
    // Set language to Japanese
    LanguageManager::getInstance().setLanguage("ja");
    
    // Reset and reinitialize metrics
    metrics_collector_->shutdown();
    metrics_collector_->initialize();
    
    // Check that metrics descriptions are in Japanese
    system_uptime_metric = metrics_collector_->getMetric("system_uptime_seconds");
    ASSERT_NE(system_uptime_metric, nullptr);
    EXPECT_TRUE(system_uptime_metric->description.find("ƒVƒXƒeƒ€‰Ò“­ŽžŠÔ") != std::string::npos ||
                system_uptime_metric->description.find("•b") != std::string::npos);
    
    // Reset language to English for other tests
    LanguageManager::getInstance().setLanguage("en");
    
    // Shutdown
    metrics_collector_->shutdown();
}
