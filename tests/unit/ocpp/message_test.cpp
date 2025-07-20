#include <gtest/gtest.h>
#include "ocpp_gateway/ocpp/message.h"
#include "ocpp_gateway/ocpp/boot_notification.h"
#include "ocpp_gateway/ocpp/heartbeat.h"
#include "ocpp_gateway/ocpp/status_notification.h"
#include "ocpp_gateway/ocpp/transaction_event.h"
#include "ocpp_gateway/ocpp/message_factory.h"
#include <chrono>
#include <memory>

namespace ocpp_gateway {
namespace ocpp {
namespace test {

// Helper function to convert time_point to ISO8601 string (declared extern)
extern std::string timePointToIso8601(const std::chrono::system_clock::time_point& tp);

// Helper function to parse ISO8601 string to time_point (declared extern)
extern std::chrono::system_clock::time_point iso8601ToTimePoint(const std::string& iso8601);

class MessageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test data
    }
    
    void TearDown() override {
        // Clean up test data
    }
};

TEST_F(MessageTest, BootNotificationRequestSerialization) {
    // Create a BootNotificationRequest
    BootNotificationRequest::ChargingStation cs{
        "Model-X",
        "VendorA",
        "1.0.0",
        "SN123456",
        "Modem-Y"
    };
    
    BootNotificationRequest request("msg001", BootNotificationRequest::Reason::PowerUp, cs);
    
    // Serialize to JSON
    std::string json = request.toJson();
    
    // Parse the JSON and verify
    auto j = nlohmann::json::parse(json);
    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 4);
    ASSERT_EQ(j[0], static_cast<int>(MessageType::Call));
    ASSERT_EQ(j[1], "msg001");
    ASSERT_EQ(j[2], "BootNotification");
    
    auto payload = j[3];
    ASSERT_EQ(payload["reason"], "PowerUp");
    ASSERT_TRUE(payload.contains("chargingStation"));
    ASSERT_EQ(payload["chargingStation"]["model"], "Model-X");
    ASSERT_EQ(payload["chargingStation"]["vendorName"], "VendorA");
    ASSERT_EQ(payload["chargingStation"]["firmwareVersion"], "1.0.0");
    ASSERT_EQ(payload["chargingStation"]["serialNumber"], "SN123456");
    ASSERT_EQ(payload["chargingStation"]["modem"], "Modem-Y");
}

TEST_F(MessageTest, BootNotificationRequestDeserialization) {
    // Create a JSON string
    std::string json = R"([2,"msg001","BootNotification",{"reason":"PowerUp","chargingStation":{"model":"Model-X","vendorName":"VendorA","firmwareVersion":"1.0.0","serialNumber":"SN123456","modem":"Modem-Y"}}])";
    
    // Create a default request
    BootNotificationRequest::ChargingStation cs{"Unknown", "Unknown"};
    BootNotificationRequest request("default", BootNotificationRequest::Reason::Unknown, cs);
    
    // Deserialize from JSON
    ASSERT_TRUE(request.fromJson(json));
    
    // Verify the deserialized data
    ASSERT_EQ(request.getMessageId(), "msg001");
    ASSERT_EQ(request.getAction(), MessageAction::BootNotification);
    ASSERT_EQ(request.getReason(), BootNotificationRequest::Reason::PowerUp);
    ASSERT_EQ(request.getChargingStation().model, "Model-X");
    ASSERT_EQ(request.getChargingStation().vendorName, "VendorA");
    ASSERT_TRUE(request.getChargingStation().firmwareVersion.has_value());
    ASSERT_EQ(*request.getChargingStation().firmwareVersion, "1.0.0");
    ASSERT_TRUE(request.getChargingStation().serialNumber.has_value());
    ASSERT_EQ(*request.getChargingStation().serialNumber, "SN123456");
    ASSERT_TRUE(request.getChargingStation().modem.has_value());
    ASSERT_EQ(*request.getChargingStation().modem, "Modem-Y");
}

TEST_F(MessageTest, BootNotificationResponseSerialization) {
    // Create a BootNotificationResponse
    auto currentTime = std::chrono::system_clock::now();
    BootNotificationResponse response("msg001", currentTime, 300, RegistrationStatus::Accepted);
    
    // Serialize to JSON
    std::string json = response.toJson();
    
    // Parse the JSON and verify
    auto j = nlohmann::json::parse(json);
    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 3);
    ASSERT_EQ(j[0], static_cast<int>(MessageType::CallResult));
    ASSERT_EQ(j[1], "msg001");
    
    auto payload = j[2];
    ASSERT_TRUE(payload.contains("currentTime"));
    ASSERT_EQ(payload["interval"], 300);
    ASSERT_EQ(payload["status"], "Accepted");
}

TEST_F(MessageTest, BootNotificationResponseDeserialization) {
    // Create a JSON string with a fixed timestamp
    std::string json = R"([3,"msg001",{"currentTime":"2023-01-01T12:00:00.000Z","interval":300,"status":"Accepted"}])";
    
    // Create a default response
    BootNotificationResponse response("default", std::chrono::system_clock::now(), 0, RegistrationStatus::Rejected);
    
    // Deserialize from JSON
    ASSERT_TRUE(response.fromJson(json));
    
    // Verify the deserialized data
    ASSERT_EQ(response.getMessageId(), "msg001");
    ASSERT_EQ(response.getInterval(), 300);
    ASSERT_EQ(response.getStatus(), RegistrationStatus::Accepted);
    
    // Convert the timestamp to string for comparison
    auto expectedTime = iso8601ToTimePoint("2023-01-01T12:00:00.000Z");
    auto actualTime = response.getCurrentTime();
    
    // Compare timestamps (allowing for small differences due to parsing)
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(
        actualTime - expectedTime).count();
    ASSERT_NEAR(diff, 0, 1); // Allow 1 second difference
}

