#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "ocpp_gateway/ocpp/websocket_client.h"
#include "ocpp_gateway/common/logger.h"

namespace ocpp_gateway {
namespace ocpp {
namespace test {

class WebSocketClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger
        common::Logger::initialize("debug", "");
        
        // Create temporary directory for test certificates
        temp_dir_ = std::filesystem::temp_directory_path() / "ocpp_gateway_test_certs";
        std::filesystem::create_directories(temp_dir_);
        
        // Create test certificates
        createTestCertificates();
    }

    void TearDown() override {
        // Clean up temporary directory
        std::filesystem::remove_all(temp_dir_);
    }
    
    void createTestCertificates() {
        // Create a self-signed certificate for testing
        ca_cert_path_ = (temp_dir_ / "ca.crt").string();
        client_cert_path_ = (temp_dir_ / "client.crt").string();
        client_key_path_ = (temp_dir_ / "client.key").string();
        
        // Write dummy content to certificate files for testing
        // In a real scenario, these would be valid certificates
        std::ofstream ca_cert(ca_cert_path_);
        ca_cert << "-----BEGIN CERTIFICATE-----\nDUMMY CA CERTIFICATE\n-----END CERTIFICATE-----\n";
        ca_cert.close();
        
        std::ofstream client_cert(client_cert_path_);
        client_cert << "-----BEGIN CERTIFICATE-----\nDUMMY CLIENT CERTIFICATE\n-----END CERTIFICATE-----\n";
        client_cert.close();
        
        std::ofstream client_key(client_key_path_);
        client_key << "-----BEGIN PRIVATE KEY-----\nDUMMY PRIVATE KEY\n-----END PRIVATE KEY-----\n";
        client_key.close();
    }

    std::filesystem::path temp_dir_;
    std::string ca_cert_path_;
    std::string client_cert_path_;
    std::string client_key_path_;
};

TEST_F(WebSocketClientTest, ConstructorWithValidConfig) {
    boost::asio::io_context io_context;
    
    WebSocketConfig config;
    config.url = "wss://example.com/ocpp";
    config.ca_cert_path = ca_cert_path_;
    config.verify_peer = true;
    
    auto client = WebSocketClient::create(io_context, config);
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->getState(), ConnectionState::DISCONNECTED);
    EXPECT_EQ(client->getUrl(), "wss://example.com/ocpp");
}

TEST_F(WebSocketClientTest, ConstructorWithInvalidUrl) {
    boost::asio::io_context io_context;
    
    WebSocketConfig config;
    config.url = "invalid-url";
    
    auto client = WebSocketClient::create(io_context, config);
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->getState(), ConnectionState::ERROR);
}

TEST_F(WebSocketClientTest, ParseUrlWithDefaultPort) {
    boost::asio::io_context io_context;
    
    WebSocketConfig config;
    config.url = "wss://example.com/ocpp";
    
    auto client = WebSocketClient::create(io_context, config);
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->getState(), ConnectionState::DISCONNECTED);
}

TEST_F(WebSocketClientTest, ParseUrlWithExplicitPort) {
    boost::asio::io_context io_context;
    
    WebSocketConfig config;
    config.url = "wss://example.com:8443/ocpp";
    
    auto client = WebSocketClient::create(io_context, config);
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->getState(), ConnectionState::DISCONNECTED);
}

TEST_F(WebSocketClientTest, ParseUrlWithoutPath) {
    boost::asio::io_context io_context;
    
    WebSocketConfig config;
    config.url = "wss://example.com";
    
    auto client = WebSocketClient::create(io_context, config);
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->getState(), ConnectionState::DISCONNECTED);
}

TEST_F(WebSocketClientTest, InitSslContextWithClientCert) {
    boost::asio::io_context io_context;
    
    WebSocketConfig config;
    config.url = "wss://example.com/ocpp";
    config.ca_cert_path = ca_cert_path_;
    config.client_cert_path = client_cert_path_;
    config.client_key_path = client_key_path_;
    config.verify_peer = true;
    
    auto client = WebSocketClient::create(io_context, config);
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->getState(), ConnectionState::DISCONNECTED);
}

TEST_F(WebSocketClientTest, InitSslContextWithoutVerification) {
    boost::asio::io_context io_context;
    
    WebSocketConfig config;
    config.url = "wss://example.com/ocpp";
    config.verify_peer = false;
    
    auto client = WebSocketClient::create(io_context, config);
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->getState(), ConnectionState::DISCONNECTED);
}

TEST_F(WebSocketClientTest, ReconnectAttemptsCounter) {
    boost::asio::io_context io_context;
    
    WebSocketConfig config;
    config.url = "wss://example.com/ocpp";
    
    auto client = WebSocketClient::create(io_context, config);
    ASSERT_NE(client, nullptr);
    
    // Initial value should be 0
    EXPECT_EQ(client->getReconnectAttempts(), 0);
    
    // Reset should keep it at 0
    client->resetReconnectAttempts();
    EXPECT_EQ(client->getReconnectAttempts(), 0);
}

TEST_F(WebSocketClientTest, MessageQueueing) {
    boost::asio::io_context io_context;
    
    WebSocketConfig config;
    config.url = "wss://example.com/ocpp";
    
    auto client = WebSocketClient::create(io_context, config);
    ASSERT_NE(client, nullptr);
    
    // Should queue messages even when disconnected
    EXPECT_TRUE(client->send("Test message 1"));
    EXPECT_TRUE(client->send("Test message 2"));
}

// Note: The following tests would require a mock WebSocket server
// or integration with a real server. For unit testing, we focus on
// the client's behavior without actual connections.

} // namespace test
} // namespace ocpp
} // namespace ocpp_gateway