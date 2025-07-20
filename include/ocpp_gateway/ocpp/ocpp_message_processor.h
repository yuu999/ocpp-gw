#pragma once

#include <string>
#include <memory>
#include <map>
#include <functional>
#include <queue>
#include <mutex>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/error.h"

namespace ocpp_gateway {
namespace ocpp {

/**
 * @enum MessageType
 * @brief OCPP message types
 */
enum class MessageType {
    CALL = 2,
    CALL_RESULT = 3,
    CALL_ERROR = 4
};

/**
 * @enum OcppMessageAction
 * @brief OCPP message actions
 */
enum class OcppMessageAction {
    // Core Profile
    AUTHORIZE,
    BOOT_NOTIFICATION,
    CANCEL_RESERVATION,
    CHANGE_AVAILABILITY,
    CLEAR_CACHE,
    CLEAR_CHARGING_PROFILE,
    DATA_TRANSFER,
    GET_COMPOSITE_SCHEDULE,
    GET_CONFIGURATION,
    GET_DIAGNOSTICS,
    GET_LOCAL_LIST_VERSION,
    HEARTBEAT,
    METER_VALUES,
    REMOTE_START_TRANSACTION,
    REMOTE_STOP_TRANSACTION,
    RESET,
    SEND_LOCAL_LIST,
    SET_CHARGING_PROFILE,
    STATUS_NOTIFICATION,
    TRANSACTION_EVENT,
    TRIGGER_MESSAGE,
    UNLOCK_CONNECTOR,
    UPDATE_FIRMWARE,
    UNKNOWN
};

/**
 * @struct OcppMessage
 * @brief OCPP message structure
 */
struct OcppMessage {
    MessageType messageType;
    std::string messageId;
    OcppMessageAction action;
    nlohmann::json payload;
    
    /**
     * @brief Create a CALL message
     * @param messageId Message ID
     * @param action Action
     * @param payload Payload
     * @return OcppMessage
     */
    static OcppMessage createCall(
        const std::string& messageId,
        OcppMessageAction action,
        const nlohmann::json& payload);
    
    /**
     * @brief Create a CALL_RESULT message
     * @param messageId Message ID
     * @param payload Payload
     * @return OcppMessage
     */
    static OcppMessage createCallResult(
        const std::string& messageId,
        const nlohmann::json& payload);
    
    /**
     * @brief Create a CALL_ERROR message
     * @param messageId Message ID
     * @param errorCode Error code
     * @param errorDescription Error description
     * @param errorDetails Error details
     * @return OcppMessage
     */
    static OcppMessage createCallError(
        const std::string& messageId,
        const std::string& errorCode,
        const std::string& errorDescription,
        const nlohmann::json& errorDetails = nlohmann::json());
};

/**
 * @class OcppMessageHandler
 * @brief Interface for OCPP message handlers
 */
class OcppMessageHandler {
public:
    virtual ~OcppMessageHandler() = default;
    
    /**
     * @brief Handle an OCPP message
     * @param message OCPP message
     * @return Response message or nullptr if no response is needed
     */
    virtual std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message) = 0;
};

/**
 * @class OcppMessageProcessor
 * @brief Processes OCPP messages
 */
class OcppMessageProcessor : public std::enable_shared_from_this<OcppMessageProcessor> {
public:
    using MessageCallback = std::function<void(const std::string&)>;
    
    /**
     * @brief Create an OcppMessageProcessor instance
     * @param io_context Boost IO context
     * @return Shared pointer to OcppMessageProcessor
     */
    static std::shared_ptr<OcppMessageProcessor> create(boost::asio::io_context& io_context);
    
    /**
     * @brief Process an incoming OCPP message
     * @param message JSON message
     * @return true if message was processed successfully, false otherwise
     */
    bool processIncomingMessage(const std::string& message);
    
    /**
     * @brief Send an OCPP message
     * @param message OCPP message
     * @return true if message was sent or queued, false otherwise
     */
    bool sendMessage(const OcppMessage& message);
    
    /**
     * @brief Set the message callback
     * @param callback Function to handle outgoing messages
     */
    void setMessageCallback(MessageCallback callback);
    
    /**
     * @brief Register a message handler
     * @param action OCPP message action
     * @param handler Message handler
     */
    void registerHandler(OcppMessageAction action, std::shared_ptr<OcppMessageHandler> handler);
    
    /**
     * @brief Set connection status
     * @param connected true if connected, false otherwise
     */
    void setConnected(bool connected);
    
    /**
     * @brief Check if connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Get the number of queued messages
     * @return Number of queued messages
     */
    size_t getQueueSize() const;
    
    /**
     * @brief Clear the message queue
     */
    void clearQueue();
    
    /**
     * @brief Process queued messages
     * @return Number of messages processed
     */
    size_t processQueue();
    
    /**
     * @brief Convert action string to enum
     * @param action Action string
     * @return OcppMessageAction enum
     */
    static OcppMessageAction stringToAction(const std::string& action);
    
    /**
     * @brief Convert action enum to string
     * @param action OcppMessageAction enum
     * @return Action string
     */
    static std::string actionToString(OcppMessageAction action);

private:
    OcppMessageProcessor(boost::asio::io_context& io_context);
    
    /**
     * @brief Parse an OCPP message
     * @param message JSON message
     * @return Parsed OCPP message
     */
    OcppMessage parseMessage(const std::string& message);
    
    /**
     * @brief Serialize an OCPP message
     * @param message OCPP message
     * @return JSON string
     */
    std::string serializeMessage(const OcppMessage& message);
    
    /**
     * @brief Handle a parsed OCPP message
     * @param message OCPP message
     * @return Response message or nullptr if no response is needed
     */
    std::unique_ptr<OcppMessage> handleMessage(const OcppMessage& message);
    
    /**
     * @brief Send a message directly
     * @param message JSON message
     * @return true if message was sent, false otherwise
     */
    bool sendDirectMessage(const std::string& message);
    
    /**
     * @brief Queue a message for later sending
     * @param message JSON message
     */
    void queueMessage(const std::string& message);

    // Boost ASIO components
    boost::asio::io_context& io_context_;
    
    // Message handlers
    std::map<OcppMessageAction, std::shared_ptr<OcppMessageHandler>> handlers_;
    
    // Message callback
    MessageCallback message_callback_;
    
    // Message queue for offline operation
    std::queue<std::string> message_queue_;
    mutable std::mutex queue_mutex_;
    
    // Connection state
    std::atomic<bool> connected_;
    
    // Pending requests
    std::map<std::string, OcppMessageAction> pending_requests_;
    std::mutex pending_mutex_;
};

} // namespace ocpp
} // namespace ocpp_gateway