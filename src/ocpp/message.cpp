#include "ocpp_gateway/ocpp/message.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace ocpp_gateway {
namespace ocpp {

Message::Message(MessageType type) : type_(type) {}

MessageType Message::getType() const {
    return type_;
}

Call::Call(const std::string& messageId, MessageAction action)
    : Message(MessageType::Call), messageId_(messageId), action_(action) {}

const std::string& Call::getMessageId() const {
    return messageId_;
}

MessageAction Call::getAction() const {
    return action_;
}

std::string Call::getActionString() const {
    return messageActionToString(action_);
}

std::string Call::toJson() const {
    nlohmann::json j = nlohmann::json::array();
    j.push_back(static_cast<int>(type_));
    j.push_back(messageId_);
    j.push_back(messageActionToString(action_));
    j.push_back(getPayloadJson());
    return j.dump();
}

bool Call::fromJson(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);
        
        if (!j.is_array() || j.size() != 4) {
            spdlog::error("Invalid OCPP Call message format");
            return false;
        }
        
        if (j[0] != static_cast<int>(MessageType::Call)) {
            spdlog::error("Message is not a Call message");
            return false;
        }
        
        messageId_ = j[1].get<std::string>();
        action_ = stringToMessageAction(j[2].get<std::string>());
        
        if (action_ == MessageAction::Unknown) {
            spdlog::error("Unknown action: {}", j[2].get<std::string>());
            return false;
        }
        
        return setPayloadFromJson(j[3]);
    } catch (const std::exception& e) {
        spdlog::error("Error parsing Call message: {}", e.what());
        return false;
    }
}

CallResult::CallResult(const std::string& messageId)
    : Message(MessageType::CallResult), messageId_(messageId) {}

const std::string& CallResult::getMessageId() const {
    return messageId_;
}

std::string CallResult::toJson() const {
    nlohmann::json j = nlohmann::json::array();
    j.push_back(static_cast<int>(type_));
    j.push_back(messageId_);
    j.push_back(getPayloadJson());
    return j.dump();
}

bool CallResult::fromJson(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);
        
        if (!j.is_array() || j.size() != 3) {
            spdlog::error("Invalid OCPP CallResult message format");
            return false;
        }
        
        if (j[0] != static_cast<int>(MessageType::CallResult)) {
            spdlog::error("Message is not a CallResult message");
            return false;
        }
        
        messageId_ = j[1].get<std::string>();
        
        return setPayloadFromJson(j[2]);
    } catch (const std::exception& e) {
        spdlog::error("Error parsing CallResult message: {}", e.what());
        return false;
    }
}

CallError::CallError(const std::string& messageId, 
                     ErrorCode errorCode, 
                     const std::string& errorDescription,
                     const nlohmann::json& errorDetails)
    : Message(MessageType::CallError), 
      messageId_(messageId), 
      errorCode_(errorCode), 
      errorDescription_(errorDescription),
      errorDetails_(errorDetails) {}

const std::string& CallError::getMessageId() const {
    return messageId_;
}

CallError::ErrorCode CallError::getErrorCode() const {
    return errorCode_;
}

std::string CallError::getErrorCodeString() const {
    return errorCodeToString(errorCode_);
}

const std::string& CallError::getErrorDescription() const {
    return errorDescription_;
}

const nlohmann::json& CallError::getErrorDetails() const {
    return errorDetails_;
}

std::string CallError::toJson() const {
    nlohmann::json j = nlohmann::json::array();
    j.push_back(static_cast<int>(type_));
    j.push_back(messageId_);
    j.push_back(errorCodeToString(errorCode_));
    j.push_back(errorDescription_);
    j.push_back(errorDetails_);
    return j.dump();
}

bool CallError::fromJson(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);
        
        if (!j.is_array() || j.size() != 5) {
            spdlog::error("Invalid OCPP CallError message format");
            return false;
        }
        
        if (j[0] != static_cast<int>(MessageType::CallError)) {
            spdlog::error("Message is not a CallError message");
            return false;
        }
        
        messageId_ = j[1].get<std::string>();
        errorCode_ = stringToErrorCode(j[2].get<std::string>());
        errorDescription_ = j[3].get<std::string>();
        errorDetails_ = j[4];
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error parsing CallError message: {}", e.what());
        return false;
    }
}

