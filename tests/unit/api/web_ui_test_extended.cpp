#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ocpp_gateway/api/web_ui.h"
#include "ocpp_gateway/common/config_manager.h"
#include <thread>
#include <chrono>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include <filesystem>

using namespace ocpp_gateway::api;
using namespace ocpp_gateway::config;
using namespace testing;
namespace fs = std::filesystem;

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

    static Response post(const std::string& host, unsigned short port, const std::string& target,
                         const std::string& body, const std::map<std::string, std::string>& headers = {}) {
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

            http::request<http::string_body> req{http::verb::post, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "WebUI Test Client");
            req.set(http::field::content_type, "application/x-www-form-urlencoded");
            req.body() = body;
            req.prepare_payload();

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

class WebUIExtendedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test document root
        test_doc_root_ = "web_ui_test_root";
        if (fs::exists(test_doc_root_)) {
            fs::remove_all(test_doc_root_);
        }
        fs::create_directory(test_doc_root_);
        
        // Create some test files
        createTestFiles();
        
        // Use a test port to avoid conflicts
        web_ui_ = std::make_unique<WebUI>(9996, "127.0.0.1", test_doc_root_);
    }

    void TearDown() override {
        if (web_ui_ && web_ui_->isRunning()) {
            web_ui_->stop();
        }
        
        // Clean up test directory
        if (fs::exists(test_doc_root_)) {
            fs::remove_all(test_doc_root_);
        }
    }
    
    void createTestFiles() {
        // Create a test HTML file
        std::ofstream html_file(test_doc_root_ + "/test.html");
        html_file << "<!DOCTYPE html>\n";
        html_file << "<html>\n";
        html_file << "<head>\n";
        html_file << "  <title>Test HTML</title>\n";
        html_file << "</head>\n";
        html_file << "<body>\n";
        html_file << "  <h1>Test HTML File</h1>\n";
        html_file << "  <p>This is a test HTML file.</p>\n";
        html_file << "</body>\n";
        html_file << "</html>\n";
        html_file.close();
        
        // Create a test CSS file
        std::ofstream css_file(test_doc_root_ + "/test.css");
        css_file << "body { font-family: Arial, sans-serif; }\n";
        css_file << "h1 { color: blue; }\n";
        css_file.close();
        
        // Create a test JavaScript file
        std::ofstream js_file(test_doc_root_ + "/test.js");
        js_file << "function test() {\n";
        js_file << "  console.log('Test');\n";
        js_file << "}\n";
        js_file.close();
        
        // Create a test JSON file
        std::ofstream json_file(test_doc_root_ + "/test.json");
        json_file << "{\n";
        json_file << "  \"test\": \"value\",\n";
        json_file << "  \"number\": 123\n";
        json_file << "}\n";
        json_file.close();
    }

    std::unique_ptr<WebUI> web_ui_;
    std::string test_doc_root_;
};

