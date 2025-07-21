#include "ocpp_gateway/ocpp/ocpp_message_handlers.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ocpp_gateway {
namespace ocpp {

using json = nlohmann::json;

// Helper function to get current ISO8601 timestamp
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&now_c), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count() << "Z";
    
    return ss.str();
}

// BootNotificationHandler implementation
std::shared_ptr<BootNotificationHandler> BootNotificationHandler::create() {
    return std::shared_ptr<BootNotificationHandler>(new BootNotificationHandler());
}

std::unique_ptr<OcppMessage> BootNotificationHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling BootNotification message");
    
    // Create response with current time and interval
    json response = {
        {"currentTime", getCurrentTimestamp()},
        {"interval", 300},
        {"status", "Accepted"}
    };
    
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, response));
}

OcppMessage BootNotificationHandler::createRequest(
    const std::string& chargePointModel,
    const std::string& chargePointVendor,
    const std::string& firmwareVersion) {
    
    json payload = {
        {"reason", "PowerUp"},
        {"chargingStation", {
            {"model", chargePointModel},
            {"vendorName", chargePointVendor}
        }}
    };
    
    if (!firmwareVersion.empty()) {
        payload["chargingStation"]["firmwareVersion"] = firmwareVersion;
    }
    
    return OcppMessage::createCall("", OcppMessageAction::BOOT_NOTIFICATION, payload);
}

// HeartbeatHandler implementation
std::shared_ptr<HeartbeatHandler> HeartbeatHandler::create() {
    return std::shared_ptr<HeartbeatHandler>(new HeartbeatHandler());
}

std::unique_ptr<OcppMessage> HeartbeatHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling Heartbeat message");
    
    // Create response with current time
    json response = {
        {"currentTime", getCurrentTimestamp()}
    };
    
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, response));
}

OcppMessage HeartbeatHandler::createRequest() {
    return OcppMessage::createCall("", OcppMessageAction::HEARTBEAT, json());
}

// StatusNotificationHandler implementation
std::shared_ptr<StatusNotificationHandler> StatusNotificationHandler::create() {
    return std::shared_ptr<StatusNotificationHandler>(new StatusNotificationHandler());
}

std::unique_ptr<OcppMessage> StatusNotificationHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling StatusNotification message");
    
    // Status notification response is empty
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, json()));
}

OcppMessage StatusNotificationHandler::createRequest(
    int connectorId,
    const std::string& errorCode,
    const std::string& status,
    const std::string& timestamp) {
    
    json payload = {
        {"connectorId", connectorId},
        {"errorCode", errorCode},
        {"connectorStatus", status},
        {"timestamp", timestamp.empty() ? getCurrentTimestamp() : timestamp}
    };
    
    return OcppMessage::createCall("", OcppMessageAction::STATUS_NOTIFICATION, payload);
}

// TransactionEventHandler implementation
std::shared_ptr<TransactionEventHandler> TransactionEventHandler::create() {
    return std::shared_ptr<TransactionEventHandler>(new TransactionEventHandler());
}

std::unique_ptr<OcppMessage> TransactionEventHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling TransactionEvent message");
    
    // Transaction event response is empty
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, json()));
}

OcppMessage TransactionEventHandler::createRequest(
    const std::string& eventType,
    const std::string& timestamp,
    const std::string& triggerReason,
    int seqNo,
    const std::string& transactionId,
    int evseId,
    double meterValue) {
    
    json payload = {
        {"eventType", eventType},
        {"timestamp", timestamp.empty() ? getCurrentTimestamp() : timestamp},
        {"triggerReason", triggerReason},
        {"seqNo", seqNo},
        {"transactionInfo", {
            {"transactionId", transactionId}
        }},
        {"evse", {
            {"id", evseId}
        }}
    };
    
    // Add meter value if provided
    if (meterValue >= 0) {
        payload["meterValue"] = {
            {
                {"timestamp", timestamp.empty() ? getCurrentTimestamp() : timestamp},
                {"sampledValue", {
                    {
                        {"value", meterValue},
                        {"context", "Transaction.Begin"},
                        {"measurand", "Energy.Active.Import.Register"},
                        {"location", "Outlet"},
                        {"unitOfMeasure", {
                            {"unit", "kWh"}
                        }}
                    }
                }}
            }
        };
    }
    
    return OcppMessage::createCall("", OcppMessageAction::TRANSACTION_EVENT, payload);
}

// MeterValuesHandler implementation
std::shared_ptr<MeterValuesHandler> MeterValuesHandler::create() {
    return std::shared_ptr<MeterValuesHandler>(new MeterValuesHandler());
}

