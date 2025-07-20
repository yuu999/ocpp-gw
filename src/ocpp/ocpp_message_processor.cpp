#include "ocpp_gateway/ocpp/ocpp_message_processor.h"
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ocpp_gateway {
namespace ocpp {

using json = nlohmann::json;

// Helper function to generate a random message ID
std::string generateMessageId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 35); // 0-9, a-z
    
    const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::string id;
    id.reserve(8);
    
    for (int i = 0; i < 8; ++i) {
        id += charset[dis(gen)];
    }
    
    return id;
}

// OcppMessage static methods
OcppMessage OcppMessage::createCall(
    const std::string& messageId,
    OcppMessageAction action,
    const json& payload) {
    
    OcppMessage message;
    message.messageType = MessageType::CALL;
    message.messageId = messageId.empty() ? generateMessageId() : messageId;
    message.action = action;
    message.payload = payload;
    return message;
}

OcppMessage OcppMessage::createCallResult(
    const std::string& messageId,
    const json& payload) {
    
    OcppMessage message;
    message.messageType = MessageType::CALL_RESULT;
    message.messageId = messageId;
    message.action = OcppMessageAction::UNKNOWN;
    message.payload = payload;
    return message;
}

OcppMessage OcppMessage::createCallError(
    const std::string& messageId,
    const std::string& errorCode,
    const std::string& errorDescription,
    const json& errorDetails) {
    
    OcppMessage message;
    message.messageType = MessageType::CALL_ERROR;
    message.messageId = messageId;
    message.action = OcppMessageAction::UNKNOWN;
    
    json error;
    error["code"] = errorCode;
    error["description"] = errorDescription;
    if (!errorDetails.is_null()) {
        error["details"] = errorDetails;
    }
    
    message.payload = error;
    return message;
}

// OcppMessageProcessor implementation
std::shared_ptr<OcppMessageProcessor> OcppMessageProcessor::create(boost::asio::io_context& io_context) {
    return std::shared_ptr<OcppMessageProcessor>(new OcppMessageProcessor(io_context));
}

OcppMessageProcessor::OcppMessageProcessor(boost::asio::io_context& io_context)
    : io_context_(io_context),
      connected_(false) {
}

bool OcppMessageProcessor::processIncomingMessage(const std::string& message) {
    try {
        LOG_DEBUG("Processing incoming OCPP message: {}", message);
        
        // Parse the message
        OcppMessage ocppMessage = parseMessage(message);
        
        // Handle the message
        std::unique_ptr<OcppMessage> response = handleMessage(ocppMessage);
        
        // Send response if needed
        if (response) {
            return sendMessage(*response);
        }
        
        return true;
    } catch (const common::OcppGatewayException& e) {
        LOG_ERROR("Error processing OCPP message: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Unexpected error processing OCPP message: {}", e.what());
        return false;
    }
}

bool OcppMessageProcessor::sendMessage(const OcppMessage& message) {
    try {
        // Serialize the message
        std::string jsonMessage = serializeMessage(message);
        
        LOG_DEBUG("Sending OCPP message: {}", jsonMessage);
        
        // If connected, send directly, otherwise queue
        if (connected_) {
            // Store pending request if it's a CALL
            if (message.messageType == MessageType::CALL) {
                std::lock_guard<std::mutex> lock(pending_mutex_);
                pending_requests_[message.messageId] = message.action;
            }
            
            return sendDirectMessage(jsonMessage);
        } else {
            queueMessage(jsonMessage);
            return true;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error sending OCPP message: {}", e.what());
        return false;
    }
}

void OcppMessageProcessor::setMessageCallback(MessageCallback callback) {
    message_callback_ = callback;
}

void OcppMessageProcessor::registerHandler(OcppMessageAction action, std::shared_ptr<OcppMessageHandler> handler) {
    handlers_[action] = handler;
    LOG_INFO("Registered handler for OCPP action: {}", actionToString(action));
}

void OcppMessageProcessor::setConnected(bool connected) {
    bool wasConnected = connected_;
    connected_ = connected;
    
    // If newly connected, process queued messages
    if (connected && !wasConnected) {
        LOG_INFO("Connection established, processing {} queued messages", getQueueSize());
        processQueue();
    }
}

bool OcppMessageProcessor::isConnected() const {
    return connected_;
}

size_t OcppMessageProcessor::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return message_queue_.size();
}

void OcppMessageProcessor::clearQueue() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    std::queue<std::string> empty;
    std::swap(message_queue_, empty);
    LOG_INFO("Message queue cleared");
}

size_t OcppMessageProcessor::processQueue() {
    if (!connected_) {
        LOG_WARN("Cannot process queue, not connected");
        return 0;
    }
    
    size_t processed = 0;
    std::queue<std::string> remaining;
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        while (!message_queue_.empty()) {
            std::string message = message_queue_.front();
            message_queue_.pop();
            
            if (!sendDirectMessage(message)) {
                // Failed to send, keep in queue
                remaining.push(message);
            } else {
                processed++;
            }
        }
        
        // Put remaining messages back in queue
        message_queue_ = std::move(remaining);
    }
    
    LOG_INFO("Processed {} queued messages, {} remaining", processed, remaining.size());
    return processed;
}

