#include "ocpp_gateway/ocpp/message_factory.h"
#include <spdlog/spdlog.h>

namespace ocpp_gateway {
namespace ocpp {

MessageFactory& MessageFactory::getInstance() {
    static MessageFactory instance;
    return instance;
}

MessageFactory::MessageFactory() {
    initialize();
}

void MessageFactory::initialize() {
    // Register creators for Call messages
    registerMessageCreator(MessageAction::BootNotification, 
        [](const std::string& messageId, const nlohmann::json& payload) -> std::shared_ptr<Call> {
            auto request = std::make_shared<BootNotificationRequest>(
                messageId,
                BootNotificationRequest::Reason::PowerUp,  // Default reason
                BootNotificationRequest::ChargingStation{
                    "Unknown",  // Default model
                    "Unknown"   // Default vendor
                }
            );
            
            if (request->setPayloadFromJson(payload)) {
                return request;
            }
            
            return nullptr;
        }
    );
    
    registerMessageCreator(MessageAction::Heartbeat,
        [](const std::string& messageId, const nlohmann::json& payload) -> std::shared_ptr<Call> {
            auto request = std::make_shared<HeartbeatRequest>(messageId);
            
            if (request->setPayloadFromJson(payload)) {
                return request;
            }
            
            return nullptr;
        }
    );
    
    registerMessageCreator(MessageAction::StatusNotification,
        [](const std::string& messageId, const nlohmann::json& payload) -> std::shared_ptr<Call> {
            auto request = std::make_shared<StatusNotificationRequest>(
                messageId,
                std::chrono::system_clock::now(),  // Default timestamp
                0,                                 // Default connector ID
                ConnectorStatus::Available,        // Default status
                1                                  // Default EVSE ID
            );
            
            if (request->setPayloadFromJson(payload)) {
                return request;
            }
            
            return nullptr;
        }
    );
    
    registerMessageCreator(MessageAction::TransactionEvent,
        [](const std::string& messageId, const nlohmann::json& payload) -> std::shared_ptr<Call> {
            // Create a default transaction event request
            Transaction transaction{"unknown", std::nullopt, std::nullopt, std::nullopt, std::nullopt};
            EVSE evse{1, std::nullopt};
            
            auto request = std::make_shared<TransactionEventRequest>(
                messageId,
                TransactionEventType::Started,     // Default event type
                std::chrono::system_clock::now(),  // Default timestamp
                TriggerReason::Authorized,         // Default trigger reason
                0,                                 // Default sequence number
                transaction,                       // Default transaction
                evse                               // Default EVSE
            );
            
            if (request->setPayloadFromJson(payload)) {
                return request;
            }
            
            return nullptr;
        }
    );
    
    // Register creators for CallResult messages
    registerResponseCreator(MessageAction::BootNotification,
        [](const std::string& messageId, const nlohmann::json& payload) -> std::shared_ptr<CallResult> {
            auto response = std::make_shared<BootNotificationResponse>(
                messageId,
                std::chrono::system_clock::now(),  // Default current time
                300,                               // Default interval
                RegistrationStatus::Accepted       // Default status
            );
            
            if (response->setPayloadFromJson(payload)) {
                return response;
            }
            
            return nullptr;
        }
    );
    
    registerResponseCreator(MessageAction::Heartbeat,
        [](const std::string& messageId, const nlohmann::json& payload) -> std::shared_ptr<CallResult> {
            auto response = std::make_shared<HeartbeatResponse>(
                messageId,
                std::chrono::system_clock::now()  // Default current time
            );
            
            if (response->setPayloadFromJson(payload)) {
                return response;
            }
            
            return nullptr;
        }
    );
    
    registerResponseCreator(MessageAction::StatusNotification,
        [](const std::string& messageId, const nlohmann::json& payload) -> std::shared_ptr<CallResult> {
            auto response = std::make_shared<StatusNotificationResponse>(messageId);
            
            if (response->setPayloadFromJson(payload)) {
                return response;
            }
            
            return nullptr;
        }
    );
    
    registerResponseCreator(MessageAction::TransactionEvent,
        [](const std::string& messageId, const nlohmann::json& payload) -> std::shared_ptr<CallResult> {
            auto response = std::make_shared<TransactionEventResponse>(messageId);
            
            if (response->setPayloadFromJson(payload)) {
                return response;
            }
            
            return nullptr;
        }
    );
}

void MessageFactory::registerMessageCreator(
    MessageAction action,
    std::function<std::shared_ptr<Call>(const std::string&, const nlohmann::json&)> creator) {
    callCreators_[action] = creator;
}

void MessageFactory::registerResponseCreator(
    MessageAction action,
    std::function<std::shared_ptr<CallResult>(const std::string&, const nlohmann::json&)> creator) {
    resultCreators_[action] = creator;
}

std::shared_ptr<Message> MessageFactory::createMessage(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);
        
