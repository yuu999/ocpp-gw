#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace ocpp_gateway {
namespace ocpp {

/**
 * @enum MessageType
 * @brief OCPP message types as defined in OCPP 2.0.1
 */
enum class MessageType {
    Call = 2,        // Request
    CallResult = 3,  // Response
    CallError = 4    // Error
};

/**
 * @enum MessageAction
 * @brief OCPP message actions as defined in OCPP 2.0.1
 */
enum class MessageAction {
    // Core Profile
    BootNotification,
    Heartbeat,
    StatusNotification,
    TransactionEvent,
    Authorize,
    RemoteStartTransaction,
    RemoteStopTransaction,
    UnlockConnector,
    TriggerMessage,
    MeterValues,
    SetChargingProfile,
    // Other profiles can be added as needed
    Unknown
};

/**
 * @class Message
 * @brief Base class for all OCPP messages
 */
class Message {
public:
    /**
     * @brief Constructor
     * @param type Message type (Call, CallResult, CallError)
     */
    explicit Message(MessageType type);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Message() = default;
    
    /**
     * @brief Get the message type
     * @return MessageType
     */
    MessageType getType() const;
    
    /**
     * @brief Serialize the message to JSON
     * @return JSON string representation of the message
     */
    virtual std::string toJson() const = 0;
    
    /**
     * @brief Deserialize the message from JSON
     * @param json JSON string to deserialize
     * @return true if deserialization was successful, false otherwise
     */
    virtual bool fromJson(const std::string& json) = 0;

protected:
    MessageType type_;
};

/**
 * @class Call
 * @brief OCPP Call message (request)
 */
class Call : public Message {
public:
    /**
     * @brief Constructor
     * @param messageId Unique message identifier
     * @param action OCPP action
     */
    Call(const std::string& messageId, MessageAction action);
    
    /**
     * @brief Get the message ID
     * @return Message ID string
     */
    const std::string& getMessageId() const;
    
    /**
     * @brief Get the action
     * @return MessageAction
     */
    MessageAction getAction() const;
    
    /**
     * @brief Get the action as string
     * @return Action string
     */
    std::string getActionString() const;
    
    /**
     * @brief Serialize the message to JSON
     * @return JSON string representation of the message
     */
    std::string toJson() const override;
    
    /**
     * @brief Deserialize the message from JSON
     * @param json JSON string to deserialize
     * @return true if deserialization was successful, false otherwise
     */
    bool fromJson(const std::string& json) override;
    
    /**
     * @brief Get the payload as JSON
     * @return JSON object containing the payload
     */
    virtual nlohmann::json getPayloadJson() const = 0;
    
    /**
     * @brief Set the payload from JSON
     * @param json JSON object containing the payload
     * @return true if successful, false otherwise
     */
    virtual bool setPayloadFromJson(const nlohmann::json& json) = 0;

protected:
    std::string messageId_;
    MessageAction action_;
};

/**
 * @class CallResult
 * @brief OCPP CallResult message (response)
 */
class CallResult : public Message {
public:
    /**
     * @brief Constructor
     * @param messageId Unique message identifier (must match the Call message ID)
     */
    explicit CallResult(const std::string& messageId);
    
    /**
     * @brief Get the message ID
     * @return Message ID string
     */
    const std::string& getMessageId() const;
    
    /**
     * @brief Serialize the message to JSON
     * @return JSON string representation of the message
     */
    std::string toJson() const override;
    
    /**
     * @brief Deserialize the message from JSON
     * @param json JSON string to deserialize
     * @return true if deserialization was successful, false otherwise
     */
    bool fromJson(const std::string& json) override;
    
    /**
     * @brief Get the payload as JSON
     * @return JSON object containing the payload
     */
    virtual nlohmann::json getPayloadJson() const = 0;
    
    /**
     * @brief Set the payload from JSON
     * @param json JSON object containing the payload
     * @return true if successful, false otherwise
     */
    virtual bool setPayloadFromJson(const nlohmann::json& json) = 0;

protected:
    std::string messageId_;
};

/**
 * @class CallError
 * @brief OCPP CallError message (error response)
 */
class CallError : public Message {
public:
    /**
     * @enum ErrorCode
     * @brief OCPP error codes as defined in OCPP 2.0.1
     */
    enum class ErrorCode {
        NotImplemented,
        NotSupported,
        InternalError,
        ProtocolError,
        SecurityError,
        FormationViolation,
        PropertyConstraintViolation,
        OccurrenceConstraintViolation,
        TypeConstraintViolation,
        GenericError
    };
    
    /**
     * @brief Constructor
     * @param messageId Unique message identifier (must match the Call message ID)
     * @param errorCode Error code
     * @param errorDescription Human-readable error description
     * @param errorDetails Additional error details (optional)
     */
    CallError(const std::string& messageId, 
              ErrorCode errorCode, 
              const std::string& errorDescription,
              const nlohmann::json& errorDetails = nlohmann::json());
    
    /**
     * @brief Get the message ID
     * @return Message ID string
     */
    const std::string& getMessageId() const;
    
    /**
     * @brief Get the error code
     * @return ErrorCode
     */
    ErrorCode getErrorCode() const;
    
    /**
     * @brief Get the error code as string
     * @return Error code string
     */
    std::string getErrorCodeString() const;
    
    /**
     * @brief Get the error description
     * @return Error description string
     */
    const std::string& getErrorDescription() const;
    
    /**
     * @brief Get the error details
     * @return JSON object containing error details
     */
    const nlohmann::json& getErrorDetails() const;
    
    /**
     * @brief Serialize the message to JSON
     * @return JSON string representation of the message
     */
    std::string toJson() const override;
    
    /**
     * @brief Deserialize the message from JSON
     * @param json JSON string to deserialize
     * @return true if deserialization was successful, false otherwise
     */
    bool fromJson(const std::string& json) override;

protected:
    std::string messageId_;
    ErrorCode errorCode_;
    std::string errorDescription_;
    nlohmann::json errorDetails_;
};

/**
 * @brief Convert MessageAction to string
 * @param action MessageAction enum value
 * @return String representation of the action
 */
std::string messageActionToString(MessageAction action);

/**
 * @brief Convert string to MessageAction
 * @param actionStr String representation of the action
 * @return MessageAction enum value
 */
MessageAction stringToMessageAction(const std::string& actionStr);

/**
 * @brief Convert CallError::ErrorCode to string
 * @param errorCode ErrorCode enum value
 * @return String representation of the error code
 */
std::string errorCodeToString(CallError::ErrorCode errorCode);

/**
 * @brief Convert string to CallError::ErrorCode
 * @param errorCodeStr String representation of the error code
 * @return ErrorCode enum value
 */
CallError::ErrorCode stringToErrorCode(const std::string& errorCodeStr);

/**
 * @brief Create a message from JSON
 * @param json JSON string
 * @return Shared pointer to the created message, nullptr if creation failed
 */
std::shared_ptr<Message> createMessageFromJson(const std::string& json);

} // namespace ocpp
} // namespace ocpp_gateway