TEST_F(MessageTest, HeartbeatRequestSerialization) {
    // Create a HeartbeatRequest
    HeartbeatRequest request("msg002");
    
    // Serialize to JSON
    std::string json = request.toJson();
    
    // Parse the JSON and verify
    auto j = nlohmann::json::parse(json);
    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 4);
    ASSERT_EQ(j[0], static_cast<int>(MessageType::Call));
    ASSERT_EQ(j[1], "msg002");
    ASSERT_EQ(j[2], "Heartbeat");
    
    auto payload = j[3];
    ASSERT_TRUE(payload.is_object());
    ASSERT_TRUE(payload.empty()); // Heartbeat request has empty payload
}

TEST_F(MessageTest, HeartbeatResponseSerialization) {
    // Create a HeartbeatResponse
    auto currentTime = std::chrono::system_clock::now();
    HeartbeatResponse response("msg002", currentTime);
    
    // Serialize to JSON
    std::string json = response.toJson();
    
    // Parse the JSON and verify
    auto j = nlohmann::json::parse(json);
    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 3);
    ASSERT_EQ(j[0], static_cast<int>(MessageType::CallResult));
    ASSERT_EQ(j[1], "msg002");
    
    auto payload = j[2];
    ASSERT_TRUE(payload.contains("currentTime"));
}

TEST_F(MessageTest, StatusNotificationRequestSerialization) {
    // Create a StatusNotificationRequest
    auto timestamp = std::chrono::system_clock::now();
    StatusNotificationRequest request("msg003", timestamp, 1, ConnectorStatus::Available, 2);
    
    // Serialize to JSON
    std::string json = request.toJson();
    
    // Parse the JSON and verify
    auto j = nlohmann::json::parse(json);
    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 4);
    ASSERT_EQ(j[0], static_cast<int>(MessageType::Call));
    ASSERT_EQ(j[1], "msg003");
    ASSERT_EQ(j[2], "StatusNotification");
    
    auto payload = j[3];
    ASSERT_TRUE(payload.contains("timestamp"));
    ASSERT_EQ(payload["connectorId"], 1);
    ASSERT_EQ(payload["connectorStatus"], "Available");
    ASSERT_EQ(payload["evseId"], 2);
}

TEST_F(MessageTest, TransactionEventRequestSerialization) {
    // Create a TransactionEventRequest
    auto timestamp = std::chrono::system_clock::now();
    Transaction transaction{"tx001", "Charging", 120, std::nullopt, std::nullopt};
    EVSE evse{1, 2};
    
    TransactionEventRequest request(
        "msg004",
        TransactionEventType::Started,
        timestamp,
        TriggerReason::Authorized,
        1,
        transaction,
        evse
    );
    
    // Serialize to JSON
    std::string json = request.toJson();
    
    // Parse the JSON and verify
    auto j = nlohmann::json::parse(json);
    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 4);
    ASSERT_EQ(j[0], static_cast<int>(MessageType::Call));
    ASSERT_EQ(j[1], "msg004");
    ASSERT_EQ(j[2], "TransactionEvent");
    
    auto payload = j[3];
    ASSERT_EQ(payload["eventType"], "Started");
    ASSERT_TRUE(payload.contains("timestamp"));
    ASSERT_EQ(payload["triggerReason"], "Authorized");
    ASSERT_EQ(payload["seqNo"], 1);
    
    ASSERT_TRUE(payload.contains("transactionInfo"));
    ASSERT_EQ(payload["transactionInfo"]["transactionId"], "tx001");
    ASSERT_EQ(payload["transactionInfo"]["chargingState"], "Charging");
    ASSERT_EQ(payload["transactionInfo"]["timeSpentCharging"], 120);
    
    ASSERT_TRUE(payload.contains("evse"));
    ASSERT_EQ(payload["evse"]["id"], 1);
    ASSERT_EQ(payload["evse"]["connectorId"], 2);
}

TEST_F(MessageTest, MessageFactoryCreateBootNotification) {
    // Create a JSON string for BootNotification request
    std::string json = R"([2,"msg001","BootNotification",{"reason":"PowerUp","chargingStation":{"model":"Model-X","vendorName":"VendorA"}}])";
    
    // Create message using factory
    auto message = MessageFactory::getInstance().createMessage(json);
    
    // Verify the message
    ASSERT_TRUE(message != nullptr);
    ASSERT_EQ(message->getType(), MessageType::Call);
    
    // Cast to Call and verify
    auto call = std::dynamic_pointer_cast<Call>(message);
    ASSERT_TRUE(call != nullptr);
    ASSERT_EQ(call->getMessageId(), "msg001");
    ASSERT_EQ(call->getAction(), MessageAction::BootNotification);
    
    // Cast to BootNotificationRequest and verify
    auto request = std::dynamic_pointer_cast<BootNotificationRequest>(call);
    ASSERT_TRUE(request != nullptr);
    ASSERT_EQ(request->getReason(), BootNotificationRequest::Reason::PowerUp);
    ASSERT_EQ(request->getChargingStation().model, "Model-X");
    ASSERT_EQ(request->getChargingStation().vendorName, "VendorA");
}

} // namespace test
} // namespace ocpp
} // namespace ocpp_gateway