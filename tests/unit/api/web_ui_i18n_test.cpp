#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ocpp_gateway/api/web_ui.h"
#include "ocpp_gateway/common/language_manager.h"
#include <thread>
#include <chrono>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>

using namespace ocpp_gateway::api;
using namespace ocpp_gateway::common;
using namespace testing;

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

class WebUIInternationalizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a test port to avoid conflicts
        web_ui_ = std::make_unique<WebUI>(9997, "127.0.0.1", "web_test");
        
        // Initialize language manager with test resources
        LanguageManager::getInstance().initialize("en", "resources/lang");
    }

    void TearDown() override {
        if (web_ui_ && web_ui_->isRunning()) {
            web_ui_->stop();
        }
    }

    std::unique_ptr<WebUI> web_ui_;
};

// Test language switching functionality
TEST_F(WebUIInternationalizationTest, LanguageSwitchingTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test default language (English)
        auto en_response = HttpClient::get("127.0.0.1", 9997, "/dashboard");
        EXPECT_EQ(en_response.status_code, 200);
        
        // Switch to Japanese
        auto ja_switch_response = HttpClient::get("127.0.0.1", 9997, "/?lang=ja");
        EXPECT_EQ(ja_switch_response.status_code, 200);
        
        // Test Japanese content
        auto ja_response = HttpClient::get("127.0.0.1", 9997, "/dashboard");
        EXPECT_EQ(ja_response.status_code, 200);
        EXPECT_TRUE(ja_response.body.find("ダッシュボード") != std::string::npos);
        EXPECT_TRUE(ja_response.body.find("システム状態") != std::string::npos);
        
        // Switch back to English
        auto en_switch_response = HttpClient::get("127.0.0.1", 9997, "/?lang=en");
        EXPECT_EQ(en_switch_response.status_code, 200);
        
        // Test English content
        auto en_response_after = HttpClient::get("127.0.0.1", 9997, "/dashboard");
        EXPECT_EQ(en_response_after.status_code, 200);
        EXPECT_TRUE(en_response_after.body.find("Dashboard") != std::string::npos);
        EXPECT_TRUE(en_response_after.body.find("System Status") != std::string::npos);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test language page
TEST_F(WebUIInternationalizationTest, LanguagePageTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Test language page
        auto language_response = HttpClient::get("127.0.0.1", 9997, "/language");
        EXPECT_EQ(language_response.status_code, 200);
        
        // Check for language selection elements
        EXPECT_TRUE(language_response.body.find("English") != std::string::npos || 
                    language_response.body.find("英語") != std::string::npos);
        EXPECT_TRUE(language_response.body.find("Japanese") != std::string::npos || 
                    language_response.body.find("日本語") != std::string::npos);
        
        // Check for language links
        EXPECT_TRUE(language_response.body.find("?lang=en") != std::string::npos);
        EXPECT_TRUE(language_response.body.find("?lang=ja") != std::string::npos);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}

// Test language API methods
TEST_F(WebUIInternationalizationTest, LanguageApiTest) {
    // Test setting language
    EXPECT_TRUE(web_ui_->setLanguage("en"));
    EXPECT_EQ(web_ui_->getCurrentLanguage(), "en");
    
    EXPECT_TRUE(web_ui_->setLanguage("ja"));
    EXPECT_EQ(web_ui_->getCurrentLanguage(), "ja");
    
    // Test invalid language
    EXPECT_FALSE(web_ui_->setLanguage("fr"));
    
    // Test available languages
    std::vector<std::string> available_languages = web_ui_->getAvailableLanguages();
    EXPECT_TRUE(std::find(available_languages.begin(), available_languages.end(), "en") != available_languages.end());
    EXPECT_TRUE(std::find(available_languages.begin(), available_languages.end(), "ja") != available_languages.end());
}

// Test error pages in different languages
TEST_F(WebUIInternationalizationTest, ErrorPagesInDifferentLanguagesTest) {
    // Start the server
    ASSERT_TRUE(web_ui_->start());
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    try {
        // Set language to English
        HttpClient::get("127.0.0.1", 9997, "/?lang=en");
        
        // Test 404 error in English
        auto en_not_found_response = HttpClient::get("127.0.0.1", 9997, "/not_exists");
        EXPECT_EQ(en_not_found_response.status_code, 404);
        EXPECT_TRUE(en_not_found_response.body.find("Error 404") != std::string::npos || 
                    en_not_found_response.body.find("Page not found") != std::string::npos);
        
        // Set language to Japanese
        HttpClient::get("127.0.0.1", 9997, "/?lang=ja");
        
        // Test 404 error in Japanese
        auto ja_not_found_response = HttpClient::get("127.0.0.1", 9997, "/not_exists");
        EXPECT_EQ(ja_not_found_response.status_code, 404);
        EXPECT_TRUE(ja_not_found_response.body.find("エラー 404") != std::string::npos || 
                    ja_not_found_response.body.find("ページが見つかりません") != std::string::npos);
    } catch (const std::exception& e) {
        FAIL() << "Exception during HTTP request: " << e.what();
    }
}