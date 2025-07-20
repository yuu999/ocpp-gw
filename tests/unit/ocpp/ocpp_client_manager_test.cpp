#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include "ocpp_gateway/ocpp/ocpp_client_manager.h"
#include "ocpp_gateway/common/logger.h"

namespace ocpp_gateway {
namespace ocpp {
namespace test {

class OcppClientManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger
        common::Logger::initialize("debug", "");
    }

    void TearDown() override {
    }
    
    // Helper method to create a basic configuration
    OcppClientConfig createTestConfig() {
        OcppClientConfig config;
        config.csms_url = "wss://example.com/ocpp";
        config.verify_peer = false; // Disable verification for tests
        config.connect_timeout = std::chrono::seconds(1);
        config.reconnect_interval = std::chrono::seconds(1);
        config.max_reconnect_interval = std::chrono::seconds(5);
        config.max_reconnect_attempts = 3;
        config.heartbeat_interval = std::chrono::seconds(5);
        return config;
    }
};

TEST_F(OcppClientManagerTest, CreateWithValidConfig) {
    boost::asio::io_context io_context;
    
    auto config = createTestConfig();
    auto manager = OcppClientManager::create(io_context, config);
    
    ASSERT_NE(manager, nullptr);
    EXPECT_FALSE(manager->isConnected());
    EXPECT_EQ(manager->getConnectionState(), ConnectionState::DISCONNECTED);
}

TEST_F(OcppClientManagerTest, StartAndStop) {
    boost::asio::io_context io_context;
    
    auto config = createTestConfig();
    auto manager = OcppClientManager::create(io_context, config);
    
    ASSERT_NE(manager, nullptr);
    
    // Start should succeed even though connection will fail (no server)
    EXPECT_TRUE(manager->start());
    
    // Run io_context for a short time to process async operations
    std::thread io_thread([&io_context]() {
        io_context.run_for(std::chrono::milliseconds(100));
    });
    
    // Stop the manager
    manager->stop();
    
    // Wait for io_context to finish
    io_thread.join();
    
    // Reset io_context for next use
    io_context.restart();
}

TEST_F(OcppClientManagerTest, SendMessageWhenDisconnected) {
    boost::asio::io_context io_context;
    
    auto config = createTestConfig();
    auto manager = OcppClientManager::create(io_context, config);
    
    ASSERT_NE(manager, nullptr);
    
    // Should fail because not connected
    EXPECT_FALSE(manager->sendMessage("Test message"));
}

// Note: More comprehensive tests would require mocking the WebSocket client
// or integration with a real server. For unit testing, we focus on
// the manager's behavior without actual connections.

} // namespace test
} // namespace ocpp
} // namespace ocpp_gateway