std::string messageActionToString(MessageAction action) {
    switch (action) {
        case MessageAction::BootNotification: return "BootNotification";
        case MessageAction::Heartbeat: return "Heartbeat";
        case MessageAction::StatusNotification: return "StatusNotification";
        case MessageAction::TransactionEvent: return "TransactionEvent";
        case MessageAction::Authorize: return "Authorize";
        case MessageAction::RemoteStartTransaction: return "RemoteStartTransaction";
        case MessageAction::RemoteStopTransaction: return "RemoteStopTransaction";
        case MessageAction::UnlockConnector: return "UnlockConnector";
        case MessageAction::TriggerMessage: return "TriggerMessage";
        case MessageAction::MeterValues: return "MeterValues";
        case MessageAction::SetChargingProfile: return "SetChargingProfile";
        default: return "Unknown";
    }
}

MessageAction stringToMessageAction(const std::string& actionStr) {
    if (actionStr == "BootNotification") return MessageAction::BootNotification;
    if (actionStr == "Heartbeat") return MessageAction::Heartbeat;
    if (actionStr == "StatusNotification") return MessageAction::StatusNotification;
    if (actionStr == "TransactionEvent") return MessageAction::TransactionEvent;
    if (actionStr == "Authorize") return MessageAction::Authorize;
    if (actionStr == "RemoteStartTransaction") return MessageAction::RemoteStartTransaction;
    if (actionStr == "RemoteStopTransaction") return MessageAction::RemoteStopTransaction;
    if (actionStr == "UnlockConnector") return MessageAction::UnlockConnector;
    if (actionStr == "TriggerMessage") return MessageAction::TriggerMessage;
    if (actionStr == "MeterValues") return MessageAction::MeterValues;
    if (actionStr == "SetChargingProfile") return MessageAction::SetChargingProfile;
    return MessageAction::Unknown;
}

std::string errorCodeToString(CallError::ErrorCode errorCode) {
    switch (errorCode) {
        case CallError::ErrorCode::NotImplemented: return "NotImplemented";
        case CallError::ErrorCode::NotSupported: return "NotSupported";
        case CallError::ErrorCode::InternalError: return "InternalError";
        case CallError::ErrorCode::ProtocolError: return "ProtocolError";
        case CallError::ErrorCode::SecurityError: return "SecurityError";
        case CallError::ErrorCode::FormationViolation: return "FormationViolation";
        case CallError::ErrorCode::PropertyConstraintViolation: return "PropertyConstraintViolation";
        case CallError::ErrorCode::OccurrenceConstraintViolation: return "OccurrenceConstraintViolation";
        case CallError::ErrorCode::TypeConstraintViolation: return "TypeConstraintViolation";
        case CallError::ErrorCode::GenericError: return "GenericError";
        default: return "GenericError";
    }
}

CallError::ErrorCode stringToErrorCode(const std::string& errorCodeStr) {
    if (errorCodeStr == "NotImplemented") return CallError::ErrorCode::NotImplemented;
    if (errorCodeStr == "NotSupported") return CallError::ErrorCode::NotSupported;
    if (errorCodeStr == "InternalError") return CallError::ErrorCode::InternalError;
    if (errorCodeStr == "ProtocolError") return CallError::ErrorCode::ProtocolError;
    if (errorCodeStr == "SecurityError") return CallError::ErrorCode::SecurityError;
    if (errorCodeStr == "FormationViolation") return CallError::ErrorCode::FormationViolation;
    if (errorCodeStr == "PropertyConstraintViolation") return CallError::ErrorCode::PropertyConstraintViolation;
    if (errorCodeStr == "OccurrenceConstraintViolation") return CallError::ErrorCode::OccurrenceConstraintViolation;
    if (errorCodeStr == "TypeConstraintViolation") return CallError::ErrorCode::TypeConstraintViolation;
    return CallError::ErrorCode::GenericError;
}

std::shared_ptr<Message> createMessageFromJson(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);
        
        if (!j.is_array() || j.empty()) {
            spdlog::error("Invalid OCPP message format");
            return nullptr;
        }
        
        int messageTypeInt = j[0].get<int>();
        MessageType messageType = static_cast<MessageType>(messageTypeInt);
        
        // Factory pattern would be better here, but for simplicity we'll use a switch
        // In a real implementation, we would register message handlers for each action
        
        // This is a placeholder - actual implementation would create specific message types
        // based on the action (for Call) or the message ID (for CallResult/CallError)
        spdlog::warn("createMessageFromJson is not fully implemented");
        return nullptr;
        
    } catch (const std::exception& e) {
        spdlog::error("Error parsing message: {}", e.what());
        return nullptr;
    }
}

} // namespace ocpp
} // namespace ocpp_gateway