OcppMessage OcppMessageProcessor::parseMessage(const std::string& message) {
    try {
        json j = json::parse(message);
        
        if (!j.is_array()) {
            throw common::ProtocolError("OCPP message must be an array");
        }
        
        OcppMessage ocppMessage;
        
        // Parse message type
        if (j.size() < 2 || !j[0].is_number()) {
            throw common::ProtocolError("Invalid OCPP message format");
        }
        
        int messageTypeInt = j[0].get<int>();
        if (messageTypeInt < 2 || messageTypeInt > 4) {
            throw common::ProtocolError("Invalid OCPP message type: " + std::to_string(messageTypeInt));
        }
        
        ocppMessage.messageType = static_cast<MessageType>(messageTypeInt);
        
        // Parse message ID
        if (!j[1].is_string()) {
            throw common::ProtocolError("Message ID must be a string");
        }
        
        ocppMessage.messageId = j[1].get<std::string>();
        
        // Parse action and payload based on message type
        switch (ocppMessage.messageType) {
            case MessageType::CALL:
                if (j.size() < 4 || !j[2].is_string()) {
                    throw common::ProtocolError("CALL message must have action and payload");
                }
                ocppMessage.action = stringToAction(j[2].get<std::string>());
                ocppMessage.payload = j[3];
                break;
                
            case MessageType::CALL_RESULT:
                if (j.size() < 3) {
                    throw common::ProtocolError("CALL_RESULT message must have payload");
                }
                
                // Look up the action from pending requests
                {
                    std::lock_guard<std::mutex> lock(pending_mutex_);
                    auto it = pending_requests_.find(ocppMessage.messageId);
                    if (it != pending_requests_.end()) {
                        ocppMessage.action = it->second;
                        pending_requests_.erase(it);
                    } else {
                        ocppMessage.action = OcppMessageAction::UNKNOWN;
                    }
                }
                
                ocppMessage.payload = j[2];
                break;
                
            case MessageType::CALL_ERROR:
                if (j.size() < 5 || !j[2].is_string() || !j[3].is_string()) {
                    throw common::ProtocolError("CALL_ERROR message must have error code, description, and details");
                }
                ocppMessage.action = OcppMessageAction::UNKNOWN;
                
                // Create error payload
                ocppMessage.payload = {
                    {"code", j[2]},
                    {"description", j[3]},
                    {"details", j.size() >= 5 ? j[4] : json()}
                };
                break;
        }
        
        return ocppMessage;
    } catch (const json::exception& e) {
        throw common::ProtocolError("JSON parsing error: " + std::string(e.what()));
    }
}

std::string OcppMessageProcessor::serializeMessage(const OcppMessage& message) {
    json j = json::array();
    
    // Add message type
    j.push_back(static_cast<int>(message.messageType));
    
    // Add message ID
    j.push_back(message.messageId);
    
    // Add action and payload based on message type
    switch (message.messageType) {
        case MessageType::CALL:
            j.push_back(actionToString(message.action));
            j.push_back(message.payload);
            break;
            
        case MessageType::CALL_RESULT:
            j.push_back(message.payload);
            break;
            
        case MessageType::CALL_ERROR:
            j.push_back(message.payload["code"]);
            j.push_back(message.payload["description"]);
            if (message.payload.contains("details")) {
                j.push_back(message.payload["details"]);
            }
            break;
    }
    
    return j.dump();
}

std::unique_ptr<OcppMessage> OcppMessageProcessor::handleMessage(const OcppMessage& message) {
    // For CALL messages, find the appropriate handler
    if (message.messageType == MessageType::CALL) {
        auto it = handlers_.find(message.action);
        if (it != handlers_.end() && it->second) {
            LOG_DEBUG("Handling OCPP {} message with registered handler", actionToString(message.action));
            return it->second->handleMessage(message);
        } else {
            LOG_WARN("No handler registered for OCPP action: {}", actionToString(message.action));
            
            // Return a NotImplemented error
            return std::make_unique<OcppMessage>(OcppMessage::createCallError(
                message.messageId,
                "NotImplemented",
                "No handler registered for action: " + actionToString(message.action)));
        }
    }
    
    // For CALL_RESULT and CALL_ERROR, find the pending request
    else {
        LOG_DEBUG("Received {} for message ID: {}", 
                 message.messageType == MessageType::CALL_RESULT ? "CALL_RESULT" : "CALL_ERROR",
                 message.messageId);
        
        // No response needed for results and errors
        return nullptr;
    }
}

