#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ocpp_gateway/api/web_ui.h"
#include "ocpp_gateway/common/config_manager.h"
#include <thread>
#include <chrono>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>

using namespace ocpp_gateway::api;
using namespace ocpp_gateway::config;
using namespace testing;

// Mock ConfigManager for testing
class MockConfigManager : public ConfigManager {
public:
    MOCK_METHOD(bool, reloadAllConfigs, (), (override));
    MOCK_METHOD(void, validateAllConfigs, (), (override));
    MOCK_METHOD(SystemConfig&, getSystemConfig, (), (override));
    MOCK_METHOD(CsmsConfig&, getCsmsConfig, (), (override));
    MOCK_METHOD(DeviceConfigs&, getDeviceConfigs, (), (override));
    MOCK_METHOD(std::optional<DeviceConfig>, getDeviceConfig, (const std::string&), (override));
};

// Helper class for making HTTP requests to the WebUI server
class HttpClient {
public:
    struct Response {
        unsigned int status_code;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    static Response get(const std::string& host, unsigned short port, const std::string& target,
                        const std::map<std::string, std::string>& headers = {}) {
        try {
            namespace beast = boost::beast;
            namespace http = beast::http;
            namespace net = boost::asio;
            using tcp = net::ip::tcp;

            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);

            auto const results = resolver.resolve(host, std::to_string(port));
            stream.connect(results);

            http::request<http::string_body> req{http::verb::get, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "WebUI Test Client");

            // Add custom headers
            for (const auto& header : headers) {
                req.set(header.first, header.second);
            }

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);

            Response response;
            response.status_code = res.result_int();
            response.body = res.body();

            // Extract headers
            for (auto const& field : res) {
                response.headers[std::string(field.name_string())] = std::string(field.value());
            }

            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);

            return response;
        } catch (const std::exception& e) {
            std::cerr << "HTTP request failed: " << e.what() << std::endl;
            return {500, "Request failed: " + std::string(e.what()), {}};
        }
    }
};

class WebUITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a test port to avoid conflicts
        web_ui_ = std::make_unique<WebUI>(9999, "127.0.0.1", "web_test");
    }

    void TearDown() override {
        if (web_ui_ && web_ui_->isRunning()) {
            web_ui_->stop();
        }
    }

    std::unique_ptr<WebUI> web_ui_;
};

// Test basic server start/stop functionality
TEST_F(WebUITest, StartStopTest) {
    EXPECT_FALSE(web_ui_->isRunning());
    
    // Start the server
    EXPECT_TRUE(web_ui_->start());
    EXPECT_TRUE(web_ui_->isRunning());
    
    // Try to start again (should return true but not restart)
    EXPECT_TRUE(web_ui_->start());
    
    // Stop the server
    web_ui_->stop();
    EXPECT_FALSE(web_ui_->isRunning());
    
    // Stop again (should be safe)
    web_ui_->stop();
    EXPECT_FALSE(web_ui_->isRunning());
}

// Test document root setting
TEST_F(WebUITest, DocumentRootTest) {
    // Set a new document root
    web_ui_->setDocumentRoot("/tmp/web_test");
    
    // We can't directly test if it was set correctly as it's a private member
    // This is a limitation of the current design
}

// Test authentication setting
TEST_F(WebUITest, AuthenticationTest) {
    // Enable authentication
    web_ui_->setAuthentication(true, "test_user", "test_pass");
    
    // We can't directly test if it was set correctly as it's a private member
    // This is a limitation of the current design
    
    // Disable authentication
    web_ui_->setAuthentication(false);
}

// Test server response to basic requests
TEST_F(WebUITest, BasicRequestTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test root path (should return dashboard)
        auto root_response = HttpClient::get("127.0.0.1", 9999, "/");
        EXPECT_EQ(root_response.status_code, 200);
        EXPECT_TRUE(root_response.body.find("OCPP 2.0.1") != std::string::npos);
        
        // Test dashboard path
        auto dashboard_response = HttpClient::get("127.0.0.1", 9999, "/dashboard");
        EXPECT_EQ(dashboard_response.status_code, 200);
        EXPECT_TRUE(dashboard_response.body.find("ダッシュボード") != std::string::npos);
        
        // Test devices path
        auto devices_response = HttpClient::get("127.0.0.1", 9999, "/devices");
        EXPECT_EQ(devices_response.status_code, 200);
        EXPECT_TRUE(devices_response.body.find("デバイス管理") != std::string::npos);
        
        // Test config path
        auto config_response = HttpClient::get("127.0.0.1", 9999, "/config");
        EXPECT_EQ(config_response.status_code, 200);
        EXPECT_TRUE(config_response.body.find("設定管理") != std::string::npos);
        
        // Test logs path
        auto logs_response = HttpClient::get("127.0.0.1", 9999, "/logs");
        EXPECT_EQ(logs_response.status_code, 200);
        EXPECT_TRUE(logs_response.body.find("ログ表示") != std::string::npos);
        
        // Test non-existent path
        auto not_found_response = HttpClient::get("127.0.0.1", 9999, "/not_exists");
        EXPECT_EQ(not_found_response.status_code, 404);
        EXPECT_TRUE(not_found_response.body.find("ページが見つかりません") != std::string::npos);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test authentication functionality
TEST_F(WebUITest, AuthenticationRequestTest) {
    // Enable authentication
    web_ui_->setAuthentication(true, "test_user", "test_pass");
    
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test without authentication (should fail)
        auto no_auth_response = HttpClient::get("127.0.0.1", 9999, "/");
        EXPECT_EQ(no_auth_response.status_code, 401);
        EXPECT_TRUE(no_auth_response.body.find("認証が必要です") != std::string::npos);
        
        // Test with invalid authentication (should fail)
        std::map<std::string, std::string> invalid_headers;
        invalid_headers["Authorization"] = "Basic invalid_token";
        auto invalid_auth_response = HttpClient::get("127.0.0.1", 9999, "/", invalid_headers);
        EXPECT_EQ(invalid_auth_response.status_code, 401);
        
        // Test with valid authentication (should succeed)
        // Note: In a real implementation, we would Base64 encode the credentials
        // For this test, we're using the simplified authentication in the WebUI class
        std::map<std::string, std::string> valid_headers;
        valid_headers["Authorization"] = "Basic test_user:test_pass";
        auto valid_auth_response = HttpClient::get("127.0.0.1", 9999, "/", valid_headers);
        EXPECT_EQ(valid_auth_response.status_code, 200);
        EXPECT_TRUE(valid_auth_response.body.find("OCPP 2.0.1") != std::string::npos);
        
        // Disable authentication and test again
        web_ui_->stop();
        web_ui_->setAuthentication(false);
        web_ui_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Now should work without authentication
        auto no_auth_after_disable = HttpClient::get("127.0.0.1", 9999, "/");
        EXPECT_EQ(no_auth_after_disable.status_code, 200);
        EXPECT_TRUE(no_auth_after_disable.body.find("OCPP 2.0.1") != std::string::npos);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test MIME type handling
TEST_F(WebUITest, MimeTypeTest) {
    // We can't directly test getMimeType as it's private
    // This is a limitation of the current design
    
    // However, we can test the behavior indirectly by requesting different file types
    // and checking the Content-Type header
    // This would require setting up test files in the document root
}

// Test page generation
TEST_F(WebUITest, PageGenerationTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test dashboard page
        auto dashboard_response = HttpClient::get("127.0.0.1", 9999, "/dashboard");
        EXPECT_EQ(dashboard_response.status_code, 200);
        EXPECT_TRUE(dashboard_response.body.find("Dashboard") != std::string::npos || 
                    dashboard_response.body.find("ダッシュボード") != std::string::npos);
        EXPECT_TRUE(dashboard_response.body.find("System Status") != std::string::npos || 
                    dashboard_response.body.find("システム状態") != std::string::npos);
        EXPECT_TRUE(dashboard_response.body.find("Device Status") != std::string::npos || 
                    dashboard_response.body.find("デバイス状態") != std::string::npos);
        EXPECT_TRUE(dashboard_response.body.find("Recent Events") != std::string::npos || 
                    dashboard_response.body.find("最近のイベント") != std::string::npos);
        
        // Test devices page
        auto devices_response = HttpClient::get("127.0.0.1", 9999, "/devices");
        EXPECT_EQ(devices_response.status_code, 200);
        EXPECT_TRUE(devices_response.body.find("Device Management") != std::string::npos || 
                    devices_response.body.find("デバイス管理") != std::string::npos);
        EXPECT_TRUE(devices_response.body.find("Registered Devices") != std::string::npos || 
                    devices_response.body.find("登録デバイス一覧") != std::string::npos);
        
        // Test config page
        auto config_response = HttpClient::get("127.0.0.1", 9999, "/config");
        EXPECT_EQ(config_response.status_code, 200);
        EXPECT_TRUE(config_response.body.find("Configuration") != std::string::npos || 
                    config_response.body.find("設定管理") != std::string::npos);
        EXPECT_TRUE(config_response.body.find("System Configuration") != std::string::npos || 
                    config_response.body.find("システム設定") != std::string::npos);
        EXPECT_TRUE(config_response.body.find("CSMS Configuration") != std::string::npos || 
                    config_response.body.find("CSMS設定") != std::string::npos);
        
        // Test logs page
        auto logs_response = HttpClient::get("127.0.0.1", 9999, "/logs");
        EXPECT_EQ(logs_response.status_code, 200);
        EXPECT_TRUE(logs_response.body.find("Logs") != std::string::npos || 
                    logs_response.body.find("ログ表示") != std::string::npos);
        EXPECT_TRUE(logs_response.body.find("Latest Logs") != std::string::npos || 
                    logs_response.body.find("最新ログ") != std::string::npos);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test error handling
TEST_F(WebUITest, ErrorHandlingTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test 404 error
        auto not_found_response = HttpClient::get("127.0.0.1", 9999, "/not_exists");
        EXPECT_EQ(not_found_response.status_code, 404);
        EXPECT_TRUE(not_found_response.body.find("エラー 404") != std::string::npos);
        EXPECT_TRUE(not_found_response.body.find("ページが見つかりません") != std::string::npos);
        
        // We can't easily test other error conditions like 500 errors
        // without modifying the WebUI class for testing
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test concurrent connections
TEST_F(WebUITest, ConcurrentConnectionsTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Create multiple threads to make concurrent requests
        const int num_threads = 10;
        std::vector<std::thread> threads;
        std::vector<HttpClient::Response> responses(num_threads);
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([i, &responses]() {
                responses[i] = HttpClient::get("127.0.0.1", 9999, "/");
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Check that all requests were successful
        for (int i = 0; i < num_threads; ++i) {
            EXPECT_EQ(responses[i].status_code, 200);
            EXPECT_TRUE(responses[i].body.find("OCPP 2.0.1") != std::string::npos);
        }
    } catch (const std::exception& e) {
        FAIL() << "Exception during concurrent HTTP requests: " << e.what();
    }
}

// Test server behavior under load
TEST_F(WebUITest, LoadTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Make multiple sequential requests
        const int num_requests = 50;
        std::vector<HttpClient::Response> responses;
        
        for (int i = 0; i < num_requests; ++i) {
            responses.push_back(HttpClient::get("127.0.0.1", 9999, "/"));
        }
        
        // Check that all requests were successful
        for (int i = 0; i < num_requests; ++i) {
            EXPECT_EQ(responses[i].status_code, 200);
            EXPECT_TRUE(responses[i].body.find("OCPP 2.0.1") != std::string::npos);
        }
    } catch (const std::exception& e) {
        FAIL() << "Exception during load test: " << e.what();
    }
}