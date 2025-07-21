#include "ocpp_gateway/common/prometheus_exporter.h"
#include "ocpp_gateway/common/metrics_collector.h"
#include "ocpp_gateway/common/logger.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <regex>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

namespace ocpp_gateway {
namespace common {

PrometheusExporter::PrometheusExporter(int port, const std::string& bind_address,
                                      const std::string& metrics_path)
    : port_(port), bind_address_(bind_address), metrics_path_(metrics_path),
      running_(false), health_check_enabled_(true),
      global_labels_(), metrics_filter_(),
      metrics_collector_(MetricsCollector::getInstance()),
      requests_total_(0), requests_errors_(0) {
    
    // グローバルラベルのデフォルト設定
    global_labels_["instance"] = "ocpp_gateway";
    global_labels_["version"] = "1.0.0";
}

PrometheusExporter::~PrometheusExporter() {
    stop();
}

bool PrometheusExporter::start() {
    if (running_.load()) {
        LOG_WARN("Prometheusエクスポーターは既に実行中です");
        return true;
    }

    try {
        running_ = true;
        server_thread_ = std::thread(&PrometheusExporter::runServer, this);
        
        // サーバー開始まで少し待機
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        LOG_INFO("Prometheusエクスポーターを開始しました: {}:{}{}", 
                bind_address_, port_, metrics_path_);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Prometheusエクスポーターの開始に失敗しました: {}", e.what());
        running_ = false;
        return false;
    }
}

void PrometheusExporter::stop() {
    if (!running_.load()) {
        return;
    }

    running_ = false;
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    LOG_INFO("Prometheusエクスポーターを停止しました");
}

bool PrometheusExporter::isRunning() const {
    return running_.load();
}

void PrometheusExporter::enableHealthCheck(bool enabled) {
    health_check_enabled_ = enabled;
    LOG_INFO("Prometheusヘルスチェックを{}しました", enabled ? "有効化" : "無効化");
}

void PrometheusExporter::setGlobalLabels(const std::map<std::string, std::string>& labels) {
    global_labels_ = labels;
    LOG_INFO("Prometheusグローバルラベルを設定しました");
}

void PrometheusExporter::setMetricsFilter(std::function<bool(const std::string&)> filter_func) {
    metrics_filter_ = filter_func;
    LOG_INFO("Prometheusメトリクスフィルターを設定しました");
}

void PrometheusExporter::runServer() {
    try {
        auto const address = net::ip::make_address(bind_address_);
        auto const port = static_cast<unsigned short>(port_);

        net::io_context ioc{1};
        net::ip::tcp::acceptor acceptor{ioc, {address, port}};
        
        LOG_INFO("Prometheusエクスポーターサーバーを開始: {}:{}", bind_address_, port);

        while (running_.load()) {
            beast::tcp_stream stream{ioc};
            
            // 接続を受け入れ
            try {
                acceptor.accept(stream.socket());
            } catch (const std::exception& e) {
                if (running_.load()) {
                    LOG_ERROR("Prometheus接続受け入れエラー: {}", e.what());
                }
                continue;
            }

            // リクエストを読み取り
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            
            try {
                http::read(stream, buffer, req);
            } catch (const std::exception& e) {
                LOG_ERROR("Prometheusリクエスト読み取りエラー: {}", e.what());
                requests_errors_++;
                continue;
            }

            requests_total_++;

            // リクエストを処理
            std::string response_content = handleRequest(
                std::string(req.target()),
                std::string(req.method_string())
            );

            // HTTPレスポンスを作成
            http::response<http::string_body> res;
            
            if (response_content.find("HTTP/") == 0) {
                // 完全なHTTPレスポンスが返された場合
                try {
                    // 簡易的なHTTPレスポンス送信
                    stream.socket().send(net::buffer(response_content));
                } catch (const std::exception& e) {
                    LOG_ERROR("Prometheusレスポンス送信エラー: {}", e.what());
                    requests_errors_++;
                }
            } else {
                // レスポンスボディのみの場合は、HTTPヘッダーを追加
                res.version(req.version());
                res.result(http::status::ok);
                res.set(http::field::server, "OCPP Gateway Prometheus Exporter");
                res.set(http::field::content_type, "text/plain; version=0.0.4; charset=utf-8");
                res.body() = response_content;
                res.prepare_payload();

                try {
                    http::write(stream, res);
                } catch (const std::exception& e) {
                    LOG_ERROR("Prometheusレスポンス送信エラー: {}", e.what());
                    requests_errors_++;
                }
            }

            // 接続を閉じる
            beast::error_code ec;
            stream.socket().shutdown(net::ip::tcp::socket::shutdown_send, ec);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Prometheusエクスポーターサーバーエラー: {}", e.what());
    }
}

std::string PrometheusExporter::handleRequest(const std::string& target, const std::string& method) {
    try {
        // GETリクエストのみ対応
        if (method != "GET") {
            return createErrorResponse(405, "Method Not Allowed");
        }

        // メトリクスエンドポイント
        if (target == metrics_path_) {
            return generatePrometheusMetrics();
        }
        
        // ヘルスチェックエンドポイント
        if (health_check_enabled_ && (target == "/health" || target == "/-/healthy")) {
            return generateHealthResponse();
        }
        
        // ルートパスの場合は簡易的な情報ページを表示
        if (target == "/") {
            std::ostringstream response;
            response << "<html><head><title>OCPP Gateway Prometheus Exporter</title></head><body>\n";
            response << "<h1>OCPP Gateway Prometheus Exporter</h1>\n";
            response << "<p><a href=\"" << metrics_path_ << "\">Metrics</a></p>\n";
            if (health_check_enabled_) {
                response << "<p><a href=\"/health\">Health</a></p>\n";
            }
            response << "</body></html>\n";
            return createHttpResponse(200, response.str(), "text/html");
        }
        
        return createErrorResponse(404, "Not Found");
    } catch (const std::exception& e) {
        LOG_ERROR("Prometheusリクエスト処理エラー: {}", e.what());
        requests_errors_++;
        return createErrorResponse(500, "Internal Server Error");
    }
}

std::string PrometheusExporter::generatePrometheusMetrics() {
    try {
        std::ostringstream metrics;
        
        // エクスポーター自体のメトリクスを追加
        metrics << "# HELP prometheus_exporter_requests_total Total number of requests to the exporter\n";
        metrics << "# TYPE prometheus_exporter_requests_total counter\n";
        metrics << "prometheus_exporter_requests_total" << getGlobalLabelsString();
        metrics << " " << requests_total_.load() << "\n\n";
        
        metrics << "# HELP prometheus_exporter_request_errors_total Total number of request errors\n";
        metrics << "# TYPE prometheus_exporter_request_errors_total counter\n";
        metrics << "prometheus_exporter_request_errors_total" << getGlobalLabelsString();
        metrics << " " << requests_errors_.load() << "\n\n";

        // メトリクス収集システムからメトリクスを取得
        auto all_metrics = metrics_collector_.getAllMetrics();
        
        for (const auto& metric_pair : all_metrics) {
            const auto& metric_name = metric_pair.first;
            const auto& metric = metric_pair.second;
            
            // フィルターが設定されている場合はチェック
            if (metrics_filter_ && !metrics_filter_(metric_name)) {
                continue;
            }
            
            std::lock_guard<std::mutex> lock(metric->mutex);
            
            if (metric->values.empty()) {
                continue;
            }
            
            // メトリクスのヘルプとタイプを出力
            std::string prefixed_name = addMetricPrefix(metric_name);
            metrics << "# HELP " << prefixed_name << " " << metric->description << "\n";
            
            std::string type_str;
            switch (metric->type) {
                case MetricType::COUNTER: type_str = "counter"; break;
                case MetricType::GAUGE: type_str = "gauge"; break;
                case MetricType::HISTOGRAM: type_str = "histogram"; break;
                case MetricType::SUMMARY: type_str = "summary"; break;
            }
            metrics << "# TYPE " << prefixed_name << " " << type_str << "\n";
            
            // 値を出力
            for (const auto& value_pair : metric->values) {
                metrics << prefixed_name;
                
                // ラベルを構築
                std::ostringstream labels;
                bool has_labels = false;
                
                // グローバルラベルを追加
                for (const auto& global_label : global_labels_) {
                    if (!has_labels) {
                        labels << "{";
                        has_labels = true;
                    } else {
                        labels << ",";
                    }
                    labels << global_label.first << "=\"" << global_label.second << "\"";
                }
                
                // メトリクス固有のラベルを追加
                for (const auto& label : value_pair.second.labels) {
                    if (!has_labels) {
                        labels << "{";
                        has_labels = true;
                    } else {
                        labels << ",";
                    }
                    labels << label.first << "=\"" << label.second << "\"";
                }
                
                if (has_labels) {
                    labels << "}";
                    metrics << labels.str();
                }
                
                // 値とタイムスタンプを出力
                metrics << " " << std::fixed << std::setprecision(6) << value_pair.second.value;
                
                // タイムスタンプをミリ秒で出力
                auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    value_pair.second.timestamp.time_since_epoch()).count();
                metrics << " " << timestamp_ms;
                
                metrics << "\n";
            }
            metrics << "\n";
        }
        
        return metrics.str();
    } catch (const std::exception& e) {
        LOG_ERROR("Prometheusメトリクス生成エラー: {}", e.what());
        return "# Error generating metrics\n";
    }
}

std::string PrometheusExporter::generateHealthResponse() {
    std::ostringstream response;
    response << "Prometheus Exporter Health Check\n";
    response << "Status: OK\n";
    response << "Timestamp: " << std::time(nullptr) << "\n";
    response << "Requests Total: " << requests_total_.load() << "\n";
    response << "Request Errors: " << requests_errors_.load() << "\n";
    
    return response.str();
}

std::string PrometheusExporter::createHttpResponse(int status_code, const std::string& content,
                                                  const std::string& content_type) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code;
    