bool OcppMessageProcessor::sendDirectMessage(const std::string& message) {
    if (!message_callback_) {
        LOG_ERROR("Cannot send message, no callback registered");
        return false;
    }
    
    try {
        message_callback_(message);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error in message callback: {}", e.what());
        return false;
    }
}

void OcppMessageProcessor::queueMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    message_queue_.push(message);
    LOG_DEBUG("Message queued, queue size: {}", message_queue_.size());
}

OcppMessageAction OcppMessageProcessor::stringToAction(const std::string& action) {
    static const std::map<std::string, OcppMessageAction> actionMap = {
        {"Authorize", OcppMessageAction::AUTHORIZE},
        {"BootNotification", OcppMessageAction::BOOT_NOTIFICATION},
        {"CancelReservation", OcppMessageAction::CANCEL_RESERVATION},
        {"ChangeAvailability", OcppMessageAction::CHANGE_AVAILABILITY},
        {"ClearCache", OcppMessageAction::CLEAR_CACHE},
        {"ClearChargingProfile", OcppMessageAction::CLEAR_CHARGING_PROFILE},
        {"DataTransfer", OcppMessageAction::DATA_TRANSFER},
        {"GetCompositeSchedule", OcppMessageAction::GET_COMPOSITE_SCHEDULE},
        {"GetConfiguration", OcppMessageAction::GET_CONFIGURATION},
        {"GetDiagnostics", OcppMessageAction::GET_DIAGNOSTICS},
        {"GetLocalListVersion", OcppMessageAction::GET_LOCAL_LIST_VERSION},
        {"Heartbeat", OcppMessageAction::HEARTBEAT},
        {"MeterValues", OcppMessageAction::METER_VALUES},
        {"RemoteStartTransaction", OcppMessageAction::REMOTE_START_TRANSACTION},
        {"RemoteStopTransaction", OcppMessageAction::REMOTE_STOP_TRANSACTION},
        {"Reset", OcppMessageAction::RESET},
        {"SendLocalList", OcppMessageAction::SEND_LOCAL_LIST},
        {"SetChargingProfile", OcppMessageAction::SET_CHARGING_PROFILE},
        {"StatusNotification", OcppMessageAction::STATUS_NOTIFICATION},
        {"TransactionEvent", OcppMessageAction::TRANSACTION_EVENT},
        {"TriggerMessage", OcppMessageAction::TRIGGER_MESSAGE},
        {"UnlockConnector", OcppMessageAction::UNLOCK_CONNECTOR},
        {"UpdateFirmware", OcppMessageAction::UPDATE_FIRMWARE}
    };
    
    auto it = actionMap.find(action);
    return it != actionMap.end() ? it->second : OcppMessageAction::UNKNOWN;
}

std::string OcppMessageProcessor::actionToString(OcppMessageAction action) {
    switch (action) {
        case OcppMessageAction::AUTHORIZE: return "Authorize";
        case OcppMessageAction::BOOT_NOTIFICATION: return "BootNotification";
        case OcppMessageAction::CANCEL_RESERVATION: return "CancelReservation";
        case OcppMessageAction::CHANGE_AVAILABILITY: return "ChangeAvailability";
        case OcppMessageAction::CLEAR_CACHE: return "ClearCache";
        case OcppMessageAction::CLEAR_CHARGING_PROFILE: return "ClearChargingProfile";
        case OcppMessageAction::DATA_TRANSFER: return "DataTransfer";
        case OcppMessageAction::GET_COMPOSITE_SCHEDULE: return "GetCompositeSchedule";
        case OcppMessageAction::GET_CONFIGURATION: return "GetConfiguration";
        case OcppMessageAction::GET_DIAGNOSTICS: return "GetDiagnostics";
        case OcppMessageAction::GET_LOCAL_LIST_VERSION: return "GetLocalListVersion";
        case OcppMessageAction::HEARTBEAT: return "Heartbeat";
        case OcppMessageAction::METER_VALUES: return "MeterValues";
        case OcppMessageAction::REMOTE_START_TRANSACTION: return "RemoteStartTransaction";
        case OcppMessageAction::REMOTE_STOP_TRANSACTION: return "RemoteStopTransaction";
        case OcppMessageAction::RESET: return "Reset";
        case OcppMessageAction::SEND_LOCAL_LIST: return "SendLocalList";
        case OcppMessageAction::SET_CHARGING_PROFILE: return "SetChargingProfile";
        case OcppMessageAction::STATUS_NOTIFICATION: return "StatusNotification";
        case OcppMessageAction::TRANSACTION_EVENT: return "TransactionEvent";
        case OcppMessageAction::TRIGGER_MESSAGE: return "TriggerMessage";
        case OcppMessageAction::UNLOCK_CONNECTOR: return "UnlockConnector";
        case OcppMessageAction::UPDATE_FIRMWARE: return "UpdateFirmware";
        case OcppMessageAction::UNKNOWN: return "Unknown";
    }
    
    return "Unknown";
}

} // namespace ocpp
} // namespace ocpp_gateway