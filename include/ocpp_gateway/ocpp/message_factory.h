#pragma once

#include "ocpp_gateway/ocpp/message.h"
#include "ocpp_gateway/ocpp/boot_notification.h"
#include "ocpp_gateway/ocpp/heartbeat.h"
#include "ocpp_gateway/ocpp/status_notification.h"
#include "ocpp_gateway/ocpp/transaction_event.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

namespace ocpp_gateway {
namespace ocpp {

/**
 * @class MessageFactory
 * @brief Factory for creating OCPP messages
 */
class MessageFactory {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the singleton instance
     */
    static MessageFactory& getInstance();
    
    /**
     * @brief Create a message from JSON
     * @param json JSON string
     * @return Shared pointer to the created message, nullptr if creation failed
     */
    std::shared_ptr<Message> createMessage(const std::string& json);
    
    /**
     * @brief Register a message creator function for a specific action
     * @param action Message action
     * @param creator Creator function
     */
    void registerMessageCreator(MessageAction action, 
                               std::function<std::shared_ptr<Call>(const std::string&, const nlohmann::json&)> creator);
    
    /**
     * @brief Register a response creator function for a specific action
     * @param action Message action
     * @param creator Creator function
     */
    void registerResponseCreator(MessageAction action,
                                std::function<std::shared_ptr<CallResult>(const std::string&, const nlohmann::json&)> creator);

private:
    /**
     * @brief Constructor (private for singleton)
     */
    MessageFactory();
    
    /**
     * @brief Initialize the factory with default message creators
     */
    void initialize();
    
    /**
     * @brief Create a Call message from JSON
     * @param messageId Message ID
     * @param action Message action
     * @param payload JSON payload
     * @return Shared pointer to the created Call message, nullptr if creation failed
     */
    std::shared_ptr<Call> createCallMessage(const std::string& messageId, 
                                          MessageAction action, 
                                          const nlohmann::json& payload);
    
    /**
     * @brief Create a CallResult message from JSON
     * @param messageId Message ID
     * @param action Message action (inferred from previous Call)
     * @param payload JSON payload
     * @return Shared pointer to the created CallResult message, nullptr if creation failed
     */
    std::shared_ptr<CallResult> createCallResultMessage(const std::string& messageId,
                                                     MessageAction action,
                                                     const nlohmann::json& payload);
    
    /**
     * @brief Create a CallError message from JSON
     * @param messageId Message ID
     * @param errorCode Error code
     * @param errorDescription Error description
     * @param errorDetails Error details
     * @return Shared pointer to the created CallError message
     */
    std::shared_ptr<CallError> createCallErrorMessage(const std::string& messageId,
                                                   const std::string& errorCode,
                                                   const std::string& errorDescription,
                                                   const nlohmann::json& errorDetails);

private:
    std::unordered_map<MessageAction, 
                      std::function<std::shared_ptr<Call>(const std::string&, const nlohmann::json&)>> 
        callCreators_;
    
    std::unordered_map<MessageAction,
                      std::function<std::shared_ptr<CallResult>(const std::string&, const nlohmann::json&)>>
        resultCreators_;
};

} // namespace ocpp
} // namespace ocpp_gateway