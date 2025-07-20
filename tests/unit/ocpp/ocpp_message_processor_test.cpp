#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/asio.hpp>
#include "ocpp_gateway/ocpp/ocpp_message_processor.h"
#include "ocpp_gateway/ocpp/ocpp_message_handlers.h"

using namespace ocpp_gateway::ocpp;
using namespace testing;
using json = nlohmann::json;

// Mock message handler for testing
class MockMessageHandler : public OcppMessageHandler {
public:
    MOCK_METHOD(std::unique_ptr<OcppMessage>, handleMessage, (const OcppMessage&), (override));
};

// Test fixture for OcppMessageProcessor tests
class OcppMessageProcessorTest : public Test {
protected:
    void SetUp() override {
        io_context_ = std::make_shared<boost::asio::io_context>();
        message_processor_ = OcppMessageProcessor::create(*io_context_);
        
        // Set up message callback
        message_processor_->setMessageCallback([this](const std::string& message) {
            sent_messages_.push_back(message);
            return true;
        });
    }
    
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<OcppMessageProcessor> message_processor_;
    std::vector<std::string> sent_messages_;
};

// Test parsing and serializing OCPP messages
TEST_F(OcppMessageProcessorTest, ParseAndSerializeMessages) {
    // Test CALL message
    std::string call_message = R"([2,"12345","BootNotification",{"reason":"PowerUp","chargingStation":{"model":"Test","vendorName":"Vendor"}}])";
    ASSERT_TRUE(message_processor_->processIncomingMessage(call_message));
    
    // Test CALL_RESULT message
    std::string call_result_message = R"([3,"12345",{"currentTime":"2023-01-01T12:00:00Z","interval":300,"status":"Accepted"}])";
    ASSERT_TRUE(message_processor_->processIncomingMessage(call_result_message));
    
    // Test CALL_ERROR message
    std::string call_error_message = R"([4,"12345","NotImplemented","Action not implemented",{}])";
    ASSERT_TRUE(message_processor_->processIncomingMessage(call_error_message));
}

// Test sending messages when connected
TEST_F(OcppMessageProcessorTest, SendMessageWhenConnected) {
    // Set connected state
    message_processor_->setConnected(true);
    
    // Create and send a message
    OcppMessage message = OcppMessage::createCall(
        "test-id",
        OcppMessageAction::HEARTBEAT,
        json());
    
    ASSERT_TRUE(message_processor_->sendMessage(message));
    ASSERT_EQ(sent_messages_.size(), 1);
    
    // Verify the sent message format
    json j = json::parse(sent_messages_[0]);
    ASSERT_EQ(j[0], 2); // CALL
    ASSERT_EQ(j[1], "test-id");
    ASSERT_EQ(j[2], "Heartbeat");
}

// Test message queueing when disconnected
TEST_F(OcppMessageProcessorTest, QueueMessageWhenDisconnected) {
    // Set disconnected state
    message_processor_->setConnected(false);
    
    // Create and send a message
    OcppMessage message = OcppMessage::createCall(
        "test-id",
        OcppMessageAction::HEARTBEAT,
        json());
    
    ASSERT_TRUE(message_processor_->sendMessage(message));
    ASSERT_EQ(sent_messages_.size(), 0); // Message should be queued, not sent
    ASSERT_EQ(message_processor_->getQueueSize(), 1);
    
    // Connect and process queue
    message_processor_->setConnected(true);
    ASSERT_EQ(message_processor_->processQueue(), 1);
    ASSERT_EQ(sent_messages_.size(), 1);
    ASSERT_EQ(message_processor_->getQueueSize(), 0);
}

// Test message handler registration and dispatch
TEST_F(OcppMessageProcessorTest, MessageHandlerDispatch) {
    // Create a mock handler
    auto mock_handler = std::make_shared<MockMessageHandler>();
    
    // Register the handler
    message_processor_->registerHandler(OcppMessageAction::HEARTBEAT, mock_handler);
    
    // Set up expectations
    EXPECT_CALL(*mock_handler, handleMessage(_))
        .WillOnce(Return(std::make_unique<OcppMessage>(
            OcppMessage::createCallResult("test-id", json({{"currentTime", "2023-01-01T12:00:00Z"}})))));
    
    // Process a message
    std::string call_message = R"([2,"test-id","Heartbeat",{}])";
    ASSERT_TRUE(message_processor_->processIncomingMessage(call_message));
    
    // Verify response was sent
    ASSERT_EQ(sent_messages_.size(), 1);
    
    // Parse and verify the response
    json j = json::parse(sent_messages_[0]);
    ASSERT_EQ(j[0], 3); // CALL_RESULT
    ASSERT_EQ(j[1], "test-id");
    ASSERT_TRUE(j[2].contains("currentTime"));
}