        if (!j.is_array() || j.empty()) {
            spdlog::error("Invalid OCPP message format");
            return nullptr;
        }
        
        int messageTypeInt = j[0].get<int>();
        MessageType messageType = static_cast<MessageType>(messageTypeInt);
        
        switch (messageType) {
            case MessageType::Call: {
                if (j.size() != 4) {
                    spdlog::error("Invalid Call message format");
                    return nullptr;
                }
                
                std::string messageId = j[1].get<std::string>();
                std::string actionStr = j[2].get<std::string>();
                MessageAction action = stringToMessageAction(actionStr);
                
                if (action == MessageAction::Unknown) {
                    spdlog::error("Unknown action: {}", actionStr);
                    return nullptr;
                }
                
                return createCallMessage(messageId, action, j[3]);
            }
            
            case MessageType::CallResult: {
                if (j.size() != 3) {
                    spdlog::error("Invalid CallResult message format");
                    return nullptr;
                }
                
                std::string messageId = j[1].get<std::string>();
                
                // In a real implementation, we would look up the action from the original Call
                // For now, we'll just use a placeholder
                MessageAction action = MessageAction::Unknown;
                
                return createCallResultMessage(messageId, action, j[2]);
            }
            
            case MessageType::CallError: {
                if (j.size() != 5) {
                    spdlog::error("Invalid CallError message format");
                    return nullptr;
                }
                
                std::string messageId = j[1].get<std::string>();
                std::string errorCode = j[2].get<std::string>();
                std::string errorDescription = j[3].get<std::string>();
                nlohmann::json errorDetails = j[4];
                
                return createCallErrorMessage(messageId, errorCode, errorDescription, errorDetails);
            }
            
            default:
                spdlog::error("Unknown message type: {}", messageTypeInt);
                return nullptr;
        }
    } catch (const std::exception& e) {
        spdlog::error("Error parsing message: {}", e.what());
        return nullptr;
    }
}

std::shared_ptr<Call> MessageFactory::createCallMessage(
    const std::string& messageId,
    MessageAction action,
    const nlohmann::json& payload) {
    
    auto it = callCreators_.find(action);
    
    if (it != callCreators_.end()) {
        return it->second(messageId, payload);
    }
    
    spdlog::error("No creator registered for action: {}", messageActionToString(action));
    return nullptr;
}

std::shared_ptr<CallResult> MessageFactory::createCallResultMessage(
    const std::string& messageId,
    MessageAction action,
    const nlohmann::json& payload) {
    
    auto it = resultCreators_.find(action);
    
    if (it != resultCreators_.end()) {
        return it->second(messageId, payload);
    }
    
    spdlog::error("No response creator registered for action: {}", messageActionToString(action));
    return nullptr;
}

std::shared_ptr<CallError> MessageFactory::createCallErrorMessage(
    const std::string& messageId,
    const std::string& errorCode,
    const std::string& errorDescription,
    const nlohmann::json& errorDetails) {
    
    CallError::ErrorCode code = stringToErrorCode(errorCode);
    
    return std::make_shared<CallError>(messageId, code, errorDescription, errorDetails);
}

} // namespace ocpp
} // namespace ocpp_gateway