std::unique_ptr<OcppMessage> MeterValuesHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling MeterValues message");
    
    // MeterValues response is empty
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, json()));
}

OcppMessage MeterValuesHandler::createRequest(
    int evseId,
    double meterValue,
    const std::string& timestamp) {
    
    std::string ts = timestamp.empty() ? getCurrentTimestamp() : timestamp;
    
    json payload = {
        {"evseId", evseId},
        {"meterValue", {
            {
                {"timestamp", ts},
                {"sampledValue", {
                    {
                        {"value", meterValue},
                        {"context", "Sample.Periodic"},
                        {"measurand", "Energy.Active.Import.Register"},
                        {"location", "Outlet"},
                        {"unitOfMeasure", {
                            {"unit", "kWh"}
                        }}
                    }
                }}
            }
        }}
    };
    
    return OcppMessage::createCall("", OcppMessageAction::METER_VALUES, payload);
}

// AuthorizeHandler implementation
std::shared_ptr<AuthorizeHandler> AuthorizeHandler::create() {
    return std::shared_ptr<AuthorizeHandler>(new AuthorizeHandler());
}

std::unique_ptr<OcppMessage> AuthorizeHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling Authorize message");
    
    // Extract idToken from request
    try {
        std::string idToken = message.payload["idToken"]["idToken"].get<std::string>();
        LOG_INFO("Authorization requested for ID: {}", idToken);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to extract idToken: {}", e.what());
    }
    
    // For demonstration, accept all tokens
    json response = {
        {"idTokenInfo", {
            {"status", "Accepted"}
        }}
    };
    
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, response));
}

OcppMessage AuthorizeHandler::createRequest(const std::string& idToken) {
    json payload = {
        {"idToken", {
            {"idToken", idToken},
            {"type", "ISO14443"}
        }}
    };
    
    return OcppMessage::createCall("", OcppMessageAction::AUTHORIZE, payload);
}

// RemoteStartTransactionHandler implementation
std::shared_ptr<RemoteStartTransactionHandler> RemoteStartTransactionHandler::create() {
    return std::shared_ptr<RemoteStartTransactionHandler>(new RemoteStartTransactionHandler());
}

std::unique_ptr<OcppMessage> RemoteStartTransactionHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling RemoteStartTransaction message");
    
    // Extract idToken and evseId from request
    try {
        std::string idToken = message.payload["idToken"]["idToken"].get<std::string>();
        
        int evseId = 0;
        if (message.payload.contains("evseId")) {
            evseId = message.payload["evseId"].get<int>();
        }
        
        LOG_INFO("Remote start requested for ID: {}, EVSE: {}", idToken, evseId);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to extract RemoteStartTransaction parameters: {}", e.what());
        
        // Return error response
        return std::make_unique<OcppMessage>(OcppMessage::createCallResult(
            message.messageId, 
            {{"status", "Rejected"}}));
    }
    
    // For demonstration, accept all requests
    json response = {
        {"status", "Accepted"}
    };
    
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, response));
}

// RemoteStopTransactionHandler implementation
std::shared_ptr<RemoteStopTransactionHandler> RemoteStopTransactionHandler::create() {
    return std::shared_ptr<RemoteStopTransactionHandler>(new RemoteStopTransactionHandler());
}

std::unique_ptr<OcppMessage> RemoteStopTransactionHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling RemoteStopTransaction message");
    
    // Extract transactionId from request
    try {
        std::string transactionId = message.payload["transactionId"].get<std::string>();
        LOG_INFO("Remote stop requested for transaction: {}", transactionId);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to extract transactionId: {}", e.what());
        
        // Return error response
        return std::make_unique<OcppMessage>(OcppMessage::createCallResult(
            message.messageId, 
            {{"status", "Rejected"}}));
    }
    
    // For demonstration, accept all requests
    json response = {
        {"status", "Accepted"}
    };
    
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, response));
}

// UnlockConnectorHandler implementation
std::shared_ptr<UnlockConnectorHandler> UnlockConnectorHandler::create() {
    return std::shared_ptr<UnlockConnectorHandler>(new UnlockConnectorHandler());
}