// Test handling unknown actions
TEST_F(OcppMessageProcessorTest, HandleUnknownAction) {
    // Process a message with unknown action
    std::string call_message = R"([2,"test-id","UnknownAction",{}])";
    ASSERT_TRUE(message_processor_->processIncomingMessage(call_message));
    
    // Verify error response was sent
    ASSERT_EQ(sent_messages_.size(), 1);
    
    // Parse and verify the response
    json j = json::parse(sent_messages_[0]);
    ASSERT_EQ(j[0], 4); // CALL_ERROR
    ASSERT_EQ(j[1], "test-id");
    ASSERT_EQ(j[2], "NotImplemented");
}

// Test handling malformed messages
TEST_F(OcppMessageProcessorTest, HandleMalformedMessage) {
    // Process a malformed message
    std::string malformed_message = R"({"not":"valid"})";
    ASSERT_FALSE(message_processor_->processIncomingMessage(malformed_message));
    
    // No response should be sent
    ASSERT_EQ(sent_messages_.size(), 0);
}

// Test clearing the message queue
TEST_F(OcppMessageProcessorTest, ClearQueue) {
    // Set disconnected state
    message_processor_->setConnected(false);
    
    // Queue some messages
    for (int i = 0; i < 5; i++) {
        OcppMessage message = OcppMessage::createCall(
            "test-id-" + std::to_string(i),
            OcppMessageAction::HEARTBEAT,
            json());
        
        ASSERT_TRUE(message_processor_->sendMessage(message));
    }
    
    ASSERT_EQ(message_processor_->getQueueSize(), 5);
    
    // Clear the queue
    message_processor_->clearQueue();
    ASSERT_EQ(message_processor_->getQueueSize(), 0);
}

// Test action string conversion
TEST_F(OcppMessageProcessorTest, ActionStringConversion) {
    // Test string to action
    ASSERT_EQ(OcppMessageProcessor::stringToAction("Heartbeat"), OcppMessageAction::HEARTBEAT);
    ASSERT_EQ(OcppMessageProcessor::stringToAction("BootNotification"), OcppMessageAction::BOOT_NOTIFICATION);
    ASSERT_EQ(OcppMessageProcessor::stringToAction("UnknownAction"), OcppMessageAction::UNKNOWN);
    
    // Test action to string
    ASSERT_EQ(OcppMessageProcessor::actionToString(OcppMessageAction::HEARTBEAT), "Heartbeat");
    ASSERT_EQ(OcppMessageProcessor::actionToString(OcppMessageAction::BOOT_NOTIFICATION), "BootNotification");
    ASSERT_EQ(OcppMessageProcessor::actionToString(OcppMessageAction::UNKNOWN), "Unknown");
}

// Test creating different types of messages
TEST_F(OcppMessageProcessorTest, CreateMessages) {
    // Test creating CALL message
    OcppMessage call = OcppMessage::createCall(
        "test-id",
        OcppMessageAction::HEARTBEAT,
        json());
    
    ASSERT_EQ(call.messageType, MessageType::CALL);
    ASSERT_EQ(call.messageId, "test-id");
    ASSERT_EQ(call.action, OcppMessageAction::HEARTBEAT);
    
    // Test creating CALL_RESULT message
    OcppMessage result = OcppMessage::createCallResult(
        "test-id",
        json({{"status", "Accepted"}}));
    
    ASSERT_EQ(result.messageType, MessageType::CALL_RESULT);
    ASSERT_EQ(result.messageId, "test-id");
    ASSERT_EQ(result.payload["status"], "Accepted");
    
    // Test creating CALL_ERROR message
    OcppMessage error = OcppMessage::createCallError(
        "test-id",
        "NotImplemented",
        "Action not implemented");
    
    ASSERT_EQ(error.messageType, MessageType::CALL_ERROR);
    ASSERT_EQ(error.messageId, "test-id");
    ASSERT_EQ(error.payload["code"], "NotImplemented");
    ASSERT_EQ(error.payload["description"], "Action not implemented");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}