    // ステータステキストを追加
    switch (status_code) {
        case 200: response << " OK"; break;
        case 404: response << " Not Found"; break;
        case 405: response << " Method Not Allowed"; break;
        case 500: response << " Internal Server Error"; break;
        default: response << " Unknown"; break;
    }
    
    response << "\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << content;
    
    return response.str();
}

std::string PrometheusExporter::createErrorResponse(int status_code, const std::string& message) {
    std::ostringstream content;
    content << "Error " << status_code << ": " << message << "\n";
    return createHttpResponse(status_code, content.str());
}

std::string PrometheusExporter::addMetricPrefix(const std::string& metric_name) {
    // 既にプレフィックスが付いている場合はそのまま返す
    if (metric_name.find("ocpp_gateway_") == 0) {
        return metric_name;
    }
    
    return "ocpp_gateway_" + metric_name;
}

std::string PrometheusExporter::getGlobalLabelsString() {
    if (global_labels_.empty()) {
        return "";
    }
    
    std::ostringstream labels;
    labels << "{";
    bool first = true;
    for (const auto& label : global_labels_) {
        if (!first) {
            labels << ",";
        }
        labels << label.first << "=\"" << label.second << "\"";
        first = false;
    }
    labels << "}";
    
    return labels.str();
}

} // namespace common
} // namespace ocpp_gateway 