std::unique_ptr<OcppMessage> UnlockConnectorHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling UnlockConnector message");
    
    // Extract evseId and connectorId from request
    int evseId = 0;
    int connectorId = 0;
    
    try {
        evseId = message.payload["evseId"].get<int>();
        connectorId = message.payload["connectorId"].get<int>();
        LOG_INFO("Unlock requested for EVSE: {}, Connector: {}", evseId, connectorId);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to extract UnlockConnector parameters: {}", e.what());
        
        // Return error response
        return std::make_unique<OcppMessage>(OcppMessage::createCallResult(
            message.messageId, 
            {{"status", "Rejected"}}));
    }
    
    // For demonstration, accept all requests
    json response = {
        {"status", "Unlocked"}
    };
    
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, response));
}

// TriggerMessageHandler implementation
std::shared_ptr<TriggerMessageHandler> TriggerMessageHandler::create() {
    return std::shared_ptr<TriggerMessageHandler>(new TriggerMessageHandler());
}

std::unique_ptr<OcppMessage> TriggerMessageHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling TriggerMessage message");
    
    // Extract requestedMessage from request
    try {
        std::string requestedMessage = message.payload["requestedMessage"].get<std::string>();
        
        int evseId = 0;
        if (message.payload.contains("evseId")) {
            evseId = message.payload["evseId"].get<int>();
        }
        
        LOG_INFO("Trigger requested for message: {}, EVSE: {}", requestedMessage, evseId);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to extract TriggerMessage parameters: {}", e.what());
        
        // Return error response
        return std::make_unique<OcppMessage>(OcppMessage::createCallResult(
            message.messageId, 
            {{"status", "Rejected"}}));
    }
    
    // For demonstration, accept all requests
    json response = {
        {"status", "Accepted"}
    };
    
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, response));
}

// SetChargingProfileHandler implementation
std::shared_ptr<SetChargingProfileHandler> SetChargingProfileHandler::create() {
    return std::shared_ptr<SetChargingProfileHandler>(new SetChargingProfileHandler());
}

std::unique_ptr<OcppMessage> SetChargingProfileHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling SetChargingProfile message");
    
    // Extract evseId and chargingProfile from request
    int evseId = 0;
    
    try {
        evseId = message.payload["evseId"].get<int>();
        LOG_INFO("Charging profile requested for EVSE: {}", evseId);
        
        // Log charging profile details
        if (message.payload.contains("chargingProfile")) {
            auto& profile = message.payload["chargingProfile"];
            
            if (profile.contains("id")) {
                LOG_INFO("  Profile ID: {}", profile["id"].get<int>());
            }
            
            if (profile.contains("stackLevel")) {
                LOG_INFO("  Stack Level: {}", profile["stackLevel"].get<int>());
            }
            
            if (profile.contains("chargingProfilePurpose")) {
                LOG_INFO("  Purpose: {}", profile["chargingProfilePurpose"].get<std::string>());
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to extract SetChargingProfile parameters: {}", e.what());
        
        // Return error response
        return std::make_unique<OcppMessage>(OcppMessage::createCallResult(
            message.messageId, 
            {{"status", "Rejected"}}));
    }
    
    // For demonstration, accept all requests
    json response = {
        {"status", "Accepted"}
    };
    
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, response));
}

// DataTransferHandler implementation
std::shared_ptr<DataTransferHandler> DataTransferHandler::create() {
    return std::shared_ptr<DataTransferHandler>(new DataTransferHandler());
}

std::unique_ptr<OcppMessage> DataTransferHandler::handleMessage(const OcppMessage& message) {
    LOG_INFO("Handling DataTransfer message");
    
    // Extract vendorId and messageId from request
    try {
        std::string vendorId = message.payload["vendorId"].get<std::string>();
        
        std::string messageId;
        if (message.payload.contains("messageId")) {
            messageId = message.payload["messageId"].get<std::string>();
        }
        
        LOG_INFO("Data transfer from vendor: {}, message: {}", vendorId, messageId);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to extract DataTransfer parameters: {}", e.what());
        
        // Return error response
        return std::make_unique<OcppMessage>(OcppMessage::createCallResult(
            message.messageId, 
            {{"status", "Rejected"}}));
    }
    
    // For demonstration, accept all requests
    json response = {
        {"status", "Accepted"}
    };
    
    return std::make_unique<OcppMessage>(OcppMessage::createCallResult(message.messageId, response));
}

OcppMessage DataTransferHandler::createRequest(
    const std::string& vendorId,
    const std::string& messageId,
    const json& data) {
    
    json payload = {
        {"vendorId", vendorId}
    };
    
    if (!messageId.empty()) {
        payload["messageId"] = messageId;
    }
    
    if (!data.is_null()) {
        payload["data"] = data;
    }
    
    return OcppMessage::createCall("", OcppMessageAction::DATA_TRANSFER, payload);
}

} // namespace ocpp
} // namespace ocpp_gateway