// Test static file serving
TEST_F(WebUIExtendedTest, StaticFileServingTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test HTML file
        auto html_response = HttpClient::get("127.0.0.1", 9996, "/test.html");
        EXPECT_EQ(html_response.status_code, 200);
        EXPECT_TRUE(html_response.body.find("Test HTML File") != std::string::npos);
        EXPECT_TRUE(html_response.headers["content-type"].find("text/html") != std::string::npos);
        
        // Test CSS file
        auto css_response = HttpClient::get("127.0.0.1", 9996, "/test.css");
        EXPECT_EQ(css_response.status_code, 200);
        EXPECT_TRUE(css_response.body.find("font-family") != std::string::npos);
        EXPECT_TRUE(css_response.headers["content-type"].find("text/css") != std::string::npos);
        
        // Test JavaScript file
        auto js_response = HttpClient::get("127.0.0.1", 9996, "/test.js");
        EXPECT_EQ(js_response.status_code, 200);
        EXPECT_TRUE(js_response.body.find("function test()") != std::string::npos);
        EXPECT_TRUE(js_response.headers["content-type"].find("application/javascript") != std::string::npos);
        
        // Test JSON file
        auto json_response = HttpClient::get("127.0.0.1", 9996, "/test.json");
        EXPECT_EQ(json_response.status_code, 200);
        EXPECT_TRUE(json_response.body.find("\"test\": \"value\"") != std::string::npos);
        EXPECT_TRUE(json_response.headers["content-type"].find("application/json") != std::string::npos);
        
        // Test non-existent file
        auto not_found_response = HttpClient::get("127.0.0.1", 9996, "/non_existent.html");
        EXPECT_EQ(not_found_response.status_code, 404);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test document root setting
TEST_F(WebUIExtendedTest, DocumentRootSettingTest) {
    // Create a new document root
    std::string new_doc_root = "web_ui_test_root2";
    if (fs::exists(new_doc_root)) {
        fs::remove_all(new_doc_root);
    }
    fs::create_directory(new_doc_root);
    
    // Create a test file in the new document root
    std::ofstream test_file(new_doc_root + "/new_test.html");
    test_file << "<!DOCTYPE html>\n";
    test_file << "<html>\n";
    test_file << "<head>\n";
    test_file << "  <title>New Test HTML</title>\n";
    test_file << "</head>\n";
    test_file << "<body>\n";
    test_file << "  <h1>New Test HTML File</h1>\n";
    test_file << "</body>\n";
    test_file << "</html>\n";
    test_file.close();
    
    // Start the server with the original document root
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test that the file in the original document root is accessible
        auto original_response = HttpClient::get("127.0.0.1", 9996, "/test.html");
        EXPECT_EQ(original_response.status_code, 200);
        
        // Test that the file in the new document root is not accessible
        auto new_response1 = HttpClient::get("127.0.0.1", 9996, "/new_test.html");
        EXPECT_EQ(new_response1.status_code, 404);
        
        // Change the document root
        web_ui_->setDocumentRoot(new_doc_root);
        
        // Test that the file in the new document root is now accessible
        auto new_response2 = HttpClient::get("127.0.0.1", 9996, "/new_test.html");
        EXPECT_EQ(new_response2.status_code, 200);
        EXPECT_TRUE(new_response2.body.find("New Test HTML File") != std::string::npos);
        
        // Test that the file in the original document root is no longer accessible
        auto original_response2 = HttpClient::get("127.0.0.1", 9996, "/test.html");
        EXPECT_EQ(original_response2.status_code, 404);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
    
    // Clean up
    if (fs::exists(new_doc_root)) {
        fs::remove_all(new_doc_root);
    }
}

// Test navigation links
TEST_F(WebUIExtendedTest, NavigationLinksTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Get the dashboard page
        auto dashboard_response = HttpClient::get("127.0.0.1", 9996, "/dashboard");
        EXPECT_EQ(dashboard_response.status_code, 200);
        
        // Check for navigation links
        EXPECT_TRUE(dashboard_response.body.find("href=\"/dashboard\"") != std::string::npos);
        EXPECT_TRUE(dashboard_response.body.find("href=\"/devices\"") != std::string::npos);
        EXPECT_TRUE(dashboard_response.body.find("href=\"/config\"") != std::string::npos);
        EXPECT_TRUE(dashboard_response.body.find("href=\"/logs\"") != std::string::npos);
        
        // Follow each navigation link
        auto devices_response = HttpClient::get("127.0.0.1", 9996, "/devices");
        EXPECT_EQ(devices_response.status_code, 200);
        
        auto config_response = HttpClient::get("127.0.0.1", 9996, "/config");
        EXPECT_EQ(config_response.status_code, 200);
        
        auto logs_response = HttpClient::get("127.0.0.1", 9996, "/logs");
        EXPECT_EQ(logs_response.status_code, 200);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test language page
TEST_F(WebUIExtendedTest, LanguagePageTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Get the language page
        auto language_response = HttpClient::get("127.0.0.1", 9996, "/language");
        EXPECT_EQ(language_response.status_code, 200);
        
        // Check for language selection elements
        EXPECT_TRUE(language_response.body.find("?lang=en") != std::string::npos);
        EXPECT_TRUE(language_response.body.find("?lang=ja") != std::string::npos);
        
        // Test language switching
        auto en_response = HttpClient::get("127.0.0.1", 9996, "/?lang=en");
        EXPECT_EQ(en_response.status_code, 200);
        
        auto ja_response = HttpClient::get("127.0.0.1", 9996, "/?lang=ja");
        EXPECT_EQ(ja_response.status_code, 200);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test responsive design
TEST_F(WebUIExtendedTest, ResponsiveDesignTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Get the dashboard page
        auto dashboard_response = HttpClient::get("127.0.0.1", 9996, "/dashboard");
        EXPECT_EQ(dashboard_response.status_code, 200);
        
        // Check for responsive design elements
        EXPECT_TRUE(dashboard_response.body.find("meta name=\"viewport\"") != std::string::npos);
        EXPECT_TRUE(dashboard_response.body.find("@media (max-width: 768px)") != std::string::npos);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test CSS and JavaScript inclusion
TEST_F(WebUIExtendedTest, CssAndJavaScriptTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Get the dashboard page
        auto dashboard_response = HttpClient::get("127.0.0.1", 9996, "/dashboard");
        EXPECT_EQ(dashboard_response.status_code, 200);
        
        // Check for CSS
        EXPECT_TRUE(dashboard_response.body.find("<style>") != std::string::npos);
        EXPECT_TRUE(dashboard_response.body.find("body {") != std::string::npos);
        EXPECT_TRUE(dashboard_response.body.find(".navbar {") != std::string::npos);
        
        // Check for JavaScript
        EXPECT_TRUE(dashboard_response.body.find("<script>") != std::string::npos);
        EXPECT_TRUE(dashboard_response.body.find("function ") != std::string::npos);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test error handling
TEST_F(WebUIExtendedTest, ErrorHandlingTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test 404 error
        auto not_found_response = HttpClient::get("127.0.0.1", 9996, "/not_exists");
        EXPECT_EQ(not_found_response.status_code, 404);
        EXPECT_TRUE(not_found_response.body.find("Error 404") != std::string::npos || 
                    not_found_response.body.find("エラー 404") != std::string::npos);
        
        // Test back to dashboard link in error page
        EXPECT_TRUE(not_found_response.body.find("href=\"/\"") != std::string::npos);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test authentication
TEST_F(WebUIExtendedTest, AuthenticationTest) {
    // Enable authentication
    web_ui_->setAuthentication(true, "test_user", "test_pass");
    
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test without authentication
        auto no_auth_response = HttpClient::get("127.0.0.1", 9996, "/");
        EXPECT_EQ(no_auth_response.status_code, 401);
        
        // Test with invalid authentication
        std::map<std::string, std::string> invalid_headers;
        invalid_headers["Authorization"] = "Basic invalid_token";
        auto invalid_auth_response = HttpClient::get("127.0.0.1", 9996, "/", invalid_headers);
        EXPECT_EQ(invalid_auth_response.status_code, 401);
        
        // Test with valid authentication
        std::map<std::string, std::string> valid_headers;
        valid_headers["Authorization"] = "Basic test_user:test_pass";
        auto valid_auth_response = HttpClient::get("127.0.0.1", 9996, "/", valid_headers);
        EXPECT_EQ(valid_auth_response.status_code, 200);
        
        // Disable authentication
        web_ui_->stop();
        web_ui_->setAuthentication(false);
        web_ui_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Test without authentication after disabling
        auto after_disable_response = HttpClient::get("127.0.0.1", 9996, "/");
        EXPECT_EQ(after_disable_response.status_code, 200);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test server restart
TEST_F(WebUIExtendedTest, ServerRestartTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test that the server is running
        auto response1 = HttpClient::get("127.0.0.1", 9996, "/");
        EXPECT_EQ(response1.status_code, 200);
        
        // Stop the server
        web_ui_->stop();
        EXPECT_FALSE(web_ui_->isRunning());
        
        // Try to connect (should fail)
        bool connection_failed = false;
        try {
            HttpClient::get("127.0.0.1", 9996, "/");
        } catch (...) {
            connection_failed = true;
        }
        EXPECT_TRUE(connection_failed);
        
        // Restart the server
        ASSERT_TRUE(web_ui_->start());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Test that the server is running again
        auto response2 = HttpClient::get("127.0.0.1", 9996, "/");
        EXPECT_EQ(response2.status_code, 200);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test content type handling
TEST_F(WebUIExtendedTest, ContentTypeTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Create additional test files with different extensions
        std::ofstream svg_file(test_doc_root_ + "/test.svg");
        svg_file << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\">";
        svg_file << "<circle cx=\"50\" cy=\"50\" r=\"40\" stroke=\"black\" stroke-width=\"3\" fill=\"red\" />";
        svg_file << "</svg>";
        svg_file.close();
        
        std::ofstream txt_file(test_doc_root_ + "/test.txt");
        txt_file << "This is a plain text file.";
        txt_file.close();
        
        // Test SVG file
        auto svg_response = HttpClient::get("127.0.0.1", 9996, "/test.svg");
        EXPECT_EQ(svg_response.status_code, 200);
        EXPECT_TRUE(svg_response.headers["content-type"].find("image/svg+xml") != std::string::npos);
        
        // Test TXT file (should default to octet-stream or text/plain)
        auto txt_response = HttpClient::get("127.0.0.1", 9996, "/test.txt");
        EXPECT_EQ(txt_response.status_code, 200);
        EXPECT_TRUE(txt_response.headers["content-type"].find("application/octet-stream") != std::string::npos ||
                    txt_response.headers["content-type"].find("text/plain") != std::string::npos);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test concurrent connections
TEST_F(WebUIExtendedTest, ConcurrentConnectionsTest) {
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
                responses[i] = HttpClient::get("127.0.0.1", 9996, "/");
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Check that all requests were successful
        for (int i = 0; i < num_threads; ++i) {
            EXPECT_EQ(responses[i].status_code, 200);
        }
    } catch (const std::exception& e) {
        FAIL() << "Exception during concurrent HTTP requests: " << e.